
#include <common.h>
#include <asm/io.h>
#include <asm/armv7.h>

/*Security efulse.*/
#define SEC_INTE	(SECURITY_BASE + 0x01C)
#define SEC_EFUSE_CTRL	(SECURITY_BASE + 0x4)
#define SEC_INTS	(SECURITY_BASE + 0x020)
#define EFUSE_LOAD_INTE_BIT	0
#define EFUSE_LOAD_INTS_BIT	0
#define EFUSE_LOAD_EN_BIT	0
#define SPECIAL_ECC_CORR_MASK_BIT	2

#define BUCK2_VOUT_INC1_VALUE		25	/* mV */
#define BUCK2_VOUT_LEVEL_3_LC1813S	1300	/* mV */

DECLARE_GLOBAL_DATA_PTR;

int clock_init(void);

#ifndef CONFIG_COMIP_TARGETLOADER
extern int m0_boot(void);
extern int comip_lp_regs_init(void);

static void watchdog_init(void)
{
#if CONFIG_WATCHDOG_ENABLE
	int wdt_num = 4;
	int i;

	/* reset time: 60ms. */
	__raw_writel(0x0728, (AP_PWR_CHIPRSTN_CTL));

	/* watchdog timeout: 40s . */
	/* watchdog clock rate is 52Mhz. 0x0f -- 0x7fffffff clocks, max timeout. */
	for (i = 0; i < wdt_num; i++) {
		__raw_writel(0x0f, (WDT_BASE + WDT_TORR + i * 0x400));
#if CONFIG_WATCHDOG_DEBUG
		__raw_writel(0x03, (WDT_BASE + WDT_CR + i * 0x400));
#else
		__raw_writel(0x01, (WDT_BASE + WDT_CR + i * 0x400));
#endif
	}
#endif
}

static void boot_image(void)
{
	char *kernel_name = CONFIG_PARTITION_KERNEL;
	char *ramdisk_name = CONFIG_PARTITION_RAMDISK;
	int pu_reason;
	int key_code;
	int ret;

	pu_reason = pmic_power_up_reason_get();
	if ((pu_reason == PU_REASON_REBOOT_RECOVERY)
			|| (pu_reason == PU_REASON_REBOOT_FOTA)
			|| check_recovery_misc() || check_recovery_fota()) {
		gd->boot_mode = BOOT_MODE_RECOVERY;
		ramdisk_name = CONFIG_PARTITION_RAMDISK_RECOVERY;
#if defined(CONFIG_USE_KERNEL_RECOVERY)
		kernel_name = CONFIG_PARTITION_KERNEL_RECOVERY;
#endif
	} else {
		ret = keypad_init();
		if (ret)
			printf("keypad init failed!\n");

		key_code = keypad_check();

		switch (key_code) {
		case KEY_CODE_RECOVERY:
			gd->boot_mode = BOOT_MODE_RECOVERY;
			ramdisk_name = CONFIG_PARTITION_RAMDISK_RECOVERY;
#if defined(CONFIG_USE_KERNEL_RECOVERY)
			kernel_name = CONFIG_PARTITION_KERNEL_RECOVERY;
#endif
			break;
		case KEY_CODE_AMT1:
			gd->boot_mode = BOOT_MODE_AMT1;
			ramdisk_name = CONFIG_PARTITION_RAMDISK_AMT1;
			break;
#if defined(CONFIG_USE_RAMDISK_AMT3)
		case KEY_CODE_AMT3:
			gd->boot_mode = BOOT_MODE_AMT3;
			ramdisk_name = CONFIG_PARTITION_RAMDISK_AMT3;
			break;
#endif
		default:
			gd->boot_mode = BOOT_MODE_NORMAL;
			ramdisk_name = CONFIG_PARTITION_RAMDISK;
			break;
		}

#if defined(CONFIG_COMIP_FASTBOOT)
		if ((key_code == CONFIG_KEY_CODE_FASTBOOT) && (__raw_readl(AP_PWR_BOOTCTL) & 0x02))
			gd->fastboot = 1;
#endif

		printf("key code: %d\n", key_code);
	}

	printf("kernel name: %s, ramdisk name: %s\n", kernel_name, ramdisk_name);

	flash_partition_read(kernel_name, (u8*)CONFIG_KERNEL_LOADADDR, 0xffffffff);

	flash_partition_read(ramdisk_name, (u8*)CONFIG_RAMDISK_LOADADDR, 0xffffffff);

	flash_partition_read(CONFIG_PARTITION_LOGO, (u8*)CONFIG_FB_MEMORY_BASE, 0xffffffff);

	printf("boot image end\n");
}

