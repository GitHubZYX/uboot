/* driver/video/comip/comipfb.c
**
** Use of source code is subject to the terms of the LeadCore license agreement under
** which you licensed source code. If you did not accept the terms of the license agreement,
** you are not authorized to use source code. For the terms of the license, please see the
** license agreement between you and LeadCore.
**
** Copyright (c) 1999-2015	LeadCoreTech Corp.
**
**	PURPOSE:			This files contains the driver of LCD controller.
**
**	CHANGE HISTORY:
**
**	Version		Date		Author			Description
**	0.1.0		2014-08-19				created
**
*/

#include <linux/types.h>
#include <errno.h>
#include <linux/string.h>
#include <linux/compiler.h>
#include <asm/atomic.h>
#include <linux/err.h>

#include "comipfb.h"
#include "comip_lcdc.h"
#include "comipfb_if.h"
#include "common.h"

DECLARE_GLOBAL_DATA_PTR;

/*************************************************************************************************/
/*platform resource*/

struct resource comip_resource_fb0[] = {
	[0] = {
		.start = LCDC0_BASE,
		.end = LCDC0_BASE + 0xfff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DSI_BASE,
		.end = MIPI_BASE + 0xffff,
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = INT_LCDC0,
		.end = INT_LCDC0,
		.flags = IORESOURCE_IRQ,
	},
};

/* LCD. */
static struct comipfb_platform_data comip_lcd_info = {
	.lcdcaxi_clk = 312000000,
	.lcdc_support_interface = COMIPFB_MIPI_IF,
	.phy_ref_freq = 26000,	/* KHz */
	.gpio_rst = LCD_RESET_PIN,
	.gpio_im = -1,
	.flags = COMIPFB_SLEEP_POWEROFF,
};

/*****************************************************************************/
/*clock*/
/* Clock flags */
#define CLK_RATE_FIXED		(1 << 0)
#define CLK_RATE_DIV_FIXED	(1 << 1)
#define CLK_ALWAYS_ENABLED	(1 << 2)
#define CLK_INIT_DISABLED	(1 << 3)
#define CLK_INIT_ENABLED	(1 << 4)

struct clk {
	const char		*name;
	struct clk		*parent;
	unsigned long		rate;
	unsigned int		flag;
	void __iomem		*mclk_reg;
	void __iomem		*ifclk_reg;
	void __iomem		*divclk_reg;
	void __iomem		*grclk_reg;
	void __iomem		*busclk_reg;
	unsigned char		mclk_we_bit;
	unsigned char		mclk_bit;
	unsigned char		ifclk_we_bit;
	unsigned char		ifclk_bit;
	unsigned char		divclk_we_bit;
	unsigned char		divclk_bit;
	unsigned char		divclk_val;
	unsigned char		divclk_mask;
	unsigned char		grclk_we_bit;
	unsigned char		grclk_bit;
	unsigned char		grclk_val;
	unsigned char		grclk_mask;
	unsigned char		busclk_we_bit;
	unsigned char		busclk_bit;
	void			(*init)(struct clk *);
	int			(*enable)(struct clk *);
	void			(*disable)(struct clk *);
	int			(*set_rate)(struct clk *, unsigned long);
	int			(*set_parent)(struct clk *, struct clk *);
};

static void clk_pllx_init(struct clk *clk)
{
	unsigned long parent_rate;
	unsigned int val;
	unsigned int refdiv;
	unsigned int fbdiv;
	unsigned int postdiv1;
	unsigned int postdiv2;

	parent_rate = clk->parent->rate;
	val = readl(clk->divclk_reg);
	refdiv = val & 0x3f;
	fbdiv = (val >> 8) & 0xfff;
	postdiv1 = (val >> 20) & 0x7;
	postdiv2 = (val >> 24) & 0x7;

	clk->rate = parent_rate / refdiv / postdiv1 / postdiv2 * fbdiv;

	printf("%s rate:%ld\n", clk->name, clk->rate);
}

