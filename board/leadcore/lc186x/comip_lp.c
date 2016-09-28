#include <common.h>
#include <asm/io.h>
#include <asm/armv7.h>

DECLARE_GLOBAL_DATA_PTR;

typedef struct {
	u32 reg;
	u32 val;
	u8  flag;
} reg_val; 

#define CLK_BIT_DIS(bit) ((0x0 << bit) | (0x1 << (bit + 16)))

#define AP_PWR_TM_CYCLES			1000 /* > 10ms*/

#define AP_DMAG_CH_LP_EN0			(AP_DMAG_BASE + 0x80)
#define AP_DMAG_CH_BUS_LP_EN			(AP_DMAG_BASE + 0x88)
#define AP_DMAS_LP_CTL				(AP_DMAS_BASE + 0x3fc)
#define SECURITY_LP_EN				(AP_SECURITY_BASE + 0x30)
#define CIPHER_AES_LP_CTRL			(CIPHER_AES_BASE + 0xa0)
#define CIPHER_SHA_LP_CTRL			(CIPHER_SHA_BASE + 0x84)
#define LCDC0_LP_CTRL				(LCDC0_BASE + 0x1c0)
#define LCDC1_LP_CTRL				(LCDC1_BASE + 0x1c0)
#define HPI_LP_CTRL				(HPI_BASE + 0x18)
#define DSI_ISP_CTL_LP_MODE_CTRL		(MIPI_BASE + 0xa0)
#define USB_CTL_LP_MODE_CTRL			(USB_CTL_BASE + 0xa0)
#define TOP_MAILBOX_LP_MODE_CTRL		(TOP_MAILBOX_BASE + 0x12c)
#define TOP_DMAS_LP_CTL				(TOP_DMAS_BASE + 0x3fc)
#define TOP_DMAG_CH_LP_EN0			(TOP_DMAG_BASE + 0x80)
#define TOP_DMAG_CH_BUS_LP_EN			(TOP_DMAG_BASE + 0x88)
/*PLL0: 1196M*/
#define AP_PWR_PLL0CFG_VAL	((0x00 << AP_PWR_PLL0CFG_CTL_PLL0OD_SEL) | \
				(0x2d << AP_PWR_PLL0CFG_CTL_PLL0F_SEL) | \
				(0x01 << AP_PWR_PLL0CFG_CTL_PLL0R_SEL))

/*PLL0: 988*/
#define AP_PWR_PLL0CFG_998M_VAL	((0x01 << AP_PWR_PLL0CFG_CTL_PLL0OD_SEL) | \
				(0x4b << AP_PWR_PLL0CFG_CTL_PLL0F_SEL) | \
				(0x01 << AP_PWR_PLL0CFG_CTL_PLL0R_SEL))
/* ctl_pclk, data_pclk, sec_pclk have the same default value:
*/

/*gpu_mclk: 312M*/
#define AP_PWR_GPU_CLK_VAL	((0x03 << 0) | (0x07 << 16))
 /* on2_enc_mclk: 312M */
#define AP_PWR_ON2CLK_CTL0_VAL	((0x03 << 0) | (0x07 << 16))
/* on2_codec_mclk: 312M */
#define AP_PWR_ON2CLK_CTL1_VAL	((0x03 << 0) | (0x07 << 16))
/* acc2d_mclk: 312M */
#define AP_PWR_ACC2DMCLK_CTL_VAL ((0x03 << 0) | (0x07 << 16))
/*lcdc_axi_clk: 312M*/
#define AP_PWR_LCDCAXICLK_VAL	((0x03 << 0) | (0x07 << 16))
/*lcdc0_mclk: 156*/
#define AP_PWR_LCDC0CLK_VAL	(((0x7 << 0) | (0x7 << 16)) | ((0x7 << 4) | (0xf << 20)))
/*isp_clk 156 default*/
#define AP_PWR_BUS_LP_CTL0_VAL	(0xFFFFFFFF)
#define AP_PWR_BUS_LP_CTL1_VAL	(0x7FFFFFFF)
#define AP_PWR_BUS_LP_CTL2_VAL	(0xF)

