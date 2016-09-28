#include <common.h>
#include <asm/io.h>
#include <asm/armv7.h>

DECLARE_GLOBAL_DATA_PTR;

typedef struct {
	u32 reg;
	u32 val;
	u8  flag;
} reg_val; 

#define AP_PWR_TM_CYCLES	1000 /* > 10ms*/

#define AP_DMAG_CH_LP_EN	(AP_DMAG_BASE + 0x004c)
#define AP_DMAG_AHB_LP_EN	(AP_DMAG_BASE + 0x0050)

#define CIPHER_AES_LP_CTRL	(CIPHER_BASE + 0xa0)
#define CIPHER_SHA_LP_CTRL	(CIPHER_BASE + 0x800 + 0x84)
#define AP_DMAS_LP_CTL		(AP_DMAS_BASE + 0x3FC)
#define AUDIO_DMAS_LP_CTRL	(AUDIO_DMAS_BASE + 0x3FC)
#define NFC_LP_CTRL		(NFC_BASE + 0x70)
#define LCDC0_LP_CTRL		(LCDC0_BASE + 0x1c0)
#define LCDC1_LP_CTRL		(LCDC1_BASE + 0x1c0)
#ifdef CONFIG_CPU_LC1810
#define HPI_LP_CTRL		(HPI_BASE + 0x18)
#endif
#define SECURITY_SEC_LPC_CTRL	(SECURITY_BASE + 0x14)

/*PLL0: 1196M*/
#define AP_PWR_PLL0CFG_VAL	((0x00 << AP_PWR_PLL0CFG_CTL_PLL0OD_SEL) | \
				(0x2d << AP_PWR_PLL0CFG_CTL_PLL0F_SEL) | \
				(0x01 << AP_PWR_PLL0CFG_CTL_PLL0R_SEL))

/*PLL0: 988*/
#define AP_PWR_PLL0CFG_998M_VAL	((0x01 << AP_PWR_PLL0CFG_CTL_PLL0OD_SEL) | \
				(0x4b << AP_PWR_PLL0CFG_CTL_PLL0F_SEL) | \
				(0x01 << AP_PWR_PLL0CFG_CTL_PLL0R_SEL))

/*PLL1: 1248M*/
#define AP_PWR_PLL1CFG_VAL	 ((0x00 << AP_PWR_PLL1CFG_CTL_PLL1OD_SEL) | \
				(0x2f << AP_PWR_PLL1CFG_CTL_PLL1F_SEL) | \
				(0x01 << AP_PWR_PLL1CFG_CTL_PLL1R_SEL))	 
							
/*PLL2: 1020M*/
#define AP_PWR_PLL2CFG_VAL	 ((0x00 << AP_PWR_PLL2CFG_CTL_PLL2OD_SEL) | \
				(0x54 << AP_PWR_PLL2CFG_CTL_PLL2F_SEL) | \
				(0x01 << AP_PWR_PLL2CFG_CTL_PLL2R_SEL))	 
/*
 *if pll0_out == 1196M	
 *	a9_clk: 1196M
 *	acp_clk: 149.5M
 *
 *if pll0_out == 988M
 *	a9_clk: 988
 *	acp_clk: 123.5M	
 */							
#define AP_PWR_A9_CLK_VAL	(( 0x8 << AP_PWR_A9_CLK_CTL_A9_CLK_GR) | \
				(1 << AP_PWR_A9_CLK_CTL_A9_CLK_GR_WE) | \
				(0x0 << AP_PWR_A9_CLK_CTL_A9_PERI_CLK_GR) | \
				(1 << AP_PWR_A9_CLK_CTL_A9_PERI_CLK_GR_WE) | \
				(0x0 << AP_PWR_A9_CLK_CTL_A9_ACP_CLK_GR) | \
				(1 << AP_PWR_A9_CLK_CTL_A9_ACP_CLK_GR_WE))