static void lc1813_chip_id_get(void)
{
	int i = 10;
	u32 val = 0;
	u32 flag = 0;

	/*set sec_efluse read ecc off*/
	local_irq_save(flag);
	val = __raw_readl(SEC_INTE);
	val |= 1 << EFUSE_LOAD_INTE_BIT;
	__raw_writel(val, SEC_INTE);

	val = __raw_readl(SEC_EFUSE_CTRL);
	val |= ((1 << SPECIAL_ECC_CORR_MASK_BIT) | (1 << EFUSE_LOAD_EN_BIT));
	__raw_writel(val, SEC_EFUSE_CTRL);

	do{
		val = __raw_readl(SEC_INTS);
		if((val & 0x1)){
			printf("efuse id reload ok\n");
			break;
		}
	}while(i-- > 0);

	if(i <= 0)
		printf("efuse id reload error\n");

	/*Disable EFUSE_LOAD_INTE INT*/
	val = __raw_readl(SEC_INTE);
	val &= ~(1 << EFUSE_LOAD_INTE_BIT);
	__raw_writel(val, SEC_INTE);

	/*Clear EFUSE_LOAD_INTS*/
	val = __raw_readl(SEC_INTS);
	val |= (1 << EFUSE_LOAD_INTS_BIT);
	__raw_writel(val, SEC_INTS);
	local_irq_restore(flag);

	gd->rom_id = __raw_readl(0xfffffffc);
	gd->chip_id = __raw_readl(0xa4002080);
	if (gd->chip_id == 0) {
		if (gd->rom_id == 0)
			gd->bb_id = CHIP_ID_DEFAULT; /* default. */
		else
			gd->bb_id = gd->rom_id;
	} else
		gd->bb_id = gd->chip_id;

	printf("rom id: 0x%08lx, chip id: 0x%08lx\n", gd->rom_id, gd->chip_id);
}
#endif /* !CONFIG_COMIP_TARGETLOADER */

static void gd_init(void)
{
	struct pmic_info* info = (struct pmic_info*)(&gd->pmic_info);

#ifndef CONFIG_COMIP_TARGETLOADER
	lc1813_chip_id_get();
#endif

	gd->ram_size = PHYS_SDRAM_1_SIZE;
	gd->cpu_freq = CONFIG_A7_CLK;
	gd->ddr_freq = CONFIG_DDR_BUS_CLK;
	info->buck1_vout0 = BUCK1_VOUT_LEVEL_0;
	info->buck1_vout1 = BUCK1_VOUT_LEVEL_1;
	info->buck2_vout0 = BUCK2_VOUT_LEVEL_0;
	info->buck2_vout1 = BUCK2_VOUT_LEVEL_1;
	info->buck2_vout2 = BUCK2_VOUT_LEVEL_2;
	info->buck2_vout3 = BUCK2_VOUT_LEVEL_3;
#if defined(BUCK3_VOUT_LEVEL_0)
	info->buck3_vout0 = BUCK3_VOUT_LEVEL_0;
	info->buck3_vout1 = BUCK3_VOUT_LEVEL_1;
#endif
#if defined(BUCK4_VOUT_LEVEL_0)
	info->buck4_vout0 = BUCK4_VOUT_LEVEL_0;
	info->buck4_vout1 = BUCK4_VOUT_LEVEL_1;
#endif

	if (cpu_is_lc1813s())
		info->buck2_vout3 = BUCK2_VOUT_LEVEL_3_LC1813S;

	if (cpu_volt_flag() == VOLT_ID_INC1)
		info->buck2_vout3 += BUCK2_VOUT_INC1_VALUE;
}

