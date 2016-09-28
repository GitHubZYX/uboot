/*
 * (C) Copyright 2002-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * To match the U-Boot user interface on ARM platforms to the U-Boot
 * standard (as on PPC platforms), some messages with debug character
 * are removed from the default U-Boot build.
 *
 * Define DEBUG here if you want additional info as shown below
 * printed upon startup:
 *
 * U-Boot code: 00F00000 -> 00F3C774  BSS: -> 00FC3274
 * IRQ Stack: 00ebff7c
 * FIQ Stack: 00ebef7c
 */

#include <common.h>
#include <command.h>
#include <version.h>
#include <serial.h>
#include <mmc.h>
#include <asm/armv7.h>

DECLARE_GLOBAL_DATA_PTR;

ulong monitor_flash_len;

/*
 ************************************************************************
 * Init Utilities							*
 ************************************************************************
 * Some of this code should be moved into the core functions,
 * or dropped completely,
 * but let's get it working (again) first...
 */

#if defined(CONFIG_ARM_DCC) && !defined(CONFIG_BAUDRATE)
#define CONFIG_BAUDRATE 115200
#endif

static int init_baudrate(void)
{
	gd->baudrate = CONFIG_BAUDRATE;
	return 0;
}

static int display_banner(void)
{
#ifdef CONFIG_COMIP_TARGETLOADER
	printf("\n\nTarget Loader (base on u-boot).\n");
#endif

	printf("\n\n%s\n\n", U_BOOT_VERSION_STRING);
	debug("U-Boot code: %08lX -> %08lX  BSS: -> %08lX\n",
	       _TEXT_BASE,
	       _bss_start_ofs + _TEXT_BASE, _bss_end_ofs + _TEXT_BASE);
#ifdef CONFIG_MODEM_SUPPORT
	debug("Modem Support enabled\n");
#endif
#ifdef CONFIG_USE_IRQ
	debug("IRQ Stack: %08lx\n", IRQ_STACK_START);
	debug("FIQ Stack: %08lx\n", FIQ_STACK_START);
#endif

	return (0);
}

/*
 * WARNING: this code looks "cleaner" than the PowerPC version, but
 * has the disadvantage that you either get nothing, or everything.
 * On PowerPC, you might see "DRAM: " before the system hangs - which
 * gives a simple yet clear indication which part of the
 * initialization if failing.
 */
static int display_dram_config(void)
{
	int i;

#ifdef DEBUG
	puts("RAM Configuration:\n");

	for (i = 0; i < gd->nr_dram_banks; i++) {
		printf("Bank #%d: %016llx ", i, gd->bd->bi_dram[i].start);
		print_size(gd->bd->bi_dram[i].size, "\n");
	}
#else
	unsigned long long size = 0;

	for (i = 0; i < gd->nr_dram_banks; i++)
		size += gd->bd->bi_dram[i].size;

	puts("DRAM:  ");
	print_size(size, "\n");
#endif

	return (0);
}

/*
 * Breathe some life into the board...
 *
 * Initialize a serial port as console, and carry out some hardware
 * tests.
 *
 * The first part of initialization is running from Flash memory;
 * its main purpose is to initialize the RAM so that we
 * can relocate the monitor code to RAM.
 */

/*
 * All attempts to come up with a "common" initialization sequence
 * that works for all boards and architectures failed: some of the
 * requirements are just _too_ different. To get rid of the resulting
 * mess of board dependent #ifdef'ed code we now make the whole
 * initialization sequence configurable to the user.
 *
 * The requirements for any new initalization function is simple: it
 * receives a pointer to the "global data" structure as it's only
 * argument, and returns an integer return code, where 0 means
 * "continue" and != 0 means "fatal error, hang the system".
 */
typedef int (init_fnc_t) (void);

void __dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size =  gd->ram_size;
}
void dram_init_banksize(void)
	__attribute__((weak, alias("__dram_init_banksize")));

init_fnc_t *init_sequence[] = {
#if defined(CONFIG_ARCH_CPU_INIT)
	arch_cpu_init,		/* basic arch cpu dependent setup */
#endif
#if defined(CONFIG_BOARD_EARLY_INIT_F)
	board_early_init_f,
#endif
	timer_init,		/* initialize timer */
	init_baudrate,		/* initialze baudrate settings */
	serial_init,		/* serial communications setup */
	display_banner,		/* say that we are here */
	dram_init,		/* configure available RAM banks */
	NULL,
};