/*bus_mclk: 104M*/							
#define AP_PWR_SYSCLK_VAL	((0x04 << AP_PWR_SYSCLK_CTL_BUS_MCLK_GR) | \
				(1 << AP_PWR_SYSCLK_CTL_BUS_MCLK_GR_WE) | \
				(0x03 << AP_PWR_SYSCLK_CTL_BUS_MCLK_DR) | \
				(1 << AP_PWR_SYSCLK_CTL_BUS_MCLK_DR_WE))

/*
 *ctl_pclk:  active:bus_mclk / 1 
 *ctl_pclk:  idle:   bus_mclk / 2 
 */							
#define AP_PWR_CTLPCLK_VAL	((0x03 << AP_PWR_CTLPCLK_CTL_CTL_PCLK_DIV_WK) | \
				(1 << AP_PWR_CTLPCLK_CTL_CTL_PCLK_DIV_WK_WE) | \
				(0x03 << AP_PWR_CTLPCLK_CTL_CTL_PCLK_DIV_IDLE) | \
				(1 << AP_PWR_CTLPCLK_CTL_CTL_PCLK_DIV_IDLE_WE))

/*
 *data_pclk:  active:bus_mclk / 1 
 *datal_pclk:  idle:  bus_mclk /2   
 */							
#define AP_PWR_DATAPCLK_VAL	((0x08 << AP_PWR_DATAPCLK_CTL_DATA_PCLK_DIV_WK) | \
				(1 << AP_PWR_DATAPCLK_CTL_DATA_PCLK_DIV_WK_WE) | \
				(0x04 << AP_PWR_DATAPCLK_CTL_DATA_PCLK_DIV_IDLE) | \
				(1 << AP_PWR_DATAPCLK_CTL_DATA_PCLK_DIV_IDLE_WE))

#define AP_PWR_APB_DF_VAL	((0xff << AP_PWR_CTL_DFSW_NUM) | \
				(0xff << AP_PWR_DATA_DFSW_NUM) | \
				(1 << AP_PWR_CTL_DFSW_NUM_WE) | \
				(1 << AP_PWR_DATA_DFSW_NUM_WE))
/*
 *sec_pclk: bus_mclk / 1. 
 *SDIOx_BUS_CLK is the same as sec_pclk
 */							
#define AP_PWR_SECPCLK_VAL	(0x08 << AP_PWR_SECPCLK_CTL_SEC_PCLK_DIV) 

/*gpu_mclk: 312M*/
#define AP_PWR_GPU_CLK_VAL	((0x08 << AP_PWR_GPU_CLK_CTL_GPU_CLK_GR) | \
				(1 << AP_PWR_GPU_CLK_CTL_GPU_CLK_GR_WE) | \
				(0x01 << AP_PWR_GPU_CLK_CTL_GPU_CLK_DR) | \
				(1 << AP_PWR_GPU_CLK_CTL_GPU_CLK_DR_WE))	 
							
/*on2_e0_clk/on2_e1_clk/on2_ebus_clk: 312M
 *on2_d_clk: 312M
 */
#define AP_PWR_ON2CLK_VAL	((0x08 << AP_PWR_ON2CLK_CTL_ON2ENC_CLK_GR) | \
				(1 << AP_PWR_ON2CLK_CTL_ON2ENC_CLK_GR_WE) | \
				(0x01 << AP_PWR_ON2CLK_CTL_ON2ENC_CLK_DR) | \
				(1 << AP_PWR_ON2CLK_CTL_ON2ENC_CLK_DR_WE) | \
				(0x08 << AP_PWR_ON2CLK_CTL_ON2DEC_CLK_GR) | \
				(1 << AP_PWR_ON2CLK_CTL_ON2DEC_CLK_GR_WE) | \
				(0x01 << AP_PWR_ON2CLK_CTL_ON2DEC_CLK_DR) | \
				(1 << AP_PWR_ON2CLK_CTL_ON2DEC_CLK_DR_WE))

/*
 *ddr_axi_clk: active: 312M
 *ddr_axi_clk: idle: 39M
 */