int dram_init (void)
{
	int ret;

	gd_init();

	ret = pmic_init();
	if (ret)
		printf("pmic init failed!\n");

	mdelay(1);

	ret = clock_init();
	if (ret)
		printf("clock init failed!\n");

#if defined(CONFIG_TL_NODDR) || defined(CONFIG_MENTOR_DEBUG)
	/* Don't initialize ddr */
#else
	ret = memctl_init();
	if (ret)
		printf("memctl init failed!\n");
#endif

#ifndef CONFIG_COMIP_TARGETLOADER
	pmic_power_on_key_check();
#endif

	return 0;
}

void dram_init_banksize (void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	#if defined(PHYS_SDRAM_2) && defined(PHYS_SDRAM_2_SIZE)
	gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
	gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
	#endif
}

int board_init(void)
{
	gd->bd->bi_arch_number = MACH_TYPE_LC181X;
	gd->bd->bi_boot_params = CONFIG_BOOT_PARAMS_LOADADDR;

#ifndef CONFIG_COMIP_TARGETLOADER
	if(cpu_is_lc1813s())
		m0_boot();

	watchdog_init();

	comip_lp_regs_init();
#endif
	
	icache_enable();
	//dcache_enable();
	flash_init();

#ifndef CONFIG_COMIP_TARGETLOADER
	pmic_power_on_key_check();
	boot_image();
	pmic_power_on_key_check();
#endif

	return 0;
}

int misc_init_r(void)
{
	return 0;
}

void reset_cpu(unsigned long ignored)
{
	pmic_reboot();
	while (1);
}

#if defined(CONFIG_COMIP_MMC)
int board_mmc_init(bd_t *bd)
{
	debug("board_mmc_init called\n");

	comip_mmc_init(1, 4, 0, 0);

	return 0;
}
#endif

/*******************************************************/

#define AP_PWR_TM_CYCLES	1000 /* >10ms*/

static inline int pll0_ready(unsigned int val)
{
	if (cpu_is_lc1813s())
		return val == __raw_readl(AP_PWR_PLL0CFG_CTL0);
	else
		return val == __raw_readl(AP_PWR_PLL0CFG_CTL);
}

static inline int ddr_pll_ready(unsigned int val)
{
	if (cpu_is_lc1813s())
		return val == __raw_readl(DDR_PWR_PLLCTL0);
	else
		return val == __raw_readl(DDR_PWR_PLLCTL);
}

static inline void pll0_set(unsigned int val)
{
	if (cpu_is_lc1813s())
		__raw_writel(val, AP_PWR_PLL0CFG_CTL0);
	else
		__raw_writel(val, AP_PWR_PLL0CFG_CTL);
}

static inline void ddr_pll_set(unsigned int val)
{
	if (cpu_is_lc1813s())
		__raw_writel(val, DDR_PWR_PLLCTL0);
	else
		__raw_writel(val, DDR_PWR_PLLCTL);
}

static inline void tm_stop(void)
{
	u32 val;

	val = __raw_readl(AP_PWR_TM_CTL);
	if (val & (0x01 << AP_PWR_TM_ST)) {
		val = (0x00 << AP_PWR_TM_EN) |
			(1 << AP_PWR_TM_EN_WE);
		__raw_writel(val, AP_PWR_TM_CTL);
	}
}

static void ap_pwr_tm_end(void)
{
	u32 val;

	tm_stop();

	/*disable TM INT*/
	val = __raw_readl(AP_PWR_INTEN_A7);
	val &= ~(1 << AP_PWR_AP_PWR_TM_INTR);
	__raw_writel(val, AP_PWR_INTEN_A7);

	/*clear TM INT*/
	val = __raw_readl(AP_PWR_INT_RAW);
	if (val & (1 << AP_PWR_AP_PWR_TM_INTR)) {
		val = __raw_readl(AP_PWR_INTST_A7);
		val |= 1 << AP_PWR_AP_PWR_TM_INTR;
		__raw_writel(val, AP_PWR_INTST_A7);
	} else {
		/*WARNING: other INT wakeup AP?*/
	}
}