#define AP_DMAG_CH_LP_EN_VAL	(0xfff | (0xfff << 16)) /*enable lp for ch0-ch11 */
#define AP_DMAG_CH_BUS_LP_EN_VAL	((0x01 << 1) | (0x01 << 17) | (0x01 << 0) | (0x01 << 16))
#define CTL_LP_MODE_CTRL_VAL	((0x01 << 0) | (0x0a << 8))

//#define LCDC_LP_CTRL_VAL	((0x01 << 0) | (0x01 << 16) | (0x0a << 24))
#define LCDC_LP_CTRL_VAL	((0x01 << 16) | (0x0a << 24))
#define DSI_ISP_CTL_LP_VAL	((0x01 << 0) | (0x0a << 8))
#define HPI_LP_CTRL_VAL		((0x01 << 0) | (0x0a << 8))
#define CIPHER_AES_LP_CTRL_VAL	((0x01 << 0) | (0x7f << 8))
#define CIPHER_SHA_LP_CTRL_VAL	((0x01 << 0) | (0x01 << 1) | (0x01 << 8))

#define SECURITY_LP_EN_VAL	((0x01 << 0) | (0x01 << 1) | (0x01 << 2))
#define USB_CTL_LP_VAL		((0x01 << 0) | (0x0a << 8))
#define AP_DMAS_LP_CTL_VAL	0x1ffff /*enable lp for ch00-ch15 */
#if 1
#define AP_PWR_LP_CTL_VAL	((0x01 << AP_PWR_CTL_PCLK_DFS_EN) | \
				(0x01 << AP_PWR_DATA_PCLK_DFS_EN) | \
				(0x01 << AP_PWR_SEC_PCLK_DFS_EN) | \
				(0x01 << AP_PWR_SLP_VIDEO_BUS_PD_EN) | \
				(0x01 << AP_PWR_SLP_RAM_RET_EN))
#else
#define AP_PWR_LP_CTL_VAL	((0x01 << AP_PWR_CTL_PCLK_DFS_EN) | \
				(0x01 << AP_PWR_DATA_PCLK_DFS_EN) | \
				(0x01 << AP_PWR_SEC_PCLK_DFS_EN) | \
				(0x01 << AP_PWR_SLP_HBLK0_PD_EN) | \
				(0x01 << AP_PWR_SLP_VIDEO_BUS_PD_EN) | \
				(0x01 << AP_PWR_SLP_RAM_RET_EN) | \
				(0x01 << AP_PWR_SLP_USB_BUS_PD_EN))
#endif
#define DDR_PWR_TOPBUSLPCTL_VAL	((0x01 << MIDWAY_BACK_ACK_MK) | \
	(0x01 << TL420RAM_TBAXI_LP_EN) | \
	(0x01 << DDRBUS_TBAXI_LP_EN) | \
	(0x01 << TOPCTRLBUS_TBAXI_LP_EN) | \
	(0x01 << TOCP_TBAXI_LP_EN) | \
	(0x01 << TOAP_TBAXI_LP_EN) | \
	(0x01 << APTRACE_TBAXI_LP_EN) | \
	(0x01 << CPTRACE_TBAXI_LP_EN) | \
	(0x01 << TL420AXI1_TBAXI_LP_EN) | \
	(0x01 << TL420AXI0_TBAXI_LP_EN) | \
	(0x01 << TOPDMAG_TBAXI_LP_EN) | \
	(0x01 << TOPDMAS_TBAXI_LP_EN) | \
	(0x01 << APTOP_TBAXI_LP_EN) | \
	(0x01 << CPTOP_TBAXI_LP_EN) | \
	(0x01 << CPBOOTRAM_TBAXI_LP_EN) | \
	(0x01 << TOPRAM1_TBAXI_LP_EN) | \
	(0x01 << MAIN_TBAXI_LP_EN))
