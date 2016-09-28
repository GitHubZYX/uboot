/*
 * (C) Copyright 2002-2010
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

#ifndef	__ASM_GBL_DATA_H
#define __ASM_GBL_DATA_H
/*
 * The following data structure is placed in some memory which is
 * available very early after boot (like DPRAM on MPC8xx/MPC82xx, or
 * some locked parts of the data cache) to allow for a minimum set of
 * global variables during system initialization (until we have set
 * up the memory controller so that we can use RAM).
 *
 * Keep it *SMALL* and remember to set GENERATED_GBL_DATA_SIZE > sizeof(gd_t)
 */

typedef	struct	global_data {
	bd_t		*bd;
	unsigned long	flags;
	unsigned long	baudrate;
	unsigned long	have_console;	/* serial_init() was called */
#ifdef CONFIG_PRE_CONSOLE_BUFFER
	unsigned long	precon_buf_idx;	/* Pre-Console buffer index */
#endif
	unsigned long	env_addr;	/* Address  of Environment struct */
	unsigned long	env_valid;	/* Checksum of Environment valid? */
#ifdef CONFIG_FSL_ESDHC
	unsigned long	sdhc_clk;
#endif
#ifdef CONFIG_AT91FAMILY
	/* "static data" needed by at91's clock.c */
	unsigned long	cpu_clk_rate_hz;
	unsigned long	main_clk_rate_hz;
	unsigned long	mck_rate_hz;
	unsigned long	plla_rate_hz;
	unsigned long	pllb_rate_hz;
	unsigned long	at91_pllb_usb_init;
#endif
#ifdef CONFIG_ARM
	/* "static data" needed by most of timer.c on ARM platforms */
	unsigned long	timer_rate_hz;
	unsigned long	tbl;
	unsigned long	tbu;
	unsigned long long	timer_reset_value;
	unsigned long	lastinc;
#endif
#ifdef CONFIG_IXP425
	unsigned long	timestamp;
#endif
	unsigned long	relocaddr;	/* Start address of U-Boot in RAM */
	phys_size_t	ram_size;	/* RAM size */
	unsigned long	mon_len;	/* monitor len */
	unsigned long	irq_sp;		/* irq stack pointer */
	unsigned long	start_addr_sp;	/* start_addr_stackpointer */
	unsigned long	reloc_off;
#if !(defined(CONFIG_SYS_ICACHE_OFF) && defined(CONFIG_SYS_DCACHE_OFF))
	unsigned long	tlb_addr;
#endif
	const void	*fdt_blob;	/* Our device tree, NULL if none */
	void		**jt;		/* jump table */
	char		env_buf[32];	/* buffer for getenv() before reloc. */
#if defined(CONFIG_POST) || defined(CONFIG_LOGBUFFER)
	unsigned long	post_log_word; /* Record POST activities */
	unsigned long	post_log_res; /* success of POST test */
	unsigned long	post_init_f_time; /* When post_init_f started */
#endif
#if defined(CONFIG_COMIP)
	unsigned long	pmic_info[32];
	unsigned long	boot_mode;
	unsigned long	chip_id;
	unsigned long	rom_id;
	unsigned long	bb_id;
	unsigned long	sn_id;
	unsigned long	cpu_freq;
	unsigned long	ddr_freq;
	u64		memctl0_cs0_size;
	u64		memctl0_cs1_size;
	u64		memctl1_cs0_size;
	u64		memctl1_cs1_size;
	u8		ddr_2cs_signal;
	u8		nr_dram_banks;
	u64		dram_base;
	u64		dram_size;
	u64		dram2_base;
	u64		dram2_size;
	u64		fb_base;
	u64		fb_size;
	u8		fb_num;
	u8		fb_fix;
	u64		on2_base;
	u64		on2_size;
	u8		on2_fix;
#if defined(CONFIG_SAVE_DDR_ON_FLASH)
	unsigned long	g_ddr_on_flash[16];
#endif
#if defined(CONFIG_COMIP_EMMC_ENHANCE)
	unsigned long 	g_mmc_usedma;
	char		g_mmc_info[CONFIG_MMC_STRUCT_IN_GD];
	unsigned long	g_mmc_info_used;
	void *		g_mmc_devices;
	void *		g_comip_mmc_host[4];
	int		g_cur_dev_num;
#endif
#if defined(CONFIG_COMIP_FASTBOOT)
	unsigned long	fastboot;
	unsigned char 	fastboot_device_id[28];
#endif
#if defined(CONFIG_DEBUG_TIMER_ID)
	unsigned long	showtime;
#endif
#if defined(CONFIG_LEDS_AW2013)
	unsigned long blink_delay_on[COLOR_MAX];
	unsigned long blink_delay_off[COLOR_MAX];
#endif
#endif
#if defined(CONFIG_GATE_TRAINING_SUPPORT)
	unsigned long need_gate_training;
	unsigned long emmc_mid;
	unsigned long emmc_did;
#endif
} gd_t;

/*
 * Global Data Flags
 */
#define	GD_FLG_RELOC		0x00001	/* Code was relocated to RAM		*/
#define	GD_FLG_DEVINIT		0x00002	/* Devices have been initialized	*/
#define	GD_FLG_SILENT		0x00004	/* Silent mode				*/
#define	GD_FLG_POSTFAIL		0x00008	/* Critical POST test failed		*/
#define	GD_FLG_POSTSTOP		0x00010	/* POST seqeunce aborted		*/
#define	GD_FLG_LOGINIT		0x00020	/* Log Buffer has been initialized	*/
#define GD_FLG_DISABLE_CONSOLE	0x00040	/* Disable console (in & out)		*/
#define GD_FLG_ENV_READY	0x00080	/* Environment imported into hash table	*/

#define DECLARE_GLOBAL_DATA_PTR     register volatile gd_t *gd asm ("r8")

#endif /* __ASM_GBL_DATA_H */