static void sdiv_g16plus1_init(struct clk *clk)
{
	unsigned int div;
	unsigned int grp;
	unsigned int val;

	val = readl(clk->divclk_reg);
	div = (val >> clk->divclk_bit) & clk->divclk_mask;
	clk->divclk_val = div;

	clk->rate = clk->parent->rate / (div + 1);

	val = readl(clk->grclk_reg);
	grp = (val >> clk->grclk_bit) & clk->grclk_mask;
	clk->grclk_val = grp;

	clk->rate = clk->rate / 16 * (grp + 1);
}

static int clk_enable_generic(struct clk * clk)
{
	unsigned int val;

	if (clk->mclk_reg != 0) {
		val = (1 << clk->mclk_bit) | (1 << clk->mclk_we_bit);
		writel(val, clk->mclk_reg);
	} else if (clk->grclk_reg != 0) {
		val = (clk->grclk_mask << clk->grclk_we_bit)
			| (clk->grclk_val << clk->grclk_bit);
		writel(val, clk->grclk_reg);
	}

	if (clk->ifclk_reg != 0) {
		val = (1 << clk->ifclk_bit) | (1 << clk->ifclk_we_bit);
		writel(val, clk->ifclk_reg);
	}

	if (clk->busclk_reg != 0) {
		val = (1 << clk->busclk_bit) | (1 << clk->busclk_we_bit);
		writel(val, clk->busclk_reg);
	}

	return 0;
}

static void clk_disable_generic(struct clk * clk)
{
	unsigned int val;

	if (clk->mclk_reg != 0) {
		val = (1 << clk->mclk_we_bit);
		writel(val, clk->mclk_reg);
	} else if (clk->grclk_reg != 0) {
		val = (clk->grclk_mask << clk->grclk_we_bit);
		writel(val, clk->grclk_reg);
	}

	if (clk->ifclk_reg != 0) {
		val = (1 << clk->ifclk_we_bit);
		writel(val, clk->ifclk_reg);
	}
}

static int sdiv_g16plus1_set_rate_noen(struct clk *clk, unsigned long rate)
{
	unsigned long parent_rate = clk->parent->rate;
	unsigned long cal_rate;
	unsigned int div;
	unsigned int grp;
	unsigned int val = 0;
	unsigned int gap;

	if (rate > parent_rate)
		return -EINVAL;

	gap = rate;	/*for calculate the close rate*/

	for (grp = 0; grp <= clk->grclk_mask; grp++) {
		for (div = 1; div <= clk->divclk_mask; div++) {
			cal_rate = parent_rate / (div + 1) / 16 * (grp + 1);
			if (cal_rate == rate) {
				clk->divclk_val = div;
				clk->grclk_val = grp;
				clk->rate = rate;
				val = (clk->divclk_mask << clk->divclk_we_bit) | (clk->divclk_val << clk->divclk_bit)
					| (clk->grclk_mask << clk->grclk_we_bit) | (clk->grclk_val << clk->grclk_bit);

				writel(val, clk->divclk_reg);
				udelay(2);
				return  0;
			} else {
				if (abs(rate - cal_rate) < gap) {
					clk->divclk_val = div;
					clk->grclk_val = grp;
					clk->rate = cal_rate;
					val = (clk->divclk_mask << clk->divclk_we_bit) | (clk->divclk_val << clk->divclk_bit)
						| (clk->grclk_mask << clk->grclk_we_bit) | (clk->grclk_val << clk->grclk_bit);			
					gap = abs(rate - cal_rate);
				}
			}
		}
	}

	writel(val, clk->divclk_reg);
	udelay(2);

	printf("set %s rate %ld\n",  clk->name, clk->rate);

	return  -EINVAL;

}

static void g8plus1_sdiv_init(struct clk *clk)
{
	unsigned int div;
	unsigned int grp;
	unsigned int val;

	val = readl(clk->grclk_reg);
	grp = (val >> clk->grclk_bit) & clk->grclk_mask;
	clk->grclk_val = grp;
	clk->rate = clk->parent->rate / 8 * (grp + 1);

	val = readl(clk->divclk_reg);
	div = (val >> clk->divclk_bit) & clk->divclk_mask;
	clk->divclk_val = div;

	clk->rate = clk->rate / (div + 1);
}