#define AP_PWR_DDRAXICLK_VAL	((0x08 << AP_PWR_DDRAXICLK_CTL_DDR_AXI_CLK_GR_WK) |\
				(1 << AP_PWR_DDRAXICLK_CTL_DDR_AXI_CLK_GR_WK_WE) | \
				(0x01 << AP_PWR_DDRAXICLK_CTL_DDR_AXI_CLK_GR_IDLE) | \
				(1 << AP_PWR_DDRAXICLK_CTL_DDR_AXI_CLK_GR_IDLE_WE) | \
				(0x01 << AP_PWR_DDRAXICLK_CTL_DDR_AXI_GPV_CLK_EN) | \
				(1 << AP_PWR_DDRAXICLK_CTL_DDR_AXI_GPV_CLK_EN_WE))


#define AP_PWR_DDRAXI_DF_VAL	(0x200 << AP_PWR_DDRAXI_DF_CTL_DDR_AXI_DFSW_NUM)

/*lcdc_axi_clk: 312M*/
#define AP_PWR_LCDCAXICLK_VAL	((0x08 << AP_PWR_LCDCAXICLK_CTL_LCDC_AXI_CLK_GR) | \
				(1 << AP_PWR_LCDCAXICLK_CTL_LCDC_AXI_CLK_GR_WE) | \
				(0x01 << AP_PWR_LCDCAXICLK_CTL_LCDC_AXI_CLK_DR) | \
				(1 << AP_PWR_LCDCAXICLK_CTL_LCDC_AXI_CLK_DR_WE))

/*lcdc0_mclk: 156*/
#define AP_PWR_LCDC0CLK_VAL	((0x08 << AP_PWR_LCDC0CLK_CTL_LCDC0_MCLK_GR) | \
				(1 << AP_PWR_LCDC0CLK_CTL_LCDC0_MCLK_GR_WE) | \
				(0x07 << AP_PWR_LCDC0CLK_CTL_LCDC0_MCLK_DIV) | \
				(1 << AP_PWR_LCDC0CLK_CTL_LCDC0_MCLK_DIV_WE))

/*isp_clk: 156*/
#define AP_PWR_ISPCLK_CTL0_VAL	((0x04 << AP_PWR_ISPCLK_CTL0_ISP_AXI_CLK_GR) | \
				(1 << AP_PWR_ISPCLK_CTL0_ISP_MCLK_GR_WE) | \
				(0x01 << AP_PWR_ISPCLK_CTL0_ISP_AXI_CLK_DR) | \
				(1 << AP_PWR_ISPCLK_CTL0_ISP_MCLK_DR_WE))

#define AP_PWR_ISPCLK_CTL1_VAL	((0x04<< AP_PWR_ISPCLK_CTL1_ISP_PSCLK_SCLK2_GR) | \
				(1 << AP_PWR_ISPCLK_CTL1_ISP_PSCLK_SCLK2_GR_WE) | \
				(0x01 << AP_PWR_ISPCLK_CTL1_ISP_PSCLK_SCLK2_DR) | \
				(1 << AP_PWR_ISPCLK_CTL1_ISP_PSCLK_SCLK2_DR_WE))

#ifdef CONFIG_CPU_LC1813
#define AP_PWR_BUS_LP_CTL0_VAL	( 0x3F | (0x0FFFFFF << 8))
#define AP_PWR_BUS_LP_CTL1_VAL	( 0x07 | (0x0FFF << 8))
#define AP_PWR_BUS_LP_CTL2_VAL	( 0x3F | (0x0FFFFFF << 8))
#define AP_PWR_BUS_LP_CTL3_VAL	( 0x0F | (0x0FFFF << 8) )
#define AP_PWR_BUS_LP_CTL4_VAL	( 0x3F | (0x0FFFFFF << 8) )
#define AP_PWR_BUS_LP_CTL5_VAL	( 0x0F | (0x0FFFF << 8) )
#endif

/*ddr_pwr_pll: 780M*/
#define DDR_PWR_PLLCTL_VAL	((0x00 << DDR_PWR_PLL_R_SEL) | \
				(0x1d << DDR_PWR_PLL_F_SEL) | \
				(0x01 << DDR_PWR_PLL_OD_SEL))

/*
 *i2c_ssi_clk: 130M
 *fraction_in_clk: 260M
 */
