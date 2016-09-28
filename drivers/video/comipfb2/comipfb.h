/* driver/video/comip/comipfb.h
**
** Use of source code is subject to the terms of the LeadCore license agreement under
** which you licensed source code. If you did not accept the terms of the license agreement,
** you are not authorized to use source code. For the terms of the license, please see the
** license agreement between you and LeadCore.
**
** Copyright (c) 1999-2008  LeadCoreTech Corp.
**
**  PURPOSE:   		This files contains the driver of LCD controller.
**
**  CHANGE HISTORY:
**
**	Version		Date		Author			Description
**	0.1.0		2009-03-10	liuchangmin		created
**	0.1.1		2011-03-10	zouqiao	updated
**
*/

#ifndef __COMIPFB_H__
#define __COMIPFB_H__

#include <linux/types.h>
#include <asm/atomic.h>
#include "comipfb_if.h"
#include "comipfb_dev.h"
#include "common.h"

#define COMIP_NAME		"COMIP"
#define io_p2v(x)		(x)
/*
 * abs() handles unsigned and signed longs, ints, shorts and chars.  For all
 * input types abs() returns a signed long.
 * abs() should not be used for 64-bit types (s64, u64, long long) - use abs64()
 * for those.
 */
#define abs(x) ({						\
		long ret;					\
		if (sizeof(x) == sizeof(long)) {		\
			long __x = (x);				\
			ret = (__x < 0) ? -__x : __x;		\
		} else {					\
			int __x = (x);				\
			ret = (__x < 0) ? -__x : __x;		\
		}						\
		ret;						\
	})

/******************************************************************************************/
/*platform related*/

#define COMIPFB_RGB_IF                  (0x00000002)
#define COMIPFB_MIPI_IF                 (0x00000003)
#define COMIPFB_REFRESH_THREAD				(0x00000001)
#define COMIPFB_REFRESH_SINGLE				(0x00000002)
#define COMIPFB_CACHED_BUFFER				(0x00000004)
#define COMIPFB_SLEEP_POWEROFF				(0x00010000)

#define COMIPFB_REFRESH_SOFTWARE			\
	(COMIPFB_REFRESH_THREAD | COMIPFB_REFRESH_SINGLE)

enum lcd_id {
	LCD_ID_HS_RM68190 = 1,
	LCD_ID_HS_NT35517,
	LCD_ID_TRULY_NT35595,
	LCD_ID_BT_ILI9806E,
	LCD_ID_DJN_ILI9806E,
	LCD_ID_SHARP_R69431,
	LCD_ID_AUO_NT35521,
	LCD_ID_INX_H8394D,
	LCD_ID_INX_H8392B,
	LCD_ID_AUO_H8399A,
	LCD_ID_IVO_OTM1283A,
	LCD_ID_TRULY_H8394A,
	LCD_ID_JD_9161CPT5,
	LCD_ID_JD_9261AA,
	LCD_ID_JD_9365HSD,
	LCD_ID_AUO_R61308OTP,
	LCD_ID_AUO_OTM1285A_OTP,
	LCD_ID_HT_H8394D,
};

#if defined(CONFIG_FB_COMIP2)
struct comipfb_platform_data {
	int lcdc_support_interface; //lcdc support interface.
	int phy_ref_freq;
	int lcdcaxi_clk;
	int gpio_rst;
	int gpio_im;
	int flags;
	int (*power)(int onoff);
	int (*detect_dev)(void);
	void (*bl_control)(int onoff);
};

#elif defined(CONFIG_FB_COMIP)
struct comipfb_platform_data {
	struct comipfb_spi_info spi_info;
	int lcdc_id;	//controllor and (screen or other dev) config id.
	int main_lcdc_flag; //main controllor flag.  default  0.
	int lcdc_support_interface; //lcdc support interface.
	int lcdcaxi_clk;
	int gpio_rst;
	int gpio_im;
	int flags;
	int (*power)(int onoff);
	const char* (*detect_dev)(void);
	void (*bl_control)(int onoff);
};
#else
struct comipfb_platform_data {
	int flags;
};
#endif


/******************************************************************************************/
/*resource*/
typedef phys_addr_t resource_size_t;

#define IORESOURCE_MEM		0x00000200
#define IORESOURCE_IRQ		0x00000400

struct resource {
	resource_size_t start;
	resource_size_t end;
	const char *name;
	unsigned long flags;
};

/******************************************************************************************/

#define FB_SYNC_HOR_HIGH_ACT	1	/* horizontal sync high active	*/
#define FB_SYNC_VERT_HIGH_ACT	2	/* vertical sync high active	*/

struct fb_videomode {
	const char *name;	/* optional */
	u32 refresh;		/* optional */
	u32 xres;
	u32 yres;
	u32 pixclock;
	u32 left_margin;
	u32 right_margin;
	u32 upper_margin;
	u32 lower_margin;
	u32 hsync_len;
	u32 vsync_len;
	u32 sync;
	u32 vmode;
	u32 flag;
};


/******************************************************************************************/
/*layer info*/
#define LCDC_LAYER_INPUT_FORMAT_RGB565		0x1
#define LCDC_LAYER_INPUT_FORMAT_RGB666		0x2
#define LCDC_LAYER_INPUT_FORMAT_RGB888		0x3
#define LCDC_LAYER_INPUT_FORMAT_YUV422		0x5
#define LCDC_LAYER_INPUT_FORMAT_ARGB8888	0x6
#define LCDC_LAYER_INPUT_FORMAT_ABGR8888	0x7
#define LCDC_LAYER_INPUT_FORMAT_YUV420SP	0x8

enum {
	LAYER0 = 0,
	LAYER1,
	LAYER2,
	LAYER_MAX,
};

struct comipfb_layer_info
{
	/* Must place fb as the first member.
	 * We should use fb_info.par instead */

	unsigned char		no;
	struct comipfb_info	*parent;

	/*
	 * These are the addresses we mapped
	 * the framebuffer memory region to.
	 */
	/* raw memory addresses */
	dma_addr_t		map_dma;	/* physical */
	u_char*			map_cpu;	/* virtual */
	unsigned int		map_size;

	/*
	 * Hardware control information
	 */
	unsigned char	yuv2rgb_en;
	unsigned char	keyc_en;
	unsigned long	key_color;
	unsigned char	alpha_en;
	unsigned char	alpha;
	unsigned char	byte_ex_en;
	unsigned short	input_format;
	unsigned short	win_x_size;
	unsigned short	win_y_size;
	unsigned short	win_x_offset;
	unsigned short	win_y_offset;
	unsigned short	src_width;	/* source image width in pixel */
	unsigned int	buf_addr;	/* current buffer physical address. */
	unsigned int	buf_size;	/* frame buffer size. */
	unsigned int	buf_num;	/* frame buffer number. */

};

/******************************************************************************************/
/*contain all resource that lcd driver need*/
struct comipfb_info {
	struct clk *clk;
	struct clk *dsi_cfgclk;
	struct clk *dsi_refclk;
	struct clk *lcdc_axi_clk;
	struct resource *res;
	struct resource *interface_res;

	/*
	 * Hardware control information
	 */
	struct fb_videomode display_info;    /* reparent video mode source*/
	unsigned char		panel;
	unsigned int		bpp;
	unsigned int		refresh_en;
	unsigned int		display_mode;
	unsigned int		pixclock;

	/* Layer contorl */
	struct comipfb_layer_info *layers[LAYER_MAX];

	/* Platform data. */
	struct comipfb_if	*cif;
	struct comipfb_dev	*cdev;
	struct comipfb_platform_data *pdata;

};

#endif/*__COMIPFB_H__*/