static int g8plus1_sdiv_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned long parent_rate = clk->parent->rate;
	unsigned long cal_rate;
	unsigned int div;
	unsigned int grp;
	unsigned int val;
	unsigned int en = 0;
	unsigned int gap;

	if (rate > parent_rate)
		return -EINVAL;

	/*for sdiv disable clk first*/
	if(clk->mclk_reg) {
		val = readl(clk->mclk_reg);
		en = (val >> clk->mclk_bit) & 0x1;
		if(en) {
			if(clk->disable)
				clk->disable(clk);
		}
	}

	gap = rate;	/*for calculate the close rate*/

	for (div = 1; div <= clk->divclk_mask; div++) {
		for (grp = 0; grp <= clk->grclk_mask; grp++) {
			cal_rate = parent_rate / (div + 1) / 8 * (grp + 1);
			if (cal_rate == rate) {
				clk->divclk_val = div;
				clk->grclk_val = grp;
				clk->rate = rate;
				val = (clk->divclk_mask << clk->divclk_we_bit)
					| (clk->divclk_val << clk->divclk_bit);
				val |= (clk->grclk_mask << clk->grclk_we_bit)
					| (clk->grclk_val << clk->grclk_bit);
				writel(val, clk->divclk_reg);

				/*restore enable status*/
				if(clk->mclk_reg) {
					if(en) {
						if(clk->enable)
							clk->enable(clk);
					}
				}

				return	0;
			} else {
				if (abs(rate - cal_rate) < gap) {
					clk->divclk_val = div;
					clk->grclk_val = grp;
					clk->rate = cal_rate;
					gap = abs(rate - cal_rate);
				}

			}
		}
	}

	val = (clk->divclk_mask << clk->divclk_we_bit)
		| (clk->divclk_val << clk->divclk_bit);
	val |= (clk->grclk_mask << clk->grclk_we_bit)
		| (clk->grclk_val << clk->grclk_bit);
	writel(val, clk->divclk_reg);

	/*restore enable status*/
	if(clk->mclk_reg) {
		if(en) {
			if(clk->enable)
				clk->enable(clk);
		}
	}

	printf("set %s rate %ld\n",  clk->name, clk->rate);

	return	-EINVAL;
}



/* Typical 26MHz in standalone mode. */
static struct clk osc_clk = {
	.name = "osc_clk",
	.rate = 26000000,
	.flag = CLK_RATE_FIXED | CLK_ALWAYS_ENABLED,
};

static struct clk pll1_out = {
	.name = "pll1_out",
	.parent = &osc_clk,
	.flag = CLK_RATE_FIXED | CLK_ALWAYS_ENABLED,
	.divclk_reg = (void __iomem *)io_p2v(AP_PWR_PLL1CFG_CTL0),
	.init = &clk_pllx_init,
};

static struct clk lcdc_axi_clk = {
	.name = "lcdc_axi_clk",
	.parent = &pll1_out,
	.mclk_reg = (void __iomem *)io_p2v(AP_PWR_LCDCAXICLK_CTL),
	.mclk_bit = 8,
	.mclk_we_bit = 24,
	.divclk_reg = (void __iomem *)io_p2v(AP_PWR_LCDCAXICLK_CTL),
	.divclk_bit = 0,
	.divclk_mask = 0x7,
	.divclk_we_bit = 16,
	.grclk_reg = (void __iomem *)io_p2v(AP_PWR_LCDCAXICLK_CTL),
	.grclk_bit = 4,
	.grclk_mask = 0xf,
	.grclk_we_bit = 20,
	.init = &sdiv_g16plus1_init,
	.enable = &clk_enable_generic,
	.disable = &clk_disable_generic,
	.set_rate = &sdiv_g16plus1_set_rate_noen,
};

static struct clk lcdc0_clk = {
	.name = "lcdc0_clk",
	.parent = &pll1_out,
	.mclk_reg = (void __iomem *)io_p2v(AP_PWR_LCDC0CLK_CTL),
	.mclk_bit = 8,
	.mclk_we_bit = 24,
	.divclk_reg = (void __iomem *)io_p2v(AP_PWR_LCDC0CLK_CTL),
	.divclk_bit = 4,
	.divclk_mask = 0xf,
	.divclk_we_bit = 20,
	.grclk_reg = (void __iomem *)io_p2v(AP_PWR_LCDC0CLK_CTL),
	.grclk_bit = 0,
	.grclk_mask = 0x7,
	.grclk_we_bit = 16,
	.ifclk_reg = (void __iomem *)io_p2v(AP_PWR_CLK_EN5),
	.ifclk_bit = 14,
	.ifclk_we_bit = 30,
	.init = &g8plus1_sdiv_init,
	.enable = &clk_enable_generic,
	.disable = &clk_disable_generic,
	.set_rate = &g8plus1_sdiv_set_rate,
};

