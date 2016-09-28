#include <common.h>
#include <asm/io.h>
#include <asm/armv7.h>
#include <mmc.h>

DECLARE_GLOBAL_DATA_PTR;

#define ROM_ID_ADDR	(0xfffffffc)
#define CHIP_ID_ADDR	(EFUSE_DATA_BASE + 0x04)
#define CHIP_SN_ADDR	EFUSE_DATA_BASE

#if defined (CONFIG_ENABLE_SECURE_VERIFY_FOR_BOOT)
#define IMAGE_ADDR_OFFSET  512 * 3
#else
#define IMAGE_ADDR_OFFSET  0
#endif

int clock_init(void);
#if defined(COMIP_LOW_POWER_MODE_ENABLE)
extern int comip_lp_regs_init(void);
#endif

#if CONFIG_COMIP_EMMC_ENHANCE
extern int mmc_set_dma(int usedma);
#endif

static void lc1860_chip_id_get(void)
{
	u32 val = 0;
	u32 flag = 0;
	u32 timeout = 100;

	local_irq_save(flag);

	val = __raw_readl(EFUSE_MODE);
	val &= ~(1 << EFUSE_MODE_MODE);
	__raw_writel(val, EFUSE_MODE);

	__raw_writel(EFUSE_ADDR_CHIP_ID, EFUSE_ADDR);
	__raw_writel(EFUSE_LENGTH_CHIP_ID, EFUSE_LENGTH);

	val = __raw_readl(SECURITY_INTR_EN);
	val |= (1 << SECURITY_INTR_EFUSE);
	__raw_writel(val, SECURITY_INTR_EN);

	val = __raw_readl(EFUSE_CTRL);
	val |= (1 << EFUSE_CTRL_ENABLE);
	__raw_writel(val, EFUSE_CTRL);

	do {
		val = __raw_readl(SECURITY_INTR_STATUS);
		if (val & (1 << SECURITY_INTR_EFUSE)) {
			printf("load chip id from efuse ok\n");
			break;
		}
	} while(timeout-- > 0);

	if(timeout <= 0)
		printf("load chip id from efuse failed\n");

	gd->chip_id = __raw_readl(CHIP_ID_ADDR) & 0x00ffffff;

	val = __raw_readl(SECURITY_INTR_EN);
	val &= ~(1 << SECURITY_INTR_EFUSE);
	__raw_writel(val, SECURITY_INTR_EN);

	val = __raw_readl(SECURITY_INTR_STATUS);
	val |= (1 << SECURITY_INTR_EFUSE);
	__raw_writel(val, SECURITY_INTR_STATUS);

	local_irq_restore(flag);

	gd->rom_id = __raw_readl(ROM_ID_ADDR);

	if (gd->chip_id)
		gd->bb_id = gd->chip_id;
	else
		gd->bb_id = (CHIP_ID_DEFAULT | (gd->rom_id << REV_ID_BIT));

	gd->sn_id = __raw_readl(CHIP_SN_ADDR);

	printf("chip id: 0x%lx, rom id: 0x%lx bb_id: 0x%lx sn_id: 0x%lx\n", gd->chip_id, gd->rom_id,gd->bb_id, gd->sn_id);
}

#ifndef CONFIG_COMIP_TARGETLOADER
extern int tl420_init(void);

static inline void comip_lc186x_bus_prior_config(void)
{
	/* Set isp & display bus priority. */
	__raw_writel(0x11113000, TOP_MAIL_MASTER_PRIOR2);

	//__raw_writel(0x11110000, TOP_MAIL_MASTER_PRIOR3);

	__raw_writel(0x01110223, TOP_MAIL_MASTER_PRIOR4);

	__raw_writel(0x01110121, TOP_MAIL_MASTER_PRIOR5);

	__raw_writel(0x11113321, TOP_MAIL_MASTER_PRIOR6);

	__raw_writel((3 << 5), AP_SW0_GPV_AD_QOS_CNTL_AP_SW1);

	__raw_writel(((8 << 24) | (5 << 8)), AP_SW0_GPV_MAX_OT_AP_SW1);

	__raw_writel((3 << 5), AP_SW0_GPV_AD_QOS_CNTL_AP_SW2_DDR);

	__raw_writel(((4 << 24) | (4 << 8)), AP_SW0_GPV_MAX_OT_AP_SW2_DDR);

	__raw_writel((3 << 5), AP_SW0_GPV_AD_QOS_CNTL_AP_SW3);

	__raw_writel(((6 << 24) | (4 << 8)), AP_SW0_GPV_MAX_OT_AP_SW3);

	__raw_writel((3 << 5), AP_SW0_GPV_AD_QOS_CNTL_CCI_DDR);

	__raw_writel(((6 << 24) | (5 << 8)), AP_SW0_GPV_MAX_OT_CCI_DDR);
}


