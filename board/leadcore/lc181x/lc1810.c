
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

DECLARE_GLOBAL_DATA_PTR;

extern int m0_boot(void);
extern int comip_lp_regs_init(void);

static void watchdog_init(void)
{
#if CONFIG_WATCHDOG_ENABLE
	/* reset time: 40ms. */
	__raw_writel(0x0528, (AP_PWR_CHIPRSTN_CTL));

	/* watchdog timeout: 10s . */
	/* watchdog clock rate is 52Mhz. 0x0d -- 0x1fffffff clocks. */
	__raw_writel(0x0d, (WDT_BASE + WDT_TORR));
	__raw_writel(0x03, (WDT_BASE + WDT_CR));
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

static void pllx_adjust(void);
static void ddr_pll_adjust(void); //zhxl add 20120426

static void gd_init(void)
{
	struct pmic_info* info = (struct pmic_info*)(&gd->pmic_info);

	gd->ram_size = PHYS_SDRAM_1_SIZE;
	gd->cpu_freq = CONFIG_A9_CLK;
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
}

int dram_init (void)
{
	int ret;

	gd_init();

	ret = pmic_init();
	if (ret)
		printf("pmic init failed!\n");

	mdelay(1);

	pllx_adjust();

	ddr_pll_adjust();//zhxl add 20120426

	#define A9_CLK_REG		0xA010A844
	__raw_writel(0x10008, A9_CLK_REG);

#if (CONFIG_DDR_BUS_CLK==312000000)
	debug("change ddr clk to 312MHz\n");	

	//modify com_uart clock.
	__raw_writel(((0x90<<16) | 0x1fbd), DDR_PWR_COMUARTCLKCTL);
#elif(CONFIG_DDR_BUS_CLK==351000000)
	//modify com_uart clock.
	__raw_writel(((0x180<<16) | 0x5f37), DDR_PWR_COMUARTCLKCTL);
#endif

	debug("memctl_init called\n");

        ret = memctl_init();
        if (ret)
                printf("memctl init failed!\n");

	pmic_power_on_key_check();

	return 0;
}

void dram_init_banksize (void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
}

int board_init(void)
{
	gd->bd->bi_arch_number = MACH_TYPE_LC181X;	
	gd->bd->bi_boot_params = CONFIG_BOOT_PARAMS_LOADADDR;

	m0_boot();

	watchdog_init();

	comip_lp_regs_init();
	
	icache_enable();
	//dcache_enable();
	flash_init();

	pmic_power_on_key_check();
	boot_image();
	pmic_power_on_key_check();

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

/* A9 frequency configuration. */
#if (CONFIG_A9_CLK == 988000000)
#define AP_PWR_PLL0CFG_VAL			(0x14b01)
#elif (CONFIG_A9_CLK == 1196000000)
#define AP_PWR_PLL0CFG_VAL			(0x15b01)
#else
#error "CONFIG_A9_CLK is invalid"
#endif

/* DDR bus clock configuration. */
#if(CONFIG_DDR_BUS_CLK == 312000000)
#define DDR_PWR_PLL0CFG_VAL			(0x11700)
#elif(CONFIG_DDR_BUS_CLK == 351000000)
#define DDR_PWR_PLL0CFG_VAL			(0x11a00)
#elif(CONFIG_DDR_BUS_CLK == 390000000)
#define DDR_PWR_PLL0CFG_VAL			(0x11d00)
#else
#error "CONFIG_DDR_BUS_CLK is invalid"
#endif

static inline int pll0_ready(void)
{
	return 	AP_PWR_PLL0CFG_VAL == __raw_readl(AP_PWR_PLL0CFG_CTL);
}

//zhxl 20120426 begin
static inline int ddr_pll_ready(void)
{
	return 	DDR_PWR_PLL0CFG_VAL == __raw_readl(DDR_PWR_PLLCTL);
}
//zhxl 20120426 end

static inline void pll0_set(void)
{
	__raw_writel(AP_PWR_PLL0CFG_VAL, AP_PWR_PLL0CFG_CTL);
}

//zhxl 20120426 begin
static inline void ddr_pll_set(void)
{
	__raw_writel(DDR_PWR_PLL0CFG_VAL, DDR_PWR_PLLCTL);
}
//zhxl 20120426 end

static inline void tm_stop(void)
{
	u32 val;

  //__raw_writel(0x00801fbd, DDR_PWR_COMUARTCLKCTL); // 260M  zhxl 20120503 adjust uart clk

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
	val = __raw_readl(AP_PWR_INTEN_A9);
	val &= ~(1 << AP_PWR_AP_PWR_TM_INTR);
	__raw_writel(val, AP_PWR_INTEN_A9);

	/*clear TM INT*/
	val = __raw_readl(AP_PWR_INT_RAW);
	if (val & (1 << AP_PWR_AP_PWR_TM_INTR)) {
		val = __raw_readl(AP_PWR_INTST_A9);
		val |= 1 << AP_PWR_AP_PWR_TM_INTR;
		__raw_writel(val, AP_PWR_INTST_A9);
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
	val = __raw_readl(AP_PWR_INTEN_A9);
	val |= 1 << AP_PWR_AP_PWR_TM_INTR;
	__raw_writel(val, AP_PWR_INTEN_A9);

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

static void pllx_adjust(void)
{
	int status;
	u32 flag;

	status = !pll0_ready();
	if (status)
		pll0_set();
	flag = status;

	if (flag) {
		local_irq_save(flag);
		ap_pwr_tm_start();
		ap_go_to_sleep();
		ap_pwr_tm_end();
		local_irq_restore(flag);
	}
}

//zhxl 20120426 begin
static void ddr_pll_adjust(void)
{
	int status;
	u32 flag;

	status = !ddr_pll_ready();
	if (status)
		ddr_pll_set();
	flag = status;

	if (flag) {
		local_irq_save(flag);
		ap_pwr_tm_start();
		ap_go_to_sleep();
		ap_pwr_tm_end();
		local_irq_restore(flag);
	}
}
//zhxl 20120426 end
/*
	  BootROM	  Security
	0xFFFF_FFFC	0xA400_2080[31:30]
1810:	0x0		0x0
ECO1:	0x2421_1000	0x0
ECO2:	0x2421_1000	0x1
ECO3:	0x2421_2000	0x0
*/
int lc1810_eco_ver_get(void)
{
	int eco_num = -1;
	int i = 10;
	u32 val = 0;
	u32 sig1 = 0;
	u32 sig2 = 0;
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

	sig1 = __raw_readl(0xfffffffc);
	sig2 = __raw_readl(0xa4002080);

	sig2 = (sig2 >> 30) & 0x03;

	if (0x24211000 == sig1) {
		if(0x00 == sig2)
			eco_num = 1;
		else
			eco_num = 2;
	} else if (0x24212000 == sig1) {
		if (0x00 == sig2)
			eco_num = 3;
	} else if (!sig1 && !sig2)
			eco_num = 0;

	if (eco_num < 0) {
		printf("eco number is not corrct\n");
		eco_num = 0;
	} else
		printf("eco number: %d\n", eco_num);

	return eco_num;
}