static struct clk dsi_cfg_clk = {
	.name = "dsi_cfg_clk",
	.parent = &osc_clk,
	.mclk_reg = (void __iomem *)io_p2v(AP_PWR_DISCLK_CTL_EN),
	.mclk_bit = 0,
	.mclk_we_bit = 16,
	.ifclk_reg = (void __iomem *)io_p2v(AP_PWR_CTLPCLK_EN),
	.ifclk_bit = 9,
	.ifclk_we_bit = 25,
	.enable = &clk_enable_generic,
	.disable = &clk_disable_generic,
};

static struct clk dsi_ref_clk = {
	.name = "dsi_ref_clk",
	.parent = &osc_clk,
	.mclk_reg = (void __iomem *)io_p2v(AP_PWR_DISCLK_CTL_EN),
	.mclk_bit = 1,
	.mclk_we_bit = 17,
	.ifclk_reg = (void __iomem *)io_p2v(AP_PWR_CTLPCLK_EN),
	.ifclk_bit = 9,
	.ifclk_we_bit = 25,
	.enable = &clk_enable_generic,
	.disable = &clk_disable_generic,
};

static struct clk *ap_pwr_clks[] = {
	&osc_clk,
	&pll1_out,
	&lcdc_axi_clk,
	&lcdc0_clk,
	&dsi_cfg_clk,
	&dsi_ref_clk,
};

static void clk_init(void)
{
	struct clk *clk;
	int i;

	for (i = 0; i < ARRAY_SIZE(ap_pwr_clks); i++) {
		clk = ap_pwr_clks[i];
		if (clk->init)
			clk->init(clk);
	}
}

static struct clk *clk_get(const char *id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ap_pwr_clks); i++) {
		if (!strcmp(id, ap_pwr_clks[i]->name)) {
			return ap_pwr_clks[i];
		}
	}
	return NULL;
}

static int clk_set_rate(struct clk *clk, unsigned long rate)
{
	int ret = -EINVAL;

	if (clk == NULL || IS_ERR(clk) || (clk->flag & CLK_RATE_FIXED))
		return ret;

	if (clk->set_rate)
		ret = clk->set_rate(clk, rate);

	return ret;
}

static unsigned long clk_get_rate(struct clk *clk)
{
	unsigned long ret = 0;

	if (clk == NULL || IS_ERR(clk))
		return 0;

	ret = clk->rate;

	return ret;
}
static int clk_enable(struct clk *clk)
{
	int ret = 0;

	if (clk == NULL || IS_ERR(clk) || (clk->flag & CLK_ALWAYS_ENABLED))
		return -EINVAL;

	if (clk->enable)
		ret = clk->enable(clk);

	printf("%s enabled\n", clk->name);
	return ret;
}

static void clk_disable(struct clk *clk)
{
	if (clk == NULL || IS_ERR(clk) || (clk->flag & CLK_ALWAYS_ENABLED))
		return;

	if (clk->disable)
		clk->disable(clk);
}

/*************************************************************************************************/

static struct comipfb_info *comipfb_fbi = NULL;

const unsigned char comipfb_ids[] = {0};
#define PAGE_SIZE	(1 << 12)
static int comipfb_map_video_memory(struct comipfb_layer_info *layer)
{
	layer->map_size = ((layer->buf_size * layer->buf_num + PAGE_SIZE) >> 12) << 12;	//4k align
	layer->map_dma = (u32)gd->fb_base + (layer->map_size * layer->no);
	layer->map_cpu = (u_char*)layer->map_dma;
	return 0;
}

static void comipfb_enable_layer(struct comipfb_layer_info *layer)
{
	lcdc_layer_enable(layer, 1);
}