void board_init_f(ulong bootflag)
{
	bd_t *bd;
	init_fnc_t **init_fnc_ptr;
	gd_t *id;
	ulong addr, addr_sp;

	/* Pointer is writable since we allocated a register for it */
	gd = (gd_t *) ((CONFIG_SYS_INIT_SP_ADDR) & ~0x07);
	/* compiler optimization barrier needed for GCC >= 3.4 */
	__asm__ __volatile__("": : :"memory");

	memset((void *)gd, 0, sizeof(gd_t));

	gd->mon_len = _end_ofs;

#ifdef CONFIG_OF_EMBED
	/* Get a pointer to the FDT */
	gd->fdt_blob = _binary_dt_dtb_start;
#elif defined CONFIG_OF_SEPARATE
	/* FDT is at end of image */
	gd->fdt_blob = (void *)(_end_ofs + _TEXT_BASE);
#endif

#if defined(CONFIG_DEBUG_TIMER_ID)
	debug_timer_init();
#endif

	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		if ((*init_fnc_ptr)() != 0) {
			hang ();
		}
	}

	debug("monitor len: %08lX\n", gd->mon_len);
	/*
	 * Ram is setup, size stored in gd !!
	 */
	debug("ramsize: %08lX\n", gd->ram_size);

#if defined(CONFIG_DDR_MEMTEST)
	memory_test();
#endif

#if defined(CONFIG_COMIP_TARGETLOADER)
	extern void targetloader_config(void);
	targetloader_config();
#endif

#if defined(CONFIG_SYS_MEM_TOP_HIDE)
	/*
	 * Subtract specified amount of memory to hide so that it won't
	 * get "touched" at all by U-Boot. By fixing up gd->ram_size
	 * the Linux kernel should now get passed the now "corrected"
	 * memory size and won't touch it either. This should work
	 * for arch/ppc and arch/powerpc. Only Linux board ports in
	 * arch/powerpc with bootwrapper support, that recalculate the
	 * memory size from the SDRAM controller setup will have to
	 * get fixed.
	 */
	gd->ram_size -= CONFIG_SYS_MEM_TOP_HIDE;
#endif

	addr = CONFIG_SYS_SDRAM_BASE + gd->ram_size;

#if !(defined(CONFIG_SYS_ICACHE_OFF) && defined(CONFIG_SYS_DCACHE_OFF))
	/* reserve TLB table */
	addr -= (4096 * 4);

	/* round down to next 64 kB limit */
	addr &= ~(0x10000 - 1);

	gd->tlb_addr = addr;
	debug("TLB table at: %08lx\n", addr);
#endif

	/* round down to next 4 kB limit */
	addr &= ~(4096 - 1);
	debug("Top of RAM usable for U-Boot at: %08lx\n", addr);

	/*
	 * reserve memory for U-Boot code, data & bss
	 * round down to next 4 kB limit
	 */
	addr -= gd->mon_len;
	addr &= ~(4096 - 1);

	debug("Reserving %ldk for U-Boot at: %08lx\n", gd->mon_len >> 10, addr);

#ifndef CONFIG_SPL_BUILD
	/*
	 * reserve memory for malloc() arena
	 */
	addr_sp = addr - TOTAL_MALLOC_LEN;
	debug("Reserving %dk for malloc() at: %08lx\n",
			TOTAL_MALLOC_LEN >> 10, addr_sp);
	/*
	 * (permanently) allocate a Board Info struct
	 * and a permanent copy of the "global" data
	 */
	addr_sp -= sizeof (bd_t);
	bd = (bd_t *) addr_sp;
	gd->bd = bd;
	debug("Reserving %zu Bytes for Board Info at: %08lx\n",
			sizeof (bd_t), addr_sp);

#ifdef CONFIG_MACH_TYPE
	gd->bd->bi_arch_number = CONFIG_MACH_TYPE; /* board id for Linux */
#endif

	addr_sp -= sizeof (gd_t);
	id = (gd_t *) addr_sp;
	debug("Reserving %zu Bytes for Global Data at: %08lx\n",
			sizeof (gd_t), addr_sp);

	/* setup stackpointer for exeptions */
	gd->irq_sp = addr_sp;
#ifdef CONFIG_USE_IRQ
	addr_sp -= (CONFIG_STACKSIZE_IRQ + CONFIG_STACKSIZE_FIQ);
	debug("Reserving %zu Bytes for IRQ stack at: %08lx\n",
		CONFIG_STACKSIZE_IRQ+CONFIG_STACKSIZE_FIQ, addr_sp);
#endif
	/* leave 3 words for abort-stack    */
	addr_sp -= 12;

	/* 8-byte alignment for ABI compliance */
	addr_sp &= ~0x07;
#else
	addr_sp += 128;	/* leave 32 words for abort-stack   */
	gd->irq_sp = addr_sp;
#endif

	debug("New Stack Pointer is: %08lx\n", addr_sp);

#ifdef CONFIG_POST
	post_bootmode_init();
	post_run(NULL, POST_ROM | post_bootmode_get(0));
#endif

	gd->bd->bi_baudrate = gd->baudrate;
	/* Ram ist board specific, so move it to board code ... */
	dram_init_banksize();
	display_dram_config();	/* and display it */

	/*m0_boot();*/	/*boot m0*/
	gd->relocaddr = addr;