static inline void comip_lc186x_sysclk_config(void)
{
	unsigned int val;

	/* Set bus_mclk0 416MHZ,
	* so bus_mclk1 default is 208M,
	* ctl_pclk , data_pclk, sec_pclk default is 104MHZ*/
	val = 0x2 | (0x7 << 16);
	__raw_writel(val, AP_PWR_BUSMCLK0_CTL);
}

static inline  void comip_lc186x_sec_config(void)
{
	/*Set com-i2c\ddr-pwr
	* gpio\com-i2s\com-pcm
	* com-uart\MUX-PIN non-secure
	* to allow dmas access
	*/
	__raw_writel(0x7f, TOP_CTRL_GPV_SEC_CTL_TOP_COM_APB);
}

static inline void comip_lc186x_coresight_config(void)
{
	/*Allow coresight access when cp A7 power down or sleep
	*/
	__raw_writel(0xff, DDR_PWR_CDBGPWRUPREQMASK);
}

static void watchdog_init(void)
{

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
		printf("key code: %d\n", key_code);

		 if(pu_reason == PU_REASON_USB_CHARGER
			#if defined(CONFIG_COMIP_FASTBOOT)
				&& key_code != CONFIG_KEY_CODE_FASTBOOT
			#endif
		) {
			gd->boot_mode = BOOT_MODE_NORMAL;
			ramdisk_name = CONFIG_PARTITION_RAMDISK_AMT1;
		} else {
			switch (key_code) {
			case KEY_CODE_RECOVERY:
				gd->boot_mode = BOOT_MODE_RECOVERY;
				ramdisk_name = CONFIG_PARTITION_RAMDISK_RECOVERY;
				#if defined(CONFIG_USE_KERNEL_RECOVERY)
				kernel_name = CONFIG_PARTITION_KERNEL_RECOVERY;
				#endif
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
			if (key_code == CONFIG_KEY_CODE_FASTBOOT) {
				printf("goto fastmode!\n");
				gd->fastboot = 1;
			}
			#endif
		}
	}

	printf("kernel name: %s, ramdisk name: %s\n", kernel_name, ramdisk_name);

    flash_partition_read(kernel_name, (u8*)(CONFIG_KERNEL_LOADADDR - IMAGE_ADDR_OFFSET), 0xffffffff);

	flash_partition_read(ramdisk_name, (u8*)(CONFIG_RAMDISK_LOADADDR - IMAGE_ADDR_OFFSET), 0xffffffff);

#if defined(CONFIG_COMIP_FASTBOOT) && defined(CONFIG_LCD_SUPPORT)
	if (unlikely(gd->fastboot))
		flash_partition_read(CONFIG_PARTITION_FASTBOOT_LOGO, (u8*)(unsigned int)gd->fb_base, CONFIG_FB_MEMORY_SIZE);
	else
		flash_partition_read(CONFIG_PARTITION_LOGO, (u8*)(unsigned int)gd->fb_base, CONFIG_FB_MEMORY_SIZE);
#else
	flash_partition_read(CONFIG_PARTITION_LOGO, (u8*)(unsigned int)gd->fb_base, CONFIG_FB_MEMORY_SIZE);
#endif

	printf("boot image end\n");
}
#endif /* !CONFIG_COMIP_TARGETLOADER */

