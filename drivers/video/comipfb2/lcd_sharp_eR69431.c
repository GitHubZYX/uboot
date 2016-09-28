#include "comipfb.h"
#include "comipfb_dev.h"

static u8 lcd_cmds_init[][ROW_LINE] = {
	{0x00, GEN_CMD, SW_PACK2, 0x02, 0xB0, 0x00},
	{0x00, GEN_CMD, LW_PACK, 0x08, 0x06, 0x00, 0xB3, 0x1C, 0x00, 0x00, 0x00, 0x00},
	{0x00, GEN_CMD, SW_PACK2, 0x02, 0xD6, 0x01},
	{0x00, GEN_CMD, SW_PACK2, 0x02, 0xB0, 0x03},
	{0x00, DCS_CMD, SW_PACK1, 0x01, 0x29},
	{0x8C, DCS_CMD, SW_PACK1, 0x01, 0x11},
};
static struct common_id_info lcd_common_id_info[] = {
	{ DCS_CMD, {0x94, 0x31}, 2, 0x04},
};
static int lcd_sharp_eR69431_power(struct comipfb_info *fbi, int onoff)
{
	int gpio_rst = fbi->pdata->gpio_rst;

	if (gpio_rst < 0) {
		printf("no reset pin found\n");
		return -ENXIO;
	}

	gpio_request(gpio_rst, "LCD Reset");

	if (onoff) {
		gpio_direction_output(gpio_rst, 0);
		pmic_lcdio_enable(1);
		mdelay(1);
		pmic_lcdcore_enable(1);
		mdelay(50);
		gpio_direction_output(gpio_rst, 1);
		mdelay(50);
	} else {
		pmic_lcdcore_enable(0);
		mdelay(10);
		gpio_direction_output(gpio_rst, 0);
		mdelay(10);
		pmic_lcdio_enable(0);
		mdelay(10);
	}

	gpio_free(gpio_rst);

	return 0;
}

static int lcd_sharp_eR69431_reset(struct comipfb_info *fbi)
{
	int gpio_rst = fbi->pdata->gpio_rst;

	if (gpio_rst >= 0) {
		gpio_request(gpio_rst, "LCD Reset");
		gpio_direction_output(gpio_rst, 1);
		mdelay(10);
		gpio_direction_output(gpio_rst, 0);
		mdelay(20);
		gpio_direction_output(gpio_rst, 1);
		mdelay(100);
		gpio_free(gpio_rst);
	}
	return 0;
}

struct comipfb_dev lcd_sharp_eR69431_dev = {
	.name = "lcd_sharp_eR69431",
	.interface_info = COMIPFB_MIPI_IF,
	.lcd_id = LCD_ID_SHARP_R69431,
	.refresh_en = 1,
	.bpp = 32,
	.xres = 720,
	.yres = 1280,
	.flags = 0,
	.pclk = 65000000,
	.timing = {
		.mipi = {
			.hs_freq = 65000,//71500,		//Kbyte
			.lp_freq = 13000,//14300,		//KHZ
			.no_lanes = 3,
			.display_mode = MIPI_VIDEO_MODE,
			.im_pin_val = 1,
			.color_mode = {
				.color_bits = COLOR_CODE_24BIT,
			},
			.videomode_info = {
				.hsync = 2,
				.hbp = 28,
				.hfp = 100,
				.vsync = 1,
				.vbp = 10,
				.vfp = 8,
				.sync_pol = COMIPFB_VSYNC_HIGH_ACT,
				.lp_cmd_en = 1,
				.lp_hfp_en = 1,
				.lp_hbp_en = 1,
				.lp_vact_en = 1,
				.lp_vfp_en = 1,
				.lp_vbp_en = 1,
				.lp_vsa_en = 1,
				.mipi_trans_type = VIDEO_BURST_WITH_SYNC_PULSES,
			},
			.phytime_info = {
				.clk_tprepare = 3, //HSBYTECLK
			},
			.teinfo = {
				.te_source = 1, //external signal
				.te_trigger_mode = 0,
				.te_en = 0,
				.te_sync_en = 0,
			},
			.ext_info = {
				.eotp_tx_en = 1,
			},
		},
	},
	.panel_id_info = {
		.id_info = lcd_common_id_info,
		.num_id_info = 1,
	},
	.cmds_init = {ARRAY_AND_SIZE(lcd_cmds_init)},
	.power = lcd_sharp_eR69431_power,
	.reset = lcd_sharp_eR69431_reset,
};

int lcd_sharp_eR69431_init(void)
{
	return comipfb_dev_register(&lcd_sharp_eR69431_dev);
}