#define DDR_PWR_DDRBUSLPCTL_VAL	((0x01 << AP1_DBAXI_LP_EN) | \
	(0x01 << MAIN_DBAXI_LP_EN) | \
	(0x01 << TOPBUS_DBAXI_LP_EN) | \
	(0x01 << AP0_DBAXI_LP_EN) | \
	(0x01 << CP_DBAXI_LP_EN) | \
	(0x01 << CPA7_DBAXI_LP_EN) | \
	(0x01 << DDR1_DBAXI_LP_EN) | \
	(0x01 << DDR0_DBAXI_LP_EN))
#define DDR_PWR_TOPCTRLBUSLPCTL_VAL	((0x01 << COM_APB_BUS_LP_EN) | \
	(0x01 << MAIN_TCBAXI_LP_EN) | \
	(0x01 << DDR1_TCBAXI_LP_EN) | \
	(0x01 << DDR0_TCBAXI_LP_EN) | \
	(0x01 << TOPMAILBOX_TCBAXI_LP_EN) | \
	(0x01 << TOPDMAS_TCBAXI_LP_EN) | \
	(0x01 << TOPDMAG_TCBAXI_LP_EN) | \
	(0x01 << TL420_ICTL_TCBAXI_LP_EN))

#define TOP_MAILBOX_LP_MODE_VAL	(0x1ff)
#define TOP_DMAS_LP_VAL		(0xffff)
#define TOP_DMAG_CH_LP_EN0_VAL	((0x01 << 1) | (0x01 << 17) | (0x01 << 0) | (0x01 << 16))
#define TOP_DMAG_CH_BUS_EN_VAL	((0x01 << 1) | (0x01 << 17) | (0x01 << 0) | (0x01 << 16))

static inline void regs_array_init(reg_val *array, int size)
{
	int cnt;

	for (cnt = 0; cnt < size; ++cnt) {
		__raw_writel(array[cnt].val, array[cnt].reg);
		if (array[cnt].flag)
			CP15DSB;
	}
}
/* AP TOP bus low power enable,
 * must set AP_PWR_BUSLP_CTL & DDR_PWR_TOPBUSLPCTL
 */
static inline void bus_lp_regs_init(void)
{
	u32 val;

	val = (0x1 << 1 | 0x1 << 17);
	__raw_writel(val, AP_PWR_BUSLP_CTL);

	val = __raw_readl(DDR_PWR_TOPBUSLPCTL);
	val |= (0x1 << 31);
	__raw_writel(val, DDR_PWR_TOPBUSLPCTL);

	val = __raw_readl(AP_PWR_BUSLP_CTL);
	val |= (0x1 << 0 | 0x1 << 16);
	__raw_writel(val, AP_PWR_BUSLP_CTL);

	val = __raw_readl(DDR_PWR_TOPBUSLPCTL);
	val |= DDR_PWR_TOPBUSLPCTL_VAL;
	__raw_writel(val, DDR_PWR_TOPBUSLPCTL);

	val = __raw_readl(DDR_PWR_DDRBUSLPCTL);
	val |= DDR_PWR_DDRBUSLPCTL_VAL;
	__raw_writel(val, DDR_PWR_DDRBUSLPCTL);

	val = __raw_readl(DDR_PWR_TOPCTRLBUSLPCTL);
	val |= DDR_PWR_TOPCTRLBUSLPCTL_VAL;
	__raw_writel(val, DDR_PWR_TOPCTRLBUSLPCTL);

}

static inline void cipher_regs_init(void)
{
	/* cipher_clk default enable */
	__raw_writel(CIPHER_AES_LP_CTRL_VAL, CIPHER_AES_LP_CTRL);
	__raw_writel(CIPHER_SHA_LP_CTRL_VAL, CIPHER_SHA_LP_CTRL);
	CP15DSB;
}

