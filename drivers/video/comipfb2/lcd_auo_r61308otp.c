#include "comipfb.h"
#include "comipfb_dev.h"

static u8 lcd_cmds_init[][ROW_LINE] = {
	{0x78, DCS_CMD, SW_PACK1, 0x01, 0x11, },
	{0x00, DCS_CMD, LW_PACK, 0x04, 0x02, 0x00, 0x36, 0x00, },
	{0x00, DCS_CMD, LW_PACK, 0x04, 0x02, 0x00, 0x3A, 0x70, },
	{0x00, GEN_CMD, LW_PACK, 0x04, 0x02, 0x00, 0xB0, 0x04, },
	{0x00, GEN_CMD, LW_PACK, 0x0a, 0x08, 0x00, 0xC1, 0x50, 0x02, 0x22, 0x00, 0x00, 0xED, 0x11, },
	{0x00, GEN_CMD, LW_PACK, 0x1b, 0x19, 0x00, 0xC8, 0x1A, 0x24, 0x29, 0x2D, 0x32, 0x37, 0x14, 0x13, 0x10, 0x0C, 0x0A, 0x06, 0x1A, 0x24, 0x28, 0x2D, 0x32, 0x37, 0x14, 0x13, 0x10, 0x0C, 0x0A, 0x06, },
	{0x00, GEN_CMD, LW_PACK, 0x0b, 0x09, 0x00, 0xCB, 0x10, 0x20, 0x40, 0x80, 0xA0, 0xC0, 0xD0, 0xE0, },
	{0x00, GEN_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xCC, 0xC8, 0xD8, 0xFF, },
	{0x00, GEN_CMD, LW_PACK, 0x0a, 0x08, 0x00, 0xCD, 0x1C, 0x1E, 0x1E, 0x1D, 0x1C, 0x1E, 0x1E, },
	{0x00, GEN_CMD, LW_PACK, 0x0a, 0x08, 0x00, 0xCE, 0x1E, 0x1E, 0x1E, 0x1D, 0x1D, 0x1E, 0x1E, },
	{0x00, GEN_CMD, LW_PACK, 0x0a, 0x08, 0x00, 0xCF, 0x1E, 0x1F, 0x20, 0x20, 0x20, 0x20, 0x21, },
	{0x00, GEN_CMD, LW_PACK, 0x04, 0x02, 0x00, 0xB0, 0x03, },
	{0x28, DCS_CMD, SW_PACK1, 0x01, 0x29, },
};

static u8 lcd_pre_read_id[][ROW_LINE] = {
	{0x00, GEN_CMD, SW_PACK2, 0x02, 0xB0, 0x04},
};
static struct common_id_info lcd_common_id_info[] = {
	{GEN_CMD, {0x01, 0x22, 0x13, 0x08, 0x00} , 5 , 0xBF},
};

static int lcd_auo_r61308opt_power(struct comipfb_info *fbi, int onoff)
{
	int gpio_rst = fbi->pdata->gpio_rst;

	if (gpio_rst < 0) {
	       printf("no reset pin found\n");
	       return -ENXIO;
	}

	gpio_request(gpio_rst, "LCD Reset");

	/*10 10 20ms*/
	if (onoff) {
		pmic_lcdio_enable(1);
		pmic_lcdcore_enable(1);
		gpio_direction_output(gpio_rst, 1);
		mdelay(20);
		gpio_direction_output(gpio_rst, 0);
		mdelay(20);
		gpio_direction_output(gpio_rst, 1);
		mdelay(40);
	} else {
		gpio_direction_output(gpio_rst, 0);
		pmic_lcdcore_enable(0);
		pmic_lcdio_enable(0);
		mdelay(50);
	}

	gpio_free(gpio_rst);

	return 0;
}

struct comipfb_dev lcd_auo_r61308opt_dev = {
	.name = "lcd_auo_r61308opt",
	.interface_info = COMIPFB_MIPI_IF,
	.lcd_id = LCD_ID_AUO_R61308OTP,
	.refresh_en = 1,
	.bpp = 32,
	.xres = 720,
	.yres = 1280,
	.flags = 0,
	.pclk = 65000000,
	.timing = {
		.mipi = {
			.hs_freq = 65000,		//Kbyte
			.lp_freq = 13000,		//KHZ
			.no_lanes = 3,
			.display_mode = MIPI_VIDEO_MODE,
			.color_mode = {
				.color_bits = COLOR_CODE_24BIT,
			},
			.videomode_info = {
				.hsync = 10,
				.hbp = 40,
				.hfp = 80,
				.vsync = 1,
				.vbp = 12,
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
				.te_cps = 64,
			},
			.ext_info = {
				.eotp_tx_en = 0,
			},
		},
	},
	.panel_id_info = {
		.id_info = lcd_common_id_info,
		.num_id_info = 1,
		.prepare_cmd = {ARRAY_AND_SIZE(lcd_pre_read_id)},
	},
	.cmds_init = {ARRAY_AND_SIZE(lcd_cmds_init)},

	.power = lcd_auo_r61308opt_power,
};

int lcd_auo_r61308opt_init(void)
{
	return comipfb_dev_register(&lcd_auo_r61308opt_dev);
}