#define DDR_PWR_DIVCLKCTL_VAL	((0x02 << DDR_PWR_FRACTION_IN_DIV) | \
				(0x05 << DDR_PWR_I2C_SSI_DIV))

/*
 *com_apb_pclk: active:130M
 *com_apb_pclk:idle: 39M
 */
 /*
#define DDR_PWR_APBCLKCTL_VAL	((0x05 << DDR_PWR_COM_APB_PCLK_WK_DIV) | \
				(0x13 << DDR_PWR_COM_APB_PCLK_IDLE_DIV))
*/

/*
 *com_apb_pclk: active/idle:65M
 */
 #if (CONFIG_DDR_BUS_CLK==312000000)
#define DDR_PWR_APBCLKCTL_VAL	((0x09 << DDR_PWR_COM_APB_PCLK_WK_DIV) | \
				(0x09 << DDR_PWR_COM_APB_PCLK_IDLE_DIV))
#elif (CONFIG_DDR_BUS_CLK == 351000000)
#define DDR_PWR_APBCLKCTL_VAL	((0x0a << DDR_PWR_COM_APB_PCLK_WK_DIV) | \
				(0x0a << DDR_PWR_COM_APB_PCLK_IDLE_DIV))
#else
#define DDR_PWR_APBCLKCTL_VAL	((0x0b << DDR_PWR_COM_APB_PCLK_WK_DIV) | \
				(0x0b << DDR_PWR_COM_APB_PCLK_IDLE_DIV))
#endif

#define AP_DMAG_CH_LP_EN_VAL	(0xf | (0xf << 16)) /*enable lp for ch0-ch3 */
#define AP_DMAG_AHB_LP_EN_VAL	(0x01 << 0)

#define CTL_LP_MODE_CTRL_VAL	((0x0a << CTL_LP_MODE_CTRL_AHB_IDLE_LIMIT) | \
				(0x00 << CTL_LP_MODE_CTRL_CTL_LP_EN))
								
#define CTL_AP_DMAC_LP_EN_VAL	(0x1 << CTL_AP_DMAC_LP_EN_AP_DMAC_LP_EN)

#define CTL_MH2X_CH_LP_EN_VAL	((0x01 << CTL_MH2X_CH_LP_EN_ACP_MH2X_CH0_LP_EN) | \
				(0x01 << CTL_MH2X_CH_LP_EN_ACP_MH2X_CH1_LP_EN) | \
				(0x01 << CTL_MH2X_CH_LP_EN_ACP_MH2X_CH2_LP_EN) | \
				(0x01 << CTL_MH2X_CH_LP_EN_ACP_MH2X_CH3_LP_EN) | \
				(0x01 << CTL_MH2X_CH_LP_EN_USB_MH2X_CH0_LP_EN) | \
				(0x01 << CTL_MH2X_CH_LP_EN_USB_MH2X_CH1_LP_EN) | \
				(0x01 << CTL_MH2X_CH_LP_EN_USB_MH2X_CH2_LP_EN))

#define NFC_LP_CTRL_VAL		((0x01 << 0 ) | (0x01 << 1) | (0x0a << 8))

#define LCDC_LP_CTRL_VAL	((0x01 << 0) | (0x01 << 16) | (0x0a << 24))

#define HPI_LP_CTRL_VAL		((0x01 << 0) | (0x0a << 8))

#if CONFIG_DDR_AXI_M3_OPTIMIZATION
#define AP_PWR_LP_CTL_VAL	((0x01 << AP_PWR_CTL_PCLK_LP_EN) | \
				(0x01 << AP_PWR_DATA_PCLK_LP_EN) | \
				(0x01 << AP_PWR_UART0_CLK_OFF_PROTECT_EN) | \
				(0x01 << AP_PWR_UART1_CLK_OFF_PROTECT_EN) | \
				(0x01 << AP_PWR_UART2_CLK_OFF_PROTECT_EN) | \
				(0x01 << AP_PWR_AHB_BUS_LP_EN) | \
				(0x01 << AP_PWR_ICM_LP_EN) | \
				(0x00 << AP_PWR_DDR_AXI_LP_EN) | \
				(0x03 << AP_PWR_SLP_RAM_LP_MODE) | \
				(0x03 << AP_PWR_L2C_MEM_LP_EN))