static inline void regs_close_unused_clk(void)
{
	u32 val;

	val = (CLK_BIT_DIS(AP_PWR_CLK_EN0_AP_DMAG_CLK) | \
		CLK_BIT_DIS(AP_PWR_CLK_EN0_AP_DMAC_CLK) | \
		CLK_BIT_DIS(AP_PWR_CLK_EN0_HSIC_HCLK) | \
		CLK_BIT_DIS(AP_PWR_CLK_EN0_HPI_HCLK));
#ifndef CONFIG_COMIP_NAND
	val |= CLK_BIT_DIS(AP_PWR_CLK_EN0_NFC_HCLK);
#endif
	__raw_writel(val, AP_PWR_CLK_EN0);

	val = CLK_BIT_DIS(AP_PWR_CLK_EN1_AP_SW5_CIPHER_CLK);
	__raw_writel(val, AP_PWR_CLK_EN1);

	val = (CLK_BIT_DIS(AP_PWR_CLK_EN2_AP_SW4_HSIC_CLK) | \
		CLK_BIT_DIS(AP_PWR_CLK_EN2_AP_SW5_DMAC_0_CLK) | \
		CLK_BIT_DIS(AP_PWR_CLK_EN2_AP_SW5_DMAC_1_CLK));
	__raw_writel(val, AP_PWR_CLK_EN2);

	val = CLK_BIT_DIS(AP_PWR_CLK_EN4_PLL0_DIV13_CLK);
	__raw_writel(val, AP_PWR_CLK_EN4);

#ifdef CONFIG_DEBUG_CLOCK_DISABLED
	val = CLK_BIT_DIS(AP_PWR_CLK_EN3_HA7_DBG_CLK);
	__raw_writel(val, AP_PWR_CLK_EN3);

	val = (CLK_BIT_DIS(AP_PWR_CLK_EN5_SA7_DBG_CLK) | \
		CLK_BIT_DIS(AP_PWR_CLK_EN5_SA7_ATBBRG_SCLK) | \
		CLK_BIT_DIS(AP_PWR_CLK_EN5_SA7_APBBRG_MCLK) | \
		CLK_BIT_DIS(AP_PWR_CLK_EN5_HA7_ATBBRG_SCLK) | \
		CLK_BIT_DIS(AP_PWR_CLK_EN5_HA7_APBBRG_MCLK));
	__raw_writel(val, AP_PWR_CLK_EN5);

	val = CLK_BIT_DIS(CORESIGHT_CLK_EN);
	__raw_writel(val, DDR_PWR_CLKEN0);
#endif

	val |= CLK_BIT_DIS(AP_PWR_CTLAPBMCLK_EN_PWM_CLK_EN);
	__raw_writel(val, AP_PWR_CTLAPBMCLK_EN);

	val = (CLK_BIT_DIS(AP_PWR_CTLPCLK_EN_WDT0_PCLK) | \
		CLK_BIT_DIS(AP_PWR_CTLPCLK_EN_WDT1_PCLK) | \
		CLK_BIT_DIS(AP_PWR_CTLPCLK_EN_WDT2_PCLK) | \
		CLK_BIT_DIS(AP_PWR_CTLPCLK_EN_WDT3_PCLK) | \
		CLK_BIT_DIS(AP_PWR_CTLPCLK_EN_PWM_PCLK));
	__raw_writel(val, AP_PWR_CTLPCLK_EN);

	val = CLK_BIT_DIS(AP_PWR_SECAPBMCLK_EN_KBS_MCLK_EN);
	__raw_writel(val, AP_PWR_SECAPBMCLK_EN);

	val = (CLK_BIT_DIS(AP_PWR_SECPCLK_EN_SSI2_PCLK) | \
		CLK_BIT_DIS(AP_PWR_SECPCLK_EN_BP147_PCLK) | \
		CLK_BIT_DIS(AP_PWR_SECPCLK_EN_KBS_PCLK));
	__raw_writel(val, AP_PWR_SECPCLK_EN);

	val = (CLK_BIT_DIS(AP_PWR_DATAPCLK_EN_I2S_PCLK) | \
		CLK_BIT_DIS(AP_PWR_DATAPCLK_EN_SSI0_PCLK) | \
		CLK_BIT_DIS(AP_PWR_DATAPCLK_EN_SSI1_PCLK));
	__raw_writel(val, AP_PWR_DATAPCLK_EN);
}

