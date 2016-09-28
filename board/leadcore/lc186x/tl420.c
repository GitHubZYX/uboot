#include <common.h>
#include <asm/io.h>
#include <asm/armv7.h>

#include "tl420.h"

DECLARE_GLOBAL_DATA_PTR;

#if CONFIG_DDR_TOP_NOT_SLEEP
#define TL420_MEMCTL_PD_ENABLE		(0)
#define TL420_TOP_PD_ENABLE		(0)
#else
#define TL420_MEMCTL_PD_ENABLE		(1)
#define TL420_TOP_PD_ENABLE		(1)
#endif

#define TL420_AP_OSCEN			(232)
#define TL420_CP_OSCEN			(231)

extern char tl420_input_code[];
extern char tl420_input_code_end[];
extern char tl420_input_data[];
extern char tl420_input_data_end[];
extern struct memctl_data *memctl_init_datas;
extern unsigned int memctl_init_datas_length;

extern const unsigned int memctl_dqs_length;
extern struct memctl_data *memctl1_dqs;
extern struct memctl_data *memctl0_dqs;

extern const struct memctl_data memctl0_lp_datas[];
extern const unsigned int memctl0_lp_datas_length;
extern const struct memctl_data memctl1_lp_datas[];
extern const unsigned int memctl1_lp_datas_length;

extern void  memctl_low_power_enable(void);
extern void memctl_power_down_mask(unsigned int mask);

#define TL420_RECOVERY_RAM	(TOP_RAM1_BASE + 0x00020000)
#define TL420_CODE_RAM		(TL420_RECOVERY_RAM)
#define TL420_DATA_RAM		(TL420_RECOVERY_RAM + 0x0000C000)
#define TL420_MEMCTL_DATA_RAM	(TL420_RECOVERY_RAM + 0x0000E000)
#define TL420_MEMCTL_DATA_SIZE	(0x00002000 - TL420_ARGS_SIZE)
#define TL420_ARGS_SIZE		(256)
#define TL420_ARGS_RAM		(TL420_RECOVERY_RAM + 0x00010000 - TL420_ARGS_SIZE)

static int tl420_boot_prepare(void)
{
	unsigned int val;

	writel(0x00, DDR_PWR_TL420BOOTADDRSEL);

	/* Reset. */
	val = (1 << TL420_SF_RST) | (1 << (TL420_SF_RST + 16));
	writel(val, DDR_PWR_RESET);

	/* Power on. */
	val = (1 << TL420_WK_UP) | (1 << (TL420_WK_UP + 16));
	writel(val, DDR_PWR_TL420_PD_CTL);

	return 0;
}

static int tl420_boot_end(void)
{
	unsigned int val;
	unsigned int timeout = 100000;

	while (!(readl(DDR_PWR_INTR_RAW) & (1 << TL420_PU_INTR)) && timeout--)
		udelay(1);

	if (!(readl(DDR_PWR_INTR_RAW) & (1 << TL420_PU_INTR)))
		return -1;

	/* Release reset. */
	val = (1 << (TL420_SF_RST + 16));
	writel(val, DDR_PWR_RESET);

	mdelay(8);
	return 0;
}

static int tl420_config_wakeup_gpio(struct tl420_args *args)
{
	unsigned int val;
	args->bb_id = gd->chip_id;
#if TL420_TOP_PD_ENABLE
	if (0 == gd->rom_id){
		/* For the first version. */
		/* Config test pin for ap_oscen and cp_oscen. */
		comip_mfp_config_pull(TL420_AP_OSCEN, MFP_PULL_DISABLE);
		comip_mfp_config(TL420_AP_OSCEN, MFP_PIN_MODE_1);

		comip_mfp_config_pull(TL420_CP_OSCEN, MFP_PULL_DISABLE);
		comip_mfp_config(TL420_CP_OSCEN, MFP_PIN_MODE_1);

		writel(2, MUXPIN_TESTPIN_CTRL0);

		args->sleep_gpio = 224;
		comip_mfp_config(args->sleep_gpio, MFP_PIN_MODE_GPIO);
		comip_mfp_config_pull(args->sleep_gpio, MFP_PULL_DISABLE);

		/* Low means system sleep. */
		args->gpio_sleep_status = 0;
	} else {
		/* For the eco version. */
		writel(4, MUXPIN_TESTPIN_CTRL0);

		args->sleep_gpio = 233;
		comip_mfp_config(args->sleep_gpio, MFP_PIN_MODE_1);
		comip_mfp_config_pull(args->sleep_gpio, MFP_PULL_DISABLE);

		/* High means system sleep. */
		args->gpio_sleep_status = 1;
	}

	gpio_set_debounce(args->sleep_gpio, 1);

	printf("use gpio %d for sleep wakeup\n", args->sleep_gpio);

	/* Config TL420 ICTL irq enable. */
	val = (1 << TL420_ICTL_IRQ_GPIO);
	writel(val, TL420_ICTL_IRQ_INTEN);

	val = ~(1 << TL420_ICTL_IRQ_GPIO);
	writel(val, TL420_ICTL_IRQ_INTMASK);
#else
	args->sleep_gpio = -1;

	/* Mask TL420 ICTL irq, irq is disable default. */
	writel(0xffffffff, TL420_ICTL_IRQ_INTMASK);
#endif
	return 0;
}