static void ap_pwr_tm_start(void)
{
	u32 val;

	/*if tm is running, stop it*/
	tm_stop();

	/*enable TM INT*/
	val = __raw_readl(AP_PWR_INTEN_A7);
	val |= 1 << AP_PWR_AP_PWR_TM_INTR;
	__raw_writel(val, AP_PWR_INTEN_A7);

	/*~ 1ms*/
	__raw_writel(AP_PWR_TM_CYCLES, AP_PWR_TM_INIT_VAL);

	/*start TM*/
	val = (0x00 << AP_PWR_TM_WK_MODE) |
			(1 << AP_PWR_TM_WK_MODE_WE);
	val |= (0x01 << AP_PWR_TM_LD_MODE) |
			(1 << AP_PWR_TM_LD_MODE_WE);
	val |= (0x01 << AP_PWR_TM_EN) |
			(1 << AP_PWR_TM_EN_WE);	
	__raw_writel(val, AP_PWR_TM_CTL);
}

static void ap_go_to_sleep(void)
{
	CP15DSB;
	__raw_writel(0x01 << AP_PWR_SLPST_SLP_ST, AP_PWR_SLPST);
	CP15ISB;
	while(!(__raw_readl(AP_PWR_INT_RAW)&(1 << AP_PWR_AP_PWR_TM_INTR)));
}

static unsigned int pll0_val_find(unsigned int freq)
{
	unsigned int a7_clk_ctl_val;
	unsigned int val;

	if (cpu_is_lc1813s()) {
		switch (freq) {
		case 455000000:  /* pll0: 910MHz. a7: 455MHz */
			val = 0x11204601;
			a7_clk_ctl_val = 0x10008;
		case 494000000:  /* pll0: 988MHz. a7: 494MHz */
			val = 0x11204c01;
			a7_clk_ctl_val = 0x10008;
			break;
		case 910000000:  /* pll0: 910MHz. a7: 910MHz */
			val = 0x11204601;
			a7_clk_ctl_val = 0x10010;
		case 988000000:  /* pll0: 988MHz. a7: 988MHz */
			val = 0x11204c01;
			a7_clk_ctl_val = 0x10010;
			break;
		case 1092000000:  /* pll0: 1092MHz. a7: 1092MHz */
			val = 0x11205401;
			a7_clk_ctl_val = 0x10010;
			break;
		case 1105000000:  /* pll0: 1105MHz. a7: 1105MHz */
			val = 0x11205501;
			a7_clk_ctl_val = 0x10010;
			break;
		case 1196000000: /* pll0: 1196MHz. a7: 1196MHz */
			val = 0x11205c01;
			a7_clk_ctl_val = 0x10010;
			break;
		default:
			printf("can't support the a7 clock: %d\n", freq);
			val = 0x11204601;
			a7_clk_ctl_val = 0x10010;
			break;
		}
	} else {
		switch (freq) {
		case 494000000:  /* pll0: 988MHz. a7: 494MHz */
			val = 0x14b01;
			a7_clk_ctl_val = 0x10008;
			break;
		case 988000000:  /* pll0: 988MHz. a7: 988MHz */
			val = 0x14b01;
			a7_clk_ctl_val = 0x10010;
			break;
		case 1196000000: /* pll0: 1196MHz. a7: 1196MHz */
			val = 0x15b01;
			a7_clk_ctl_val = 0x10010;
			break;
		case 1300000000: /* pll0: 1300MHz. a7: 1300MHz */
			val = 0x3101;
			a7_clk_ctl_val = 0x10010;
			break;
		case 1404000000: /* pll0: 1404MHz. a7: 1404MHz */
			val = 0x3501;
			a7_clk_ctl_val = 0x10010;
			break;
		case 1508000000: /* pll0: 1508MHz. a7: 1508MHz */
			val = 0x3901;
			a7_clk_ctl_val = 0x10010;
			break;
		default:
			printf("can't support the a7 clock: %d\n", freq);
			val = 0x14b01;
			a7_clk_ctl_val = 0x10010;
			break;
		}
	}

	__raw_writel(a7_clk_ctl_val, AP_PWR_A7_CLK_CTL);

	/* Set A7 AXI main clock to 416MHz. */
	__raw_writel(0x30008, AP_PWR_A7AXI_MAINCLK_CTL);

	/* Set EMA. */
	if (!cpu_is_lc1813s())
		__raw_writel(__raw_readl(CTL_RAM_EMA_CTRL) & (~(3 << 4)), CTL_RAM_EMA_CTRL);

	return val;
}