static void comipfb_config_layer(struct comipfb_layer_info *layer,
				unsigned int flag)
{
		lcdc_layer_config(layer, flag);
}

static void comipfb_init_layer(struct comipfb_layer_info *layer)
{
	struct comipfb_info *fbi = layer->parent;

	layer->win_x_size = fbi->display_info.xres;
	layer->win_y_size = fbi->display_info.yres;
	layer->src_width = fbi->display_info.xres;
	layer->buf_addr = layer->map_dma;
	comipfb_config_layer(layer, LCDC_LAYER_CFG_ALL);
}

static int comipfb_open(struct comipfb_layer_info *layer, int user)
{
	int ret = 0;

	if (layer->no > LAYER_MAX) {
		ret = -EBADF;
		goto err;
	}

	comipfb_init_layer(layer);
	if (layer->no == comipfb_ids[0])
		comipfb_enable_layer(layer);

	return 0;

err:
	return ret;
}


/*****************lcdc init************************/
static int comipfb_clk_enable(struct comipfb_info *fbi) {
	 int ret = 0;
	/*enable lcdc0 axi clock*/
	fbi->lcdc_axi_clk = clk_get( "lcdc_axi_clk");
	if (IS_ERR(fbi->lcdc_axi_clk)) {
		ret = PTR_ERR(fbi->lcdc_axi_clk);
		return ret;
	}
	if (fbi->pdata->lcdcaxi_clk)
		clk_set_rate(fbi->lcdc_axi_clk, fbi->pdata->lcdcaxi_clk);
	clk_enable(fbi->lcdc_axi_clk);

	fbi->dsi_refclk = clk_get( "dsi_ref_clk");
	if (IS_ERR(fbi->dsi_refclk)) {
		ret = PTR_ERR(fbi->dsi_refclk);
		return ret;
	}
	clk_enable(fbi->dsi_refclk);

	fbi->dsi_cfgclk = clk_get( "dsi_cfg_clk");
	if (IS_ERR(fbi->dsi_cfgclk)) {
		ret = PTR_ERR(fbi->dsi_cfgclk);
		return ret;
	}
	clk_enable(fbi->dsi_cfgclk);

	return ret;
}

static void comipfb_clk_disable(struct comipfb_info *fbi) {
	if (fbi->lcdc_axi_clk != NULL)
		clk_disable(fbi->lcdc_axi_clk);
	if (fbi->dsi_refclk != NULL)
		clk_disable(fbi->dsi_refclk);
	if (fbi->dsi_cfgclk != NULL)
		clk_disable(fbi->dsi_cfgclk);
}

static int comipfb_hw_release(struct comipfb_info *fbi)
{
	lcdc_exit(fbi);

	if (fbi->cdev->power)
		fbi->cdev->power(fbi, 0);
	else {
		if (fbi->pdata->power)
			fbi->pdata->power(0);
	}

	return 0;
}

static int comipfb_hw_init(struct comipfb_info *fbi)
{
	if (fbi->cdev->power)
		fbi->cdev->power(fbi, 1);
	else {
		if (fbi->pdata->power)
			fbi->pdata->power(1);
	}

	/* Initialize LCD controller */
	lcdc_init(fbi);

	return 0;
}

static void comipfb_layer_release(struct comipfb_layer_info *layer)
{
	return;
}

static int comipfb_layer_init(struct comipfb_layer_info *layer,
				struct comipfb_info *fbi)
{
	layer->win_x_size = fbi->display_info.xres;
	layer->win_y_size = fbi->display_info.yres;
	layer->src_width = fbi->display_info.xres;
	layer->alpha = 255; // 255 = solid 0 = transparent
	layer->alpha_en = 0;
	layer->input_format = LCDC_LAYER_INPUT_FORMAT_ARGB8888;//LCDC_LAYER_INPUT_FORMAT_RGB565;
	layer->buf_num = CONFIG_FB_MEMORY_NUM;

	if (fbi->bpp <= 16 ) {
		/* 8, 16 bpp */
		layer->buf_size = fbi->display_info.xres *
			fbi->display_info.yres * fbi->bpp / 8;
	} else {
		/* 18, 32 bpp*/
		layer->buf_size = fbi->display_info.xres * fbi->display_info.yres * 4;
	}