static int tl420_misc_prepare(struct tl420_args *args)
{
#if TL420_MEMCTL_PD_ENABLE
	/* Memctl low power. */
	memctl_low_power_enable();

	/* Unmask memctl power down. */
	memctl_power_down_mask(0);
#else
	/* Memctl low power. */
	memctl_low_power_enable();

	printf("set memctl not power down\n");
	/* Mask memctl power down. */
	memctl_power_down_mask(1);
#endif
	tl420_config_wakeup_gpio(args);

	return 0;
}

static int tl420_set_args(struct tl420_args *args)
{
	unsigned int tl420_args_length = 0;
	memset(args, 0, sizeof(struct tl420_args));

#if TL420_MEMCTL_PD_ENABLE
	/* MUST Enable CKE fix. */
	args->flags |= TL420_MEMCTL_CKE_FIX;
	/* Enable low power mode. */
	args->flags |= TL420_MEMCTL_LP_EN;
	/* Enable power down. */
	args->flags |= TL420_MEMCTL_PD_EN;
#endif

	/* Enable dqs. */
	args->flags |= TL420_MEMCTL_DQS_EN;

	/* Dump & Print. */
	args->flags |= TL420_PRINT_ENABLE;
	args->flags |= TL420_DUMP_ENABLE;
	//args->flags |= TL420_DUMP_FORCE; // only for debug.
	args->dump_type = DUMP_SERIAL_DEBUG;

#if 1
	args->debug_gpio = -1;
	args->debug_gpio_ddr = -1;
#else
	args->flags |= TL420_GPIO_DBG_ENABLE;
	args->debug_gpio = 71;
	comip_mfp_config(args->debug_gpio, MFP_PIN_MODE_GPIO);
	args->debug_gpio_ddr = 246;
	comip_mfp_config(args->debug_gpio_ddr, MFP_PIN_MODE_GPIO);
	args->flags |= TL420_OSCEN_SHOW_ENABLE;
#endif

	args->memctl_para_size = memctl_init_datas_length / sizeof(struct memctl_data);
	args->memctl_para_addr = TL420_MEMCTL_DATA_RAM;
	tl420_args_length += memctl_init_datas_length;

	args->memctl0_lp_size = memctl0_lp_datas_length / sizeof(struct memctl_data);
	args->memctl0_lp_addr = TL420_MEMCTL_DATA_RAM + tl420_args_length;
	tl420_args_length += memctl0_lp_datas_length;

	args->memctl1_lp_size = memctl1_lp_datas_length / sizeof(struct memctl_data);
	args->memctl1_lp_addr = TL420_MEMCTL_DATA_RAM + tl420_args_length;
	tl420_args_length += memctl1_lp_datas_length;

	/* Memctl useage. */
	if (gd->dram_size) {
		args->flags |= TL420_MEMCTL_USE_0;
		args->memctl0_dqs_size = memctl_dqs_length / sizeof(struct memctl_data);
		args->memctl0_dqs_addr = TL420_MEMCTL_DATA_RAM + tl420_args_length;
		tl420_args_length += memctl_dqs_length;
	}

	if (gd->memctl1_cs0_size || gd->memctl1_cs1_size) {
		args->flags |= TL420_MEMCTL_USE_1;
		args->memctl1_dqs_size = memctl_dqs_length / sizeof(struct memctl_data);
		args->memctl1_dqs_addr = TL420_MEMCTL_DATA_RAM + tl420_args_length;
		tl420_args_length += memctl_dqs_length;
	}

	if ((tl420_args_length	> TL420_MEMCTL_DATA_SIZE)
		|| (args->memctl_para_addr % 8)
		|| (args->memctl0_lp_addr % 8)
		|| (args->memctl1_lp_addr % 8)
		|| (args->memctl0_dqs_addr % 8)
		|| (args->memctl1_dqs_addr % 8)
		|| (TL420_ARGS_SIZE < sizeof(struct tl420_args))) {
		printf("TL420: memctl parameter data size(%x) is too large"
			"or address(0x%08x,0x%08x,0x%08x,0x%08x,0x%08x) is invalid "
			"or struct tl420_args size(%d) is too large!!\n",
			tl420_args_length,
			args->memctl_para_addr,
			args->memctl0_lp_addr,
			args->memctl1_lp_addr,
			args->memctl0_dqs_addr,
			args->memctl1_dqs_addr,
			sizeof(struct tl420_args));
		return -1;
	}

	return 0;
}