static void gd_init(void)
{
	struct pmic_info* info = (struct pmic_info*)(&gd->pmic_info);
#ifndef BUCK2_VOUT_LEVEL_3
	u8 buck2_vout_index = 0;
	u32 buck2_vout_level3_table[] = {	\
		11000, /* level 0: 1.1V */	\
		11375, /* level 1: 1.1V */	\
		11375, /* level 2: 1.1V */	\
		11375, /* level 3: 1.1V */	\
		11375, /* level 4: 1.1V */	\
		11375, /* level 5: 1.1V */	\
		11500, /* level 6: 1.1V */	\
		11500, /* level 7: 1.1V */
	};
#endif

#ifndef BUCK4_VOUT_LEVEL_1
	u8 buck4_vout_index = 0;
	u32 buck4_vout_level1_table[] = {	\
		12250, /* level 0: 1.225V */	\
		12250, /* level 1: 1.225V */	\
		12250, /* level 2: 1.225V */	\
		12250, /* level 3: 1.225V */	\
		12250, /* level 4: 1.225V */	\
		12250, /* level 5: 1.225V */	\
		13000, /* level 6: 1.3V */	\
		13000, /* level 7: 1.3V */	\
	};
#endif

	lc1860_chip_id_get();

	gd->cpu_freq = CONFIG_A7_CLK;
	gd->ddr_freq = CONFIG_DDR_BUS_CLK;
#if defined(BUCK1_VOUT_LEVEL_0)
	info->buck1_vout0 = BUCK1_VOUT_LEVEL_0;
	info->buck1_vout1 = BUCK1_VOUT_LEVEL_1;
#endif
#if defined(BUCK2_VOUT_LEVEL_0)
	info->buck2_vout0 = BUCK2_VOUT_LEVEL_0;
	info->buck2_vout1 = BUCK2_VOUT_LEVEL_1;
	info->buck2_vout2 = BUCK2_VOUT_LEVEL_2;
#if defined(BUCK2_VOUT_LEVEL_3)
	info->buck2_vout3 = BUCK2_VOUT_LEVEL_3;
#else
	buck2_vout_index = cpu_volt_flag();
	info->buck2_vout3 = buck2_vout_level3_table[buck2_vout_index];
#endif
#endif
#if defined(BUCK3_VOUT_LEVEL_0)
	info->buck3_vout0 = BUCK3_VOUT_LEVEL_0;
	info->buck3_vout1 = BUCK3_VOUT_LEVEL_1;
	info->buck3_vout2 = BUCK3_VOUT_LEVEL_2;
	info->buck3_vout3 = BUCK3_VOUT_LEVEL_3;
#endif
#if defined(BUCK4_VOUT_LEVEL_0)
	info->buck4_vout0 = BUCK4_VOUT_LEVEL_0;
#if defined(BUCK4_VOUT_LEVEL_1)
	info->buck4_vout1 = BUCK4_VOUT_LEVEL_1;
#else
	buck4_vout_index = cpu_volt_flag();
	info->buck4_vout1 = buck4_vout_level1_table[buck4_vout_index];
#endif
#endif
#if defined(BUCK5_VOUT_LEVEL_0)
	info->buck5_vout0 = BUCK5_VOUT_LEVEL_0;
	info->buck5_vout1 = BUCK5_VOUT_LEVEL_1;
#endif
#if defined(BUCK6_VOUT_LEVEL_0)
	info->buck6_vout0 = BUCK6_VOUT_LEVEL_0;
	info->buck6_vout1 = BUCK6_VOUT_LEVEL_1;
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

	ret = clock_init();
	if (ret)
		printf("clock init failed!\n");

#if CONFIG_COMIP_EMMC_ENHANCE
	mmc_set_dma(0);
	flash_init();
#endif

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
	gd->bd->bi_dram[0].start = gd->dram_base;
	gd->bd->bi_dram[0].size = gd->dram_size;

	if (gd->dram2_size) {
		gd->bd->bi_dram[1].start = gd->dram2_base;
		gd->bd->bi_dram[1].size = gd->dram2_size;
	}
}

int board_init(void)
{
	gd->bd->bi_arch_number = MACH_TYPE_LC186X;
	gd->bd->bi_boot_params = CONFIG_BOOT_PARAMS_LOADADDR;

#ifndef CONFIG_COMIP_TARGETLOADER
	tl420_init();

	watchdog_init();

	comip_lc186x_coresight_config();

	comip_lc186x_sysclk_config();

	comip_lc186x_sec_config();

	comip_lc186x_bus_prior_config();
#endif

#if defined(COMIP_LOW_POWER_MODE_ENABLE)
	comip_lp_regs_init();
#endif
	icache_enable();
	//dcache_enable();
#if CONFIG_COMIP_EMMC_ENHANCE
	mmc_set_dma(1);
#endif
	flash_init();

#ifndef CONFIG_COMIP_TARGETLOADER
	pmic_power_on_key_check();
	boot_image();
	pmic_power_on_key_check();
#endif

#ifdef CONFIG_PMIC_VIBRATOR
	pmic_vibrator_enable_set();
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

	if (EMMC_SAMSUNG == get_emmc_vendor())
		comip_mmc_init(1, 4, 6, 5);
	else
		comip_mmc_init(1, 4, 0, 0);

	return 0;
}
#endif