#else
#define AP_PWR_LP_CTL_VAL	((0x01 << AP_PWR_CTL_PCLK_LP_EN) | \
				(0x01 << AP_PWR_DATA_PCLK_LP_EN) | \
				(0x01 << AP_PWR_UART0_CLK_OFF_PROTECT_EN) | \
				(0x01 << AP_PWR_UART1_CLK_OFF_PROTECT_EN) | \
				(0x01 << AP_PWR_UART2_CLK_OFF_PROTECT_EN) | \
				(0x01 << AP_PWR_AHB_BUS_LP_EN) | \
				(0x01 << AP_PWR_ICM_LP_EN) | \
				(0x01 << AP_PWR_DDR_AXI_LP_EN) | \
				(0x03 << AP_PWR_SLP_RAM_LP_MODE) | \
				(0x03 << AP_PWR_L2C_MEM_LP_EN))
#endif

#define DDR_PWR_LPCTL_VAL	((0x00 << DDR_PWR_CTRL_DDR_LP_BYPASS) | \
				(0x04 << DDR_PWR_DDR_LP_CLK_CLOSE_CNT) | \
				(0x00 << DDR_PWR_PHY_DDR_LP_BYPASS) | \
				(0x00 << DDR_PWR_DDR_AXI_LP_BYPASS))

#define DDR_PWR_CTL_VAL		((0x01 << DDR_PWR_CTL_CP_DDR_BRIDGE_LP_BYPASS) | \
				(0x01 << DDR_PWR_CTL_COM_APB_BUS_MONITOR_BYPASS) | \
				(0x01 << DDR_PWR_CTL_COM_UART_CLK_OFF_PROTECT_EN) | \
				(0x00 << DDR_PWR_CTL_COM_UART_LP_REQ_SCLK) | \
				(0x00 << DDR_PWR_CTL_COM_UART_LP_REQ_PCLK))

#define CIPHER_AES_LP_CTRL_VAL	((0x01 << 0) | (0x7f << 8))

#define CIPHER_SHA_LP_CTRL_VAL	((0x01 << 0) | (0x01 << 1) | (0x01 << 8))

#define SECURITY_SEC_LPC_CTRL_VAL	((0x01 << 0) | (0x01 << 1) | \
					(0x0a << 4) | (0x0a << 8))

#define AP_DMAS_LP_CTL_VAL	0x1ffff /*enable lp for ch00-ch15 */

#define AUDIO_DMAS_LP_CTL_VAL	0x1ffff /*enable lp for ch00-ch10 */


static inline void regs_array_init(reg_val *array, int size)
{
	int cnt;

	for (cnt = 0; cnt < size; ++cnt) {
		__raw_writel(array[cnt].val, array[cnt].reg);
		if (array[cnt].flag)
			CP15DSB;
	}
}

static inline void cipher_regs_init(void)
{
	u32 val;

	/*cipher_clk_en*/
	val = 1 << 5 | 1 << (5 + 16);
	__raw_writel(val, AP_PWR_SYSCLK_EN1);
	CP15DSB;

	__raw_writel(CIPHER_AES_LP_CTRL_VAL, CIPHER_AES_LP_CTRL);
	__raw_writel(CIPHER_SHA_LP_CTRL_VAL, CIPHER_SHA_LP_CTRL);
	CP15DSB;

	/*disable cipher_clk_en*/
	val = 0 << 5 | 1 << (5 + 16);
	__raw_writel(val, AP_PWR_SYSCLK_EN1);	
}