static int tl420_boot(void)
{
	unsigned int code_addr = TL420_CODE_RAM;
	unsigned int data_addr = TL420_DATA_RAM;
	unsigned int args_addr = TL420_ARGS_RAM;
	unsigned int code_len = tl420_input_code_end - tl420_input_code;
	unsigned int data_len = tl420_input_data_end - tl420_input_data;
	unsigned int args_len = sizeof(struct tl420_args);
	struct tl420_args args;

	if (tl420_set_args(&args))
		return -1;

	if (tl420_misc_prepare(&args))
		return -1;

	if (tl420_boot_prepare())
		return -1;

	memcpy((char *)code_addr, tl420_input_code, code_len);
	memcpy((char *)data_addr, tl420_input_data, data_len);
	memcpy((char *)args.memctl_para_addr, 
			(char *)memctl_init_datas,
			memctl_init_datas_length);
	memcpy((char *)args.memctl0_lp_addr,
			(char *)memctl0_lp_datas,
			memctl0_lp_datas_length);
	memcpy((char *)args.memctl1_lp_addr,
			(char *)memctl1_lp_datas,
			memctl1_lp_datas_length);
	memcpy((char *)args.memctl0_dqs_addr,
			(char *)memctl0_dqs,
			memctl_dqs_length);
	memcpy((char *)args.memctl1_dqs_addr,
			(char *)memctl1_dqs,
			memctl_dqs_length);
	memcpy((char *)args_addr, &args, args_len);

	/* Lock recovery ram. */
	writel(0x01, DDR_PWR_REVLOCK);

	return tl420_boot_end();
}

static void tl420_regs_init(void)
{
	unsigned int i;
	unsigned int val;
	unsigned int mask = (1 << AP_WDT0_RESET_INTR)
		| (1 << AP_WDT1_RESET_INTR)
		| (1 << AP_WDT2_RESET_INTR)
		| (1 << AP_WDT3_RESET_INTR)
		| (1 << AP_WDT4_RESET_INTR)
		| (1 << CP_WDT_RESET_INTR);

	/* Set top bus clock (TL420) to 330M */
	writel(0xF000F00,DDR_PWR_BUSCLKDIV);

	/* Enable DDR_PWR Low power.
	 * Set these bits one by one. */
	val = readl(DDR_PWR_TOPBUSLPCTL);
	val |= (1 << MIDWAY_BACK_ACK_MK);
	writel(val, DDR_PWR_TOPBUSLPCTL);

	val = readl(DDR_PWR_TOPBUSLPCTL);
	val |= (1 << TL420RAM_TBAXI_LP_EN);
	writel(val, DDR_PWR_TOPBUSLPCTL);

	/* Enable TL420 interrupts. */
	val = mask;
	writel(val, DDR_PWR_INTR_EN_TL420);

	val = ~mask;
	writel(val, DDR_PWR_INTR_MASK_TL420);

	/* Enable TL420 sleep. */
	val = readl(DDR_PWR_TOPFSMCTL);
	val |= (1 << TL420_SLEEP_EN);
	writel(val, DDR_PWR_TOPFSMCTL);

	/* Enable TL420 module clk disable. */
	val = readl(DDR_PWR_TL420LPCTL);
	val |= (1 << TL420_CLK_LP_EN);
	writel(val, DDR_PWR_TL420LPCTL);

	/* Config TL420 ICTL vertors. */
	for (i = 0; i < TL420_ICTL_IRQ_NUM; i++)
		writel(TL420_CODE_RAM, TL420_ICTL_IRQ_VECTOR_N(i));

	#if 1
	/* Only for tl420 debug. */
	/* Disable cp gpio intr. */
	{
		unsigned int addr;
		for (addr = 0x220; addr <= 0x25c; addr += 4) {
			writel(0xffffffff, GPIO_BASE + addr);
		}
	}
	#endif

	/* Config power down counter. */
	writel(0x60, DDR_PWR_TL420_PD_CNT1);
	writel(0x30, DDR_PWR_TL420_PD_CNT2);
	writel(0x30, DDR_PWR_TL420_PD_CNT3);

	/* Config power down control. */
	val = (1 << (TL420_PD_MK + 16));
	val |= (1 << TL420_WK_ACK_MK) | (1 << (TL420_WK_ACK_MK + 16));
	writel(val, DDR_PWR_TL420_PD_CTL);
}

int tl420_init(void)
{
	tl420_regs_init();

	return tl420_boot();
}