#if !defined(PLL_DDR_ADJUST_BY_INT)
#define AP_PWR_TM_CYCLES	1000 /* >10ms*/

static inline void tm_stop(void)
{
	u32 val;

	val = __raw_readl(AP_PWR_TM32K_CTL);
	if (val & (0x01 << AP_PWR_TM_ST)) {
		val = (0x00 << AP_PWR_TM_EN) |
			(1 << AP_PWR_TM_EN_WE);
		__raw_writel(val, AP_PWR_TM32K_CTL);
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
	__raw_writel(AP_PWR_TM_CYCLES, AP_PWR_TM32K_INIT_VAL);

	/*start TM*/
	val = (0x00 << AP_PWR_TM_WK_MODE) |
			(1 << AP_PWR_TM_WK_MODE_WE);
	val |= (0x01 << AP_PWR_TM_LD_MODE) |
			(1 << AP_PWR_TM_LD_MODE_WE);
	val |= (0x01 << AP_PWR_TM_EN) |
			(1 << AP_PWR_TM_EN_WE);
	__raw_writel(val, AP_PWR_TM32K_CTL);
}

static void ap_go_to_sleep(void)
{
	CP15DSB;
	__raw_writel(0x01 << AP_PWR_SLPST_SLP_ST, AP_PWR_SLPST);
	CP15ISB;
	while(!(__raw_readl(AP_PWR_INT_RAW)&(1 << AP_PWR_AP_PWR_TM_INTR)));
}
#endif

static inline void pll0_set(unsigned int val)
{
	__raw_writel(val, AP_PWR_PLL0CFG_CTL0);
}

static inline void ddr_pll_set(unsigned int val)
{
#if	defined(PLL_DDR_ADJUST_BY_SLP)
	__raw_writel(val, DDR_PWR_PLLCFG);
#else
	int timeout = 100000;
	unsigned int status;

	__raw_writel(val, DDR_PWR_PLLCFG);
	__raw_writel(0x1, DDR_PWR_PLLCTL);

	do {
		status = readl(DDR_PWR_PLLCTL);
		if(status == 0)
			break;
	} while (--timeout);

	if (timeout <= 0)
		printf("ddr_pwr_pll adjust fail!\n");
#endif
}

static inline int pll0_ready(unsigned int val)
{
	return val == __raw_readl(AP_PWR_PLL0CFG_CTL0);
}

static inline int ddr_pll_ready(unsigned int val)
{
	return val == __raw_readl(DDR_PWR_PLLCFG);
}

static unsigned int pll0_val_find(unsigned int freq)
{
	unsigned int a7_clk_ctl_val;
	unsigned int val;

	switch (freq) {
	case 494000000:  /* pll0: 494MHz. ha7: 494MHz */
		val = 0x11102602;
		a7_clk_ctl_val = 0x1F0010;
		break;
	case 988000000:  /* pll0: 988MHz. ha7: 988MHz */
		val = 0x11104C02;
		a7_clk_ctl_val = 0x1F0010;
		break;
	case 1196000000: /* pll0: 1196MHz. ha7: 1196MHz */
		val = 0x11105C02;
		a7_clk_ctl_val = 0x1F0010;
		break;
	case 1404000000: /* pll0: 1404MHz. ha7: 1404MHz */
		val = 0x11106C02;
		a7_clk_ctl_val = 0x1F0010;
		break;
	case 1495000000: /* pll0: 1404MHz. ha7: 1404MHz */
		val = 0x11107302;
		a7_clk_ctl_val = 0x1F0010;
		break;
	case 1599000000: /* pll0: 1599MHz. ha7: 1599MHz */
		val = 0x11107B02;
		a7_clk_ctl_val = 0x1F0010;
		break;
	case 1807000000: /* pll0: 1807MHz. ha7: 1807MHz */
		val = 0x11108B02;
		a7_clk_ctl_val = 0x1F0010;
		break;
	default:
		printf("can't support the a7 clock: %d\n", freq);
		val = 0x11102602;
		a7_clk_ctl_val = 0x1F0010;
		break;
	}

	__raw_writel(a7_clk_ctl_val, AP_PWR_HA7_CLK_CTL);

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

static void uart_clock_set(unsigned int val)
{
	__raw_writel(val, DDR_PWR_COMUARTCLKDIV);
}

static void ddr_clock_set(unsigned int divl,
		unsigned int divh, unsigned int divn)
{
	/* Prepare for two freq divider. */
	writel(divl, DDR_PWR_DDRCLKDIVL);
	writel(divh, DDR_PWR_DDRCLKDIVH);
	writel(divn, DDR_PWR_DDRCLKDIV_NOW);
}

static unsigned int ddr_pll_val_find(unsigned int freq)
{
	unsigned int val;
	unsigned int ddr_divl_val;
	unsigned int ddr_divh_val;
	unsigned int ddr_divn_val;
	unsigned int uart_div_val;

	switch (freq) {
	case 165750000: /* ddr_pwr_pll: 1326MHz. */
		val = 0x11106602;
		ddr_divl_val = 7;
		ddr_divh_val = 7;
		ddr_divn_val = 7;
		uart_div_val = 0x0240A1DD;
		break;
	case 221000000: /* ddr_pwr_pll: 1326MHz. */
		val = 0x11106602;
		ddr_divl_val = 5;
		ddr_divh_val = 5;
		ddr_divn_val = 5;
		uart_div_val = 0x0240A1DD;
		break;
	case 331500000: /* ddr_pwr_pll: 1326MHz. */
		val = 0x11106602;
		ddr_divl_val = 7;
		ddr_divh_val = 3;
		ddr_divn_val = 3;
		uart_div_val = 0x0240A1DD;
		break;
	case 520000000: /* ddr_pwr_pll: 1040MHz. */
		val = 0x11105002;
		ddr_divl_val = 3;
		ddr_divh_val = 1;
		ddr_divn_val = 1;
		uart_div_val = 0x00801c36;
		break;
	case 533000000: /* ddr_pwr_pll: 1066MHz. */
		val = 0x11105202;
		ddr_divl_val = 3;
		ddr_divh_val = 1;
		ddr_divn_val = 1;
		uart_div_val = 0x01204110;
		break;
	case 663000000: /* ddr_pwr_pll: 1326MHz. */
		val = 0x11106602;
		ddr_divl_val = 3;
		ddr_divh_val = 1;
		ddr_divn_val = 1;
		uart_div_val = 0x0240A1DD;
		break;
	default:
		printf("can't support the dll clock: %d\n", freq);
		val = 0x11106602;
		ddr_divl_val = 1;
		ddr_divh_val = 1;
		ddr_divn_val = 1;
		uart_div_val = 0x0240A1DD;
		break;
	}

	uart_clock_set(uart_div_val);
	ddr_clock_set(ddr_divl_val, ddr_divh_val, ddr_divn_val);

	return val;
}

static void ddr_pll_adjust(void)
{
	unsigned int val = ddr_pll_val_find(gd->ddr_freq);
	int status;

	status = !ddr_pll_ready(val);
	if (status)
		ddr_pll_set(val);

#if defined(PLL_DDR_ADJUST_BY_SLP)
	if (status) {
		local_irq_save(status);
		ap_pwr_tm_start();
		ap_go_to_sleep();
		ap_pwr_tm_end();
		local_irq_restore(status);
	}
#endif
}

int clock_init(void)
{
#ifndef CONFIG_MENTOR_DEBUG
	pllx_adjust();
	ddr_pll_adjust();
#endif

	return 0;
}

#ifdef CONFIG_COMIP_LC1860_MENTOR
#define DDR_CFG_BASE_0		0xE1001000
#define DDR_CTL_BASE_0		DDR_CFG_BASE_0
#define DENALI_0_CTL_45	((volatile unsigned  int*)(DDR_CTL_BASE_0 + 0xB4))
#define DENALI_0_CTL_75	((volatile unsigned  int*)(DDR_CTL_BASE_0 + 0x12C))
#define DENALI_0_CTL_44	((volatile unsigned  int*)(DDR_CTL_BASE_0 + 0xB0))
#define DENALI_0_CTL_45_DATA	0x00080000
#define DENALI_0_CTL_44_DATA	0x02020200
#define DENALI_0_CTL_75_DATA	0x00029b1f
#define dsb()	CP15DSB

static void memctl_pd_pu_set(void)
{
	printf("memctl_pd_pu_set\n");
	//disable memctl1
	__raw_writel(0x0, (DDR_PWR_MEMCTL1EN));
	//mem pd data_retention
	__raw_writel(0x80000, (DDR_PWR_MEMCTL0_PD_CTL));
	//change memctl pu cnt
	__raw_writel(0x200200, (DDR_PWR_MEMCTL_PD_CNT2));
	__raw_writel(0x200, (DDR_PWR_MEMCTL_PD_CNT3));

	__raw_writel(DENALI_0_CTL_44_DATA, (DENALI_0_CTL_44));
	__raw_writel(DENALI_0_CTL_45_DATA, (DENALI_0_CTL_45));
	__raw_writel(DENALI_0_CTL_75_DATA, (DENALI_0_CTL_75));
	dsb();
}
#endif

static void cci_init(void)
{
#define CCI_BASE 		0xA0000000
#define CCI_SNOOP_CTL4_HA7 	((volatile unsigned  *)( CCI_BASE + 0x95000 ))
#define CCT_SNOOP_STATUS  	((volatile unsigned  *)( CCI_BASE + 0x9000c ))
	/* Enable HA7 SNOOP */
	unsigned int val;
	int timeout = 10000000;

	val = __raw_readl(CCI_SNOOP_CTL4_HA7);
	if((val & 0x3) == 0x3)
		return;
	val |= 0x3;
	__raw_writel(val, CCI_SNOOP_CTL4_HA7);

	do {
		val = __raw_readl(CCT_SNOOP_STATUS);
		if (!(val & 0x1))
			break;
	} while(--timeout);
	if(timeout <= 0)
		printf("Enable cci SNOOP ERROR!!\n");
}

int board_early_init_f (void)
{
	unsigned long cpuID = 0;
	unsigned long clusterID =0;
	unsigned long val=0;

	__asm__ __volatile__("mrc p15, 0, %0, c0, c0, 5"
	                     :"=r" (cpuID)
	                     :
	                     :"memory");

	clusterID = cpuID & 0x00000f00;
	clusterID >>= 8;
	cpuID = cpuID & 0x03;

	if (0x1 == clusterID) {
#ifdef CONFIG_COMIP_LC1860_MENTOR
		memctl_pd_pu_set();
#endif
		if(cpuID != 0)
			while(1);

		printf("HA(cpu%lu) runing.\n",cpuID);
		/* Set SA7 freq to 624MHZ default */
		__raw_writel(0x1 << 4 | 0x7 << 20, AP_PWR_SA7CLK_CTL0);
		cci_init();
	} else if (0x0 == clusterID) {
		printf("sa7 runing.\n");

		__raw_writel(CONFIG_HA_START_ADDR, CTL_AP_HA7_CORE0_WBOOT_ADDR);
		__raw_writel(CONFIG_HA_START_ADDR, CTL_AP_HA7_CORE1_WBOOT_ADDR);
		__raw_writel(CONFIG_HA_START_ADDR, CTL_AP_HA7_CORE2_WBOOT_ADDR);
		__raw_writel(CONFIG_HA_START_ADDR, CTL_AP_HA7_CORE3_WBOOT_ADDR);

		/* Disable HA7 cpu1/2/3 power up with SCU  */
		__raw_writel(0x1 << 22, AP_PWR_HA7_C1_PD_CTL);
		__raw_writel(0x1 << 22, AP_PWR_HA7_C2_PD_CTL);
		__raw_writel(0x1 << 22, AP_PWR_HA7_C3_PD_CTL);

		/* Power on HA7 core0 */
		__raw_writel(((0x1 << 1) | (0x1 << 17)), AP_PWR_HA7_C0_PD_CTL);
		/* Disable SA7 L2C access */
		val = __raw_readl(CTL_AP1CORE_CTRL);
		val |= 0x1;
		__raw_writel(val, CTL_AP1CORE_CTRL);
		/* Enable sa7 scu power down */
		__raw_writel(0x3 << 18, AP_PWR_SA7_SCU_PD_CTL);
		/* Disable SA7 power on with SCU & IRQ */
		__raw_writel((0x1 << 5 | 0x3 << 21), AP_PWR_SA7_C_PD_CTL);
		/* Enable SA7 auto power-down */
		__raw_writel(0x1 << 20, AP_PWR_SA7_C_PD_CTL);
		__asm__ __volatile__ ("wfi" : : : "memory");

	} else {
		printf("Unkowned cluster ID!\n");
		return 1;
	}

	return 0;
}

void ap_sleep_wakeup(void)
{
	int status;
	local_irq_save(status);
	ap_pwr_tm_start();
	ap_go_to_sleep();
	ap_pwr_tm_end();
	local_irq_restore(status);
}