#if defined(CONFIG_COMIP_FASTBOOT) && defined(CONFIG_LCD_SUPPORT)
	/*FIXME, reserved framebuffer for logo*/
	gd->start_addr_sp = addr_sp - CONFIG_FB_MEMORY_NUM * CONFIG_FB_MEMORY_SIZE;
#else
	gd->start_addr_sp = addr_sp;
#endif
	gd->reloc_off = addr - _TEXT_BASE;
	debug("relocation Offset is: %08lx\n", gd->reloc_off);
	memcpy(id, (void *)gd, sizeof(gd_t));

	relocate_code(addr_sp, id, addr);

	/* NOTREACHED - relocate_code() does not return */
}

static struct tag *params;
/*
 * CP15 Barrier instructions
 * Please note that we have separate barrier instructions in ARMv7
 * However, we use the CP15 based instructtions because we use
 * -march=armv5 in U-Boot
 */
//#define CP15ISB	asm volatile ("mcr     p15, 0, %0, c7, c5, 4" : : "r" (0))
//#define CP15DSB	asm volatile ("mcr     p15, 0, %0, c7, c10, 4" : : "r" (0))
//#define CP15DMB	asm volatile ("mcr     p15, 0, %0, c7, c10, 5" : : "r" (0))

void do_bootm_linux(void)
{
	bd_t *bd = gd->bd;
	void (*theKernel) (int zero, int arch, uint params);
	theKernel = (void (*)(int, int, uint))CONFIG_KERNEL_LOADADDR;

	params = (struct tag *)bd->bi_boot_params;

	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size(tag_core);
	params->u.core.flags = 0;
	params->u.core.pagesize = 0;
	params->u.core.rootdev = 0;
	params = tag_next(params);

	params = comip_set_boot_params(params);

	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;

	/* we assume that the kernel is in place */
	printf("\nStarting kernel ...\n");
	cleanup_before_linux();
	theKernel(0, bd->bi_arch_number, bd->bi_boot_params);
}
/*
 ************************************************************************
 *
 * This is the next part if the initialization sequence: we are now
 * running from RAM and have a "normal" C environment, i. e. global
 * data can be written, BSS has been cleared, the stack size in not
 * that critical any more, etc.
 *
 ************************************************************************
 */
void board_init_r(gd_t *id, ulong dest_addr)
{
	gd = id;

	gd->flags |= GD_FLG_RELOC;	/* tell others: relocation done */

	monitor_flash_len = _end_ofs;

	debug("monitor flash len: %08lX\n", monitor_flash_len);
	board_init();	/* Setup chipselects */

#if defined(CONFIG_MISC_INIT_R)
	/* miscellaneous platform dependent initialisations */
	misc_init_r();
#endif

#if defined(CONFIG_USE_IRQ)
	/* set up exceptions */
	interrupt_init();

	/* enable exceptions */
	enable_interrupts();
	printf("init interrupt done!\n");
#endif

#if defined(CONFIG_COMIP_FASTBOOT) && defined(CONFIG_LCD_SUPPORT)
	if (gd->fastboot) {
	/*register lcd support*/
#if defined (CONFIG_LCD_AUO_OTM1285A_OTP)
		extern int lcd_auo_otm1285a_otp_init(void);
		lcd_auo_otm1285a_otp_init();
#endif
#if defined (CONFIG_LCD_AUO_R61308OTP)
		extern int lcd_auo_r61308opt_init(void);
		lcd_auo_r61308opt_init();
#endif
#if defined (CONFIG_LCD_AUO_NT35521)
		extern int lcd_auo_nt35521_init(void);
		lcd_auo_nt35521_init();
#endif
#if defined (CONFIG_LCD_SHARP_R69431)
		extern int lcd_sharp_eR69431_init(void);
		lcd_sharp_eR69431_init();
#endif
		/*initialize lcdc & display logo*/
		extern int comipfb_probe(void);
		comipfb_probe();
	}
#endif

#if defined(CONFIG_COMIP_TARGETLOADER)
	extern int targetloader_init(void);
	targetloader_init();
#elif defined(CONFIG_COMIP_FASTBOOT)
	if (gd->fastboot) {
		extern int fastboot_init(void);
		fastboot_init();
		while(1);
	}
#endif

#if defined(CONFIG_ENABLE_SECURE_VERIFY_FOR_BOOT)
        extern  int secure_verify(void);
        extern void pmic_power_off(void);
        if(secure_verify()) {
                printf("Secure verify failed! Shutdown now!\n");
                pmic_power_off();
        } else {
                printf("Secure verify succeed!\n");
        }
#endif

	do_bootm_linux();

	/* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;) {
		//main_loop();
	}

	/* NOTREACHED - no way out of command loop except booting */
}

void hang(void)
{
	puts("### ERROR ### Please RESET the board ###\n");
	for (;;);
}