static inline void regs_close_unused_clk(void)
{
	u32 val;

#ifndef CONFIG_COMIP_NAND
	/*NFC_CLK_EN = 0*/
	val = 0 << 0 | 1 << (0 + 8);
	/*HPI_CLK_EN = 0*/
	val |= 0 << 1 | 1 << (1 + 8);
	__raw_writel(val, AP_PWR_NFCCLK_EN);
#endif

	val = 0;
	/*TB_MCLK_EN = 0*/
	val |= 0 << 0 | 1 << (0 + 8);
	/*PWM_MCLK_EN = 0*/
	val |= 0 << 4 | 1 << (4 + 8);	
	__raw_writel(val, AP_PWR_CTLAPBMCLK_EN);

	/*TB_PCLK_EN = 0*/
	val = 0 << 0 | 1 << (0 + 16);
	/*WDT_PCLK_EN = 0*/
	val |= 0 << 1 | 1 << (1 + 16);
#ifdef CONFIG_CPU_LC1813
	/*WDT0_PCLK_EN = 0*/
	val |= 0 << 8 | 1 << (8 + 16);
	/*WDT1_PCLK_EN = 0*/
	val |= 0 << 9 | 1 << (9 + 16);
	/*WDT2_PCLK_EN = 0*/
	val |= 0 << 10 | 1 << (10 + 16);
#endif
	/*PWM_PCLK_EN = 0*/
	val |= 0 << 6 | 1 << (6 + 16);	
	__raw_writel(val, AP_PWR_CTLPCLK_EN);

	val = 0;
#ifndef CONFIG_CPU_LC1813
	/*SDMMC3_PCLK_EN = 0*/
	val |= 0 << 8 | 1 << (8 + 16);
#endif
	/*SSI2_PCLK_EN = 0*/
	val |= 0 << 2 | 1 << (2 + 16);
	/*BP147_PCLK_EN = 0*/
	val |= 0 << 3 | 1 << (3 + 16);
	__raw_writel(val, AP_PWR_SECPCLK_EN);

#ifdef CONFIG_CPU_LC1813
	val = 0;
	/*SSI0_PCLK_EN = 0*/
	val |= 0 << 2 | 1 << (2 + 16);
	/*SSI1_PCLK_EN = 0*/
	val |= 0 << 3 | 1 << (3 + 16);
	/*SSI3_PCLK_EN = 0*/
	val |= 0 << 4 | 1 << (4 + 16);
	__raw_writel(val, AP_PWR_DATAPCLK_EN);
#endif

	val = 0;
#ifndef CONFIG_COMIP_NAND
	/*NFC_HCLK_EN = 0*/
	val |= 0 << 13 | 1 << (13 + 16);
#endif
#ifndef CONFIG_CPU_LC1813
	/*HPI_HCLK_EN = 0*/
	val |= 0 << 14 | 1 << (14 + 16);
#endif
	/*AP_DMAG_CLK_EN = 0*/
	val |= 0 << 11 | 1 << (11 + 16);
	__raw_writel(val, AP_PWR_SYSCLK_EN0);
}

