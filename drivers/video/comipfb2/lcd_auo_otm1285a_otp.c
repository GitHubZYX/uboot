#include "comipfb.h"
#include "comipfb_dev.h"

static u8 lcd_cmds_init[][ROW_LINE] = {
	{0x00, GEN_CMD, LW_PACK, 0x04, 0x02, 0x00, 0x00, 0x00, },
	{0xC8, GEN_CMD, LW_PACK, 0x03, 0x01, 0x00, 0x11, },
	{0x00, GEN_CMD, LW_PACK, 0x04, 0x02, 0x00, 0x00, 0x00, },
	{0xC8, GEN_CMD, LW_PACK, 0x03, 0x01, 0x00, 0x29, },

};

static struct common_id_info lcd_common_id_info[] = {
	{ DCS_CMD, {0x85} , 1 , 0xDA},
	{ DCS_CMD, {0x38} , 1 , 0xDB},
};

static int lcd_auo_otm1285a_otp_power(struct comipfb_info *fbi, int onoff)
{
	int gpio_rst = fbi->pdata->gpio_rst;

	if (gpio_rst < 0) {
	       printf("no reset pin found\n");
	       return -ENXIO;
	}

	gpio_request(gpio_rst, "LCD Reset");

	/*10 10 20ms*/
	if (onoff) {
		gpio_direction_output(gpio_rst, 0);
		pmic_lcdio_enable(1);
		mdelay(10);
		pmic_lcdcore_enable(1);
		gpio_direction_output(gpio_rst, 1);
		mdelay(5);
		gpio_direction_output(gpio_rst, 0);
		mdelay(10);
		gpio_direction_output(gpio_rst, 1);
		mdelay(50);
	} else {
		gpio_direction_output(gpio_rst, 0);
		mdelay(20);
		pmic_lcdcore_enable(0);
		pmic_lcdio_enable(0);
		mdelay(50);
	}

	gpio_free(gpio_rst);

	return 0;
}

struct comipfb_dev lcd_auo_otm1285a_otp_dev = {
	.name = "lcd_auo_otm1285a",
	.interface_info = COMIPFB_MIPI_IF,
	.lcd_id = LCD_ID_AUO_OTM1285A_OTP,
	.refresh_en = 1,
	.bpp = 32,
	.xres = 720,
	.yres = 1280,
	.flags = 0,
	.pclk = 65000000,
	.timing = {
		.mipi = {
			.hs_freq = 65000,		//Kbyte
			.lp_freq = 16250,		//KHZ
			.no_lanes = 3,
			.display_mode = MIPI_VIDEO_MODE,
			.color_mode = {
				.color_bits = COLOR_CODE_24BIT,
			},
			.videomode_info = {
				.hsync = 2,
				.hbp = 44,
				.hfp = 44,
				.vsync = 1,
				.vbp = 30,
				.vfp = 30,
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
		.num_id_info = 2,
	},
	.cmds_init = {ARRAY_AND_SIZE(lcd_cmds_init)},
	.power = lcd_auo_otm1285a_otp_power,
};

int lcd_auo_otm1285a_otp_init(void)
{
	return comipfb_dev_register(&lcd_auo_otm1285a_otp_dev);
}