	if (comipfb_map_video_memory(layer)) {
		printf("Fail to allocate video RAM\n");
		return -ENOMEM;
	}

	layer->buf_addr = layer->map_dma;

	return 0;
}

static void comipfb_fbinfo_release(struct comipfb_info *fbi)
{
	struct comipfb_layer_info *layer;
	unsigned char i;

	for (i = 0; i < ARRAY_SIZE(comipfb_ids); i++) {
		layer = fbi->layers[comipfb_ids[i]];
		if (layer) {
			comipfb_layer_release(layer);
			free(layer);
		}
	}
}

static int comipfb_fbinfo_init(struct comipfb_info *fbi)
{
	struct comipfb_layer_info *layer;
	struct comipfb_dev *cdev;
	unsigned char i;

	cdev = comipfb_dev_get(fbi);
	if (!cdev) {
		printf( "cdev is not finded !!\n");
		return -EINVAL;
	}
	fbi->cdev = cdev;
	
	fbi->cif = comipfb_if_get(fbi);
	if (!fbi->cif) {
		printf( "dev interface is not finded !!\n");
		return -EINVAL;
	}
	
	fbi->bpp = cdev->bpp;
	fbi->pixclock = cdev->pclk;
	fbi->panel = 0;
	fbi->refresh_en = cdev->refresh_en; //TODO MAYBE RGB	
	clk_set_rate(fbi->clk, fbi->pixclock);
	fbi->pixclock = clk_get_rate(fbi->clk); 
	printf("fbi->pixclock = %d\n", fbi->pixclock);
	clk_enable(fbi->clk);

	switch(cdev->interface_info) {
		case COMIPFB_MIPI_IF:
			fbi->display_mode = cdev->timing.mipi.display_mode;
			if (fbi->display_mode == MIPI_VIDEO_MODE){
				fbi->refresh_en = 1;
				cdev->refresh_en = 1;
				fbi->display_info.name = cdev->name;
				fbi->display_info.xres = cdev->xres;
				fbi->display_info.yres = cdev->yres;
				fbi->display_info.pixclock = 1000000 / (fbi->pixclock / 1000000);
				fbi->display_info.sync = 0;
				fbi->display_info.left_margin = cdev->timing.mipi.videomode_info.hbp;
				fbi->display_info.right_margin = cdev->timing.mipi.videomode_info.hfp;
				fbi->display_info.upper_margin = cdev->timing.mipi.videomode_info.vbp;
				fbi->display_info.lower_margin = cdev->timing.mipi.videomode_info.vfp;
				fbi->display_info.hsync_len = cdev->timing.mipi.videomode_info.hsync;
				fbi->display_info.vsync_len = cdev->timing.mipi.videomode_info.vsync;
				if (cdev->timing.mipi.videomode_info.sync_pol == COMIPFB_HSYNC_HIGH_ACT)
					fbi->display_info.sync = FB_SYNC_HOR_HIGH_ACT;
				if (cdev->timing.mipi.videomode_info.sync_pol == COMIPFB_VSYNC_HIGH_ACT)
					fbi->display_info.sync = FB_SYNC_VERT_HIGH_ACT;
			}else if (fbi->display_mode == MIPI_COMMAND_MODE){
				fbi->display_info.name = cdev->name;
				fbi->display_info.xres = cdev->xres;
				fbi->display_info.yres = cdev->yres;
				fbi->display_info.pixclock = 1000000 / (fbi->pixclock / 1000000);
				fbi->display_info.left_margin = 0;
				fbi->display_info.right_margin = 0;
				fbi->display_info.upper_margin = 0;
				fbi->display_info.lower_margin = 0;
				fbi->display_info.hsync_len = 0;
				fbi->display_info.vsync_len = 0;
				fbi->display_info.sync = 0;
			}
		break;
		case COMIPFB_RGB_IF:
		break;
		default:
		break;
	}

	for (i = 0; i < ARRAY_SIZE(comipfb_ids); i++) {
		layer = calloc(1, sizeof(struct comipfb_layer_info));
		if (!layer)
			goto failed;

		layer->no = comipfb_ids[i];
		layer->parent = fbi;
		if (comipfb_layer_init(layer, fbi) < 0)
			goto failed;
		fbi->layers[layer->no] = layer;
	}

	return 0;

failed:
	comipfb_fbinfo_release(fbi);

	return -EINVAL;
}