int comip_lp_regs_init(void)
{
	/*LP*/
	reg_val lp_regs[] = {
		{AP_DMAG_CH_LP_EN,		AP_DMAG_CH_LP_EN_VAL,		0},
		{AP_DMAG_AHB_LP_EN,		AP_DMAG_AHB_LP_EN_VAL,		0},
		{AP_DMAS_LP_CTL,		AP_DMAS_LP_CTL_VAL,		0},
		{AUDIO_DMAS_LP_CTRL,		AUDIO_DMAS_LP_CTL_VAL,		0},
		{CTL_LP_MODE_CTRL,		CTL_LP_MODE_CTRL_VAL,		0},
		{CTL_AP_DMAC_LP_EN,		CTL_AP_DMAC_LP_EN_VAL,		0},
		{CTL_MH2X_CH_LP_EN,		CTL_MH2X_CH_LP_EN_VAL,		0},
		//{NFC_LP_CTRL,			NFC_LP_CTRL_VAL,		0},
		{LCDC0_LP_CTRL,			LCDC_LP_CTRL_VAL,		0},
#ifndef CONFIG_CPU_LC1813
		{LCDC1_LP_CTRL,			LCDC_LP_CTRL_VAL,		0},
		{HPI_LP_CTRL,			HPI_LP_CTRL_VAL,		0},
#endif
		{AP_PWR_LP_CTL,			AP_PWR_LP_CTL_VAL,		0},
		//{DDR_PWR_LPCTL,			DDR_PWR_LPCTL_VAL,		0},
		{DDR_PWR_CTL,			DDR_PWR_CTL_VAL,		0},
		{AP_PWR_APB_DF_CTL,		AP_PWR_APB_DF_VAL,		0},
		{AP_PWR_DDRAXI_DF_CTL,		AP_PWR_DDRAXI_DF_VAL,		0},
		{SECURITY_SEC_LPC_CTRL,		SECURITY_SEC_LPC_CTRL_VAL,	0},
	};

	/*AP-pwr*/
	reg_val ap_pwr_regs[] = {
		//{AP_PWR_A9_CLK_CTL,		AP_PWR_A9_CLK_VAL,		1},
		{AP_PWR_SYSCLK_CTL,		AP_PWR_SYSCLK_VAL,		1},
		{AP_PWR_CTLPCLK_CTL,		AP_PWR_CTLPCLK_VAL,		1},
		{AP_PWR_DATAPCLK_CTL,		AP_PWR_DATAPCLK_VAL,		1},
		{AP_PWR_SECPCLK_CTL,		AP_PWR_SECPCLK_VAL,		1},
		{AP_PWR_GPU_CLK_CTL,		AP_PWR_GPU_CLK_VAL,		1},
		{AP_PWR_ON2CLK_CTL,		AP_PWR_ON2CLK_VAL,		1},
		{AP_PWR_DDRAXICLK_CTL,		AP_PWR_DDRAXICLK_VAL,		1},
		{AP_PWR_LCDCAXICLK_CTL,		AP_PWR_LCDCAXICLK_VAL,		1},
		{AP_PWR_LCDC0CLK_CTL,		AP_PWR_LCDC0CLK_VAL,		1},
		{AP_PWR_ISPCLK_CTL0,		AP_PWR_ISPCLK_CTL0_VAL,		1},
		{AP_PWR_ISPCLK_CTL1,		AP_PWR_ISPCLK_CTL1_VAL,		1},
#ifdef CONFIG_CPU_LC1813
#if CONFIG_BUS_LP_ENABLE
		{AP_PWR_BUS_LP_CTL0, 		AP_PWR_BUS_LP_CTL0_VAL,		1},
		{AP_PWR_BUS_LP_CTL1, 		AP_PWR_BUS_LP_CTL1_VAL,		1},
		{AP_PWR_BUS_LP_CTL2, 		AP_PWR_BUS_LP_CTL2_VAL,		1},
		{AP_PWR_BUS_LP_CTL3, 		AP_PWR_BUS_LP_CTL3_VAL,		1},
		{AP_PWR_BUS_LP_CTL4, 		AP_PWR_BUS_LP_CTL4_VAL,		1},
#endif
#endif
	};

#ifdef CONFIG_CPU_LC1813
#if CONFIG_BUS_LP_ENABLE
	/*1813s-AP_PWR_BUS_LP_CTL5*/
	reg_val lp_1813s_reg[] ={
		{AP_PWR_BUS_LP_CTL5, 		AP_PWR_BUS_LP_CTL5_VAL,		1},
	};

	if(cpu_is_lc1813s())
		regs_array_init(lp_1813s_reg, ARRAY_SIZE(lp_1813s_reg));
#endif
#endif

	/*ddr-pwr*/
	reg_val ddr_pwr_regs[] = {
		{DDR_PWR_DIVCLKCTL,		DDR_PWR_DIVCLKCTL_VAL,		1},
		{DDR_PWR_APBCLKCTL,		DDR_PWR_APBCLKCTL_VAL,		1},
	};
	
	/*pllx_adjust();*/
	regs_array_init(ap_pwr_regs, ARRAY_SIZE(ap_pwr_regs));
	regs_array_init(ddr_pwr_regs, ARRAY_SIZE(ddr_pwr_regs));	
	regs_array_init(lp_regs, ARRAY_SIZE(lp_regs));

	cipher_regs_init();
	regs_close_unused_clk();
	
	return 0;
}