static void pllx_adjust(void)
{
	unsigned int val = pll0_val_find(gd->cpu_freq);
	int status;
	u32 flag;

	status = !pll0_ready(val);
	if (status)
		pll0_set(val);
	flag = status;

	if (flag) {
		local_irq_save(flag);
		ap_pwr_tm_start();
		ap_go_to_sleep();
		ap_pwr_tm_end();
		local_irq_restore(flag);
	}
}

static unsigned int ddr_pll_val_find(unsigned int freq)
{
	unsigned int val;

	if (cpu_is_lc1813s()) {
		switch (freq) {
		case 195000000: /* ap_ddrclk_to_ddrpll: 780MHz. */
			val = 0x11203c01;
			break;
		case 312000000: /* ap_ddrclk_to_ddrpll: 624MHz. */
			val = 0x11203001;
			break;
		case 351000000: /* ap_ddrclk_to_ddrpll: 702MHz. */
			val = 0x11203601;
			break;
		case 390000000: /* ap_ddrclk_to_ddrpll: 780MHz. */
			val = 0x11203c01;
			break;
		default:
			printf("can't support the dll clock: %d\n", freq);
			val = 0x11203c01;
			break;
		}
	} else {
		switch (freq) {
		case 195000000: /* ap_ddrclk_to_ddrpll: 780MHz. */
			val = 0x11d00;
			break;
		case 312000000: /* ap_ddrclk_to_ddrpll: 624MHz. */
			val = 0x11700;
			break;
		case 351000000: /* ap_ddrclk_to_ddrpll: 702MHz. */
			val = 0x11a00;
			break;
		case 390000000: /* ap_ddrclk_to_ddrpll: 780MHz. */
			val = 0x11d00;
			break;
		default:
			printf("can't support the dll clock: %d\n", freq);
			val = 0x11d00;
			break;
		}
	}

	return val;
}

static void ddr_pll_adjust(void)
{
	unsigned int val = ddr_pll_val_find(gd->ddr_freq);
	int status;
	u32 flag;

	status = !ddr_pll_ready(val);
	if (status)
		ddr_pll_set(val);
	flag = status;

	if (flag) {
		local_irq_save(flag);
		ap_pwr_tm_start();
		ap_go_to_sleep();
		ap_pwr_tm_end();
		local_irq_restore(flag);
	}
}

static unsigned int uart_clock_val_find(unsigned int freq)
{
	unsigned int val;

	switch (freq) {
	case 195000000: /* ap_ddrclk_to_ddrpll: 780MHz. */
		val = 0x02409eb1;
		break;
	case 312000000: /* ap_ddrclk_to_ddrpll: 624MHz. */
		val = 0x00901fbd;
		break;
	case 351000000: /* ap_ddrclk_to_ddrpll: 702MHz. */
		val = 0x01805f37;
		break;
	case 390000000: /* ap_ddrclk_to_ddrpll: 780MHz. */
		val = 0x02409eb1;
		break;
	default:
		printf("can't support the dll clock: %d\n", freq);
		val = 0x02409eb1;
		break;
	}

	return val;
}

static void uart_clock_adjust(void)
{
	unsigned int val = uart_clock_val_find(gd->ddr_freq);

	__raw_writel(val, DDR_PWR_COMUARTCLKCTL);
}

int clock_init(void)
{
	pllx_adjust();
	ddr_pll_adjust();
	uart_clock_adjust();

	return 0;
}