static int comipfb_read_image(struct comipfb_layer_info *layer)
{
	layer->buf_addr = layer->map_dma;

	comipfb_config_layer(layer, LCDC_LAYER_CFG_BUF);

	return 0;
}

static void comipfb_show_logo(struct comipfb_info *fbi)
{
	struct comipfb_layer_info *layer = fbi->layers[comipfb_ids[0]];
	int ret;

	comipfb_fbi = fbi;
	ret = comipfb_read_image(layer);
	if (ret == 0) {
		/* Get logo data. */
		comipfb_open(layer, 0);
	}

	if (fbi->display_mode == MIPI_COMMAND_MODE) {
		lcdc_start(fbi, 1);
	}

}


int comipfb_probe(void)
{
	struct comipfb_info *fbi = NULL;
	struct comipfb_platform_data *prdata;
	struct resource *r, *r1;
	int ret;
	char clk_name[12];

	printf("comipfb_probe\n");

	prdata = &comip_lcd_info;

	fbi = calloc(1, sizeof(struct comipfb_info));
	if (!fbi)
		return -EINVAL;

	r = &comip_resource_fb0[0];
	fbi->res = r;

	r1 = &comip_resource_fb0[1];
	if (r1 != NULL)
		fbi->interface_res = r1;
	
	if (!r) {
		printf("Invalid platform data.\n");
		free(fbi);
		return -ENXIO;
	}

	strcpy(clk_name, "lcdc0_clk");	
	fbi->clk = clk_get(clk_name);
	if (IS_ERR(fbi->clk)) {
		ret = PTR_ERR(fbi->clk);
		goto failed;
	}

	fbi->pdata = prdata;

	display_powerup(fbi);
	clk_init();
	/*bring forward the clk enable operation*/
	if (!prdata->detect_dev) {	//recognition with ID Read
		ret = comipfb_clk_enable(fbi);
		if (ret)
			goto failed_enable_clk;
		/*NOTICE: in lightweight init, hs_freq is fixed to 39000Kbytes/s, escape division is fixed to 3*/
		ret = lcdc_init_lw(fbi);
		if (ret < 0) {
			printf("Failed to initialize mipi lowlevel\n");
			goto failed_enable_clk;
		}
	}

	ret = comipfb_fbinfo_init(fbi);
	if (ret < 0) {
		printf("Failed to initialize framebuffer device\n");
		goto failed_enable_clk;
	}

	if (!fbi->cdev ||
		!fbi->cif ||
		!fbi->cif->init ||
		!fbi->cif->exit) {
		printf("Invalid interface or device\n");
		ret = -EINVAL;
		goto failed_check_val;
	}

	if (!fbi->display_info.xres ||
		!fbi->display_info.yres ||
		!fbi->pixclock||
		!fbi->bpp) {
		printf("Invalid resolution or bit depth\n");
		ret = -EINVAL;
		goto failed_check_val;
	}

	printf("got a %dx%dx%d LCD(%s)\n",
		fbi->display_info.xres, fbi->display_info.yres, fbi->bpp, fbi->display_info.name);

	if (prdata->detect_dev) {
		ret = comipfb_clk_enable(fbi);
		if (ret)
			goto failed_check_val;
	}

	comipfb_hw_init(fbi);
	
	comipfb_show_logo(fbi);
	msleep(100);

	/*config mux, backlight on*/
	comip_mfp_config(LCD_BACKLIGHT_PIN, MFP_PIN_MODE_GPIO);

	gpio_direction_output(LCD_BACKLIGHT_PIN, 1);

	printf( "*lcd init finish*\n");

	return 0;


failed_check_val:
	comipfb_hw_release(fbi);
	comipfb_fbinfo_release(fbi);
failed_enable_clk:
	comipfb_clk_disable(fbi);
	if (fbi->clk != NULL) {
		clk_disable(fbi->clk);
	}
failed:
	free(fbi);
	return ret;
}