int comip_lp_regs_init(void)
{
	/* LP */
	reg_val lp_regs[] = {
		{AP_DMAG_CH_LP_EN0,		AP_DMAG_CH_LP_EN_VAL,		0},
		//{AP_DMAG_CH_BUS_LP_EN,		AP_DMAG_AHB_LP_EN_VAL,		0},
		//{AP_DMAG_CH_BUS_LP_EN, 		AP_DMAG_AXI_LP_EN_VAL,		0},
		{AP_DMAG_CH_BUS_LP_EN,		AP_DMAG_CH_BUS_LP_EN_VAL,		0},
		{AP_DMAS_LP_CTL,		AP_DMAS_LP_CTL_VAL,		0},
		{CTL_LP_MODE_CTRL,		CTL_LP_MODE_CTRL_VAL,		0},
		{DSI_ISP_CTL_LP_MODE_CTRL,	DSI_ISP_CTL_LP_VAL, 		0},
		{LCDC0_LP_CTRL,			LCDC_LP_CTRL_VAL,		0},
		{LCDC1_LP_CTRL,			LCDC_LP_CTRL_VAL,		0},
		/* Disable HPI lowpower mode, because it could lead to system hang on leadcore picture */
		//{HPI_LP_CTRL,			HPI_LP_CTRL_VAL,		0},
		{AP_PWR_LP_CTL,			AP_PWR_LP_CTL_VAL,		0},
	};

	/* AP_PWR */
	reg_val ap_pwr_regs[] = {
		{AP_PWR_GPUCLK_CTL,		AP_PWR_GPU_CLK_VAL,		1},
		{AP_PWR_ON2CLK_CTL0,		AP_PWR_ON2CLK_CTL0_VAL,		1},
		{AP_PWR_ON2CLK_CTL1,		AP_PWR_ON2CLK_CTL1_VAL,	1},
		{AP_PWR_ACC2DMCLK_CTL,		AP_PWR_ACC2DMCLK_CTL_VAL,	1},
		{AP_PWR_LCDCAXICLK_CTL,		AP_PWR_LCDCAXICLK_VAL,		1},
		{AP_PWR_LCDC0CLK_CTL,		AP_PWR_LCDC0CLK_VAL,		1},
#if CONFIG_BUS_LP_ENABLE
		{AP_PWR_BUSLP_EN0, 		AP_PWR_BUS_LP_CTL0_VAL,		1},
		{AP_PWR_BUSLP_EN1, 		AP_PWR_BUS_LP_CTL1_VAL,		1},
		{AP_PWR_BUSLP_EN2, 		AP_PWR_BUS_LP_CTL2_VAL,		1},
#endif
	};
	
	/* TOP LP MODE */
	reg_val top_lp_regs[] = {
		//{TOP_MAILBOX_LP_MODE_CTRL,	TOP_MAILBOX_LP_MODE_VAL,	1},
		{TOP_DMAS_LP_CTL,		TOP_DMAS_LP_VAL,		1},
		{TOP_DMAG_CH_LP_EN0,		TOP_DMAG_CH_LP_EN0_VAL,		1},
		{TOP_DMAG_CH_BUS_LP_EN,		TOP_DMAG_CH_BUS_EN_VAL,		1},				
	};
	/*pllx_adjust();*/
	bus_lp_regs_init();
	regs_array_init(ap_pwr_regs, ARRAY_SIZE(ap_pwr_regs));
	regs_array_init(lp_regs, ARRAY_SIZE(lp_regs));
	regs_array_init(top_lp_regs, ARRAY_SIZE(top_lp_regs));

	regs_close_unused_clk();
	
	return 0;
}


