/* driver/video/comip/comipfb_if.c
**
** Use of source code is subject to the terms of the LeadCore license agreement under
** which you licensed source code. If you did not accept the terms of the license agreement,
** you are not authorized to use source code. For the terms of the license, please see the
** license agreement between you and LeadCore.
**
** Copyright (c) 1999-2015      LeadCoreTech Corp.
**
**      PURPOSE:                        This files contains the driver of LCD controller.
**
**      CHANGE HISTORY:
**
**      Version         Date            Author          Description
**      0.1.0           2012-03-10      liuyong         created
**
*/

#include <linux/compiler.h>
#include "comipfb.h"
#include "comipfb_if.h"
#include "mipi_cmd.h"
#include "common.h"

static struct comipfb_info *fs_fbi;

/*
 *
 * MIPI interface
 */
static int comipfb_mipi_mode_change(struct comipfb_info *fbi)
{
	int gpio_im, gpio_val;
	struct comipfb_dev_timing_mipi *mipi;

	mipi = &(fbi->cdev->timing.mipi);
	
	if (fbi != NULL) {
		gpio_im = fbi->pdata->gpio_im;
		gpio_val = mipi->im_pin_val;
		if (gpio_im >= 0) {
			gpio_request(gpio_im, "LCD IM");
			gpio_direction_output(gpio_im, gpio_val);
		}
	}
	return 0;
}

int comipfb_if_mipi_dev_cmds(struct comipfb_info *fbi, struct comipfb_dev_cmds *cmds)
{
	int ret = -1;
	int loop = 0;
	unsigned char *p;

	if (!cmds)
		return 0;
	if (!cmds->n_pack || !cmds->cmds)
		return 0;
	for (loop = 0; loop < cmds->n_pack; loop++) {
		p = cmds->cmds + loop * ROW_LINE;
		if (p[1] == DCS_CMD)
			ret = mipi_dsih_dcs_wr_cmd(fbi, 0, &(p[2]), (u16)p[3]);
		else if (p[1] == GEN_CMD)
			ret = mipi_dsih_gen_wr_cmd(fbi, 0, &(p[2]), (u16)p[3]);
		if (ret < 0) {
			printf("*MIPI send command failed !!*\n");
			return ret;
		}
		if (p[0] > 0) {
#ifdef CONFIG_FBCON_DRAW_PANIC_TEXT
			if (unlikely(kpanic_in_progress == 1)) {
				if (p[0] == 0xff)
					mdelay(200);
				else
					mdelay(p[0]);
			}
			else {

				if (p[0] == 0xff)
					msleep(200);
				else
					msleep(p[0]);
			}
#else
			if (p[0] == 0xff)
				msleep(200);
			else
				msleep(p[0]);
#endif

		}
	}
	return ret;
}

void comipfb_if_mipi_reset(struct comipfb_info *fbi)
{
	struct comipfb_dev_timing_mipi *mipi;
	int stop_status = 0;
	int check_times = 3;
	int i = 0;

	mipi = &(fbi->cdev->timing.mipi);

	switch (mipi->no_lanes) {
		case 1:
			stop_status = 0x10;
			break;
		case 2:
			stop_status = 0x90;
			break;
		case 3:
			stop_status = 0x290;
			break;
		case 4:
			stop_status = 0xa90;
			break;
		default:
			break;
	}

	for (i = 0; i < check_times; i++) {
		if ((mipi_dsih_dphy_get_status(fbi) & stop_status) == stop_status)
			break;
		udelay(5);
	}

	mipi_dsih_hal_power(fbi, 0);
	mipi_dsih_hal_power(fbi, 1);
}

static int comipfb_if_mipi_init(struct comipfb_info *fbi)
{
	int ret = 0;
	struct comipfb_dev_timing_mipi *mipi;

	fs_fbi = fbi;

	mipi = &(fbi->cdev->timing.mipi);

	comipfb_mipi_mode_change(fbi);

	mipi_dsih_dphy_enable_hs_clk(fbi, 1);
	msleep(5);

	if (mipi->display_mode == MIPI_VIDEO_MODE)
		mipi_dsih_hal_mode_config(fbi, 1);

	mipi_dsih_dphy_enable_hs_clk(fbi, 0);
	/* Reset device. */
	if (!(fbi->cdev->power)) {
		if (fbi->cdev->reset)
			fbi->cdev->reset(fbi);
	}
	ret = comipfb_if_mipi_dev_cmds(fbi, &fbi->cdev->cmds_init);
	msleep(10);
	if (mipi->display_mode == MIPI_VIDEO_MODE) {
		mipi_dsih_hal_mode_config(fbi, 0);
	}else
		mipi_dsih_hal_dcs_wr_tx_type(fbi, 3, 0);

	msleep(5);
	mipi_dsih_dphy_enable_hs_clk(fbi, 1);

	return ret;
}

static int comipfb_if_mipi_exit(struct comipfb_info *fbi)
{
	int gpio_im = fbi->pdata->gpio_im;
	if (gpio_im >= 0)
		gpio_free(gpio_im);

	return 0;
}

static void comipfb_if_mipi_bl_change(struct comipfb_info *fbi, int val)
{
	unsigned int bit;
	struct comipfb_dev_cmds *lcd_backlight_cmd;

	if (fbi == NULL) {
		printf("%s ,fbi is NULL", __func__);
		return ;
	}

	bit = fbi->cdev->backlight_info.brightness_bit;
 	lcd_backlight_cmd = &(fbi->cdev->backlight_info.bl_cmd);
	lcd_backlight_cmd->cmds[bit] = (unsigned char)val;
	comipfb_if_mipi_dev_cmds(fbi, lcd_backlight_cmd);
}

static unsigned char te_cmds[][10] = {
/****TE command***/
        {0x00, DCS_CMD, SW_PACK2, 0x02, 0x35, 0x00},
};
static void comipfb_if_mipi_te_trigger(struct comipfb_info *fbi)
{
	struct comipfb_dev_cmds te_pkt;

	te_pkt.cmds = (unsigned char *)te_cmds;
	te_pkt.n_pack = 1;

	comipfb_if_mipi_dev_cmds(fbi, &te_pkt);
}

static int comipfb_if_mipi_read_lcm_id(struct comipfb_info *fbi , struct comipfb_id_info *cur_id_info)
{
	unsigned char rd_cnt=0,lp_cnt=0;
	unsigned char cmd;
	unsigned char *id_val;
	u8 lcm_id[8] = {0};
	int i, ret = 0;

	/*ready to read id*/
	if(cur_id_info->prepare_cmd.cmds)
		comipfb_if_mipi_dev_cmds(fbi, &(cur_id_info->prepare_cmd));

	while (lp_cnt < cur_id_info->num_id_info) {
		cmd = cur_id_info->id_info[lp_cnt].cmd;
		rd_cnt = cur_id_info->id_info[lp_cnt].id_count;
		id_val = cur_id_info->id_info[lp_cnt].id;

		mipi_dsih_gen_wr_packet(fbi, 0, 0x37, &rd_cnt, 1);
		if (cur_id_info->id_info[lp_cnt].pack_type == DCS_CMD) {
			ret = mipi_dsih_dcs_rd_cmd(fbi, 0, cmd, rd_cnt, lcm_id);
		} else if (cur_id_info->id_info[lp_cnt].pack_type == GEN_CMD) {
			ret = mipi_dsih_gen_rd_cmd(fbi, 0, &cmd, 1, rd_cnt, lcm_id);
		}
		ret = strncmp((char *)lcm_id, (char *)id_val, rd_cnt);
		if (ret) {
			printf("request:");
			for (i = 0; i< rd_cnt; i++)
				printf("0x%x,", id_val[i]);
			printf(" read:");
			for (i = 0; i< rd_cnt; i++)
				printf("0x%x,", lcm_id[i]);
			return -1;
		}
		lp_cnt++;
	}
	return 0;
}

int comipfb_read_lcm_id(struct comipfb_info *fbi, void *dev)
{
	static int check_result, common_pwup = 0;
	struct comipfb_dev *lcm_dev = (struct comipfb_dev *)dev;

	fbi->cdev = lcm_dev;

	/*power on LCM and reset*/
	if (fbi->cdev->power)
		fbi->cdev->power(fbi, 1);
	else {
		if ((!common_pwup) && fbi->pdata->power) {
			fbi->pdata->power(1);
			common_pwup = 1;
		}
		if (fbi->cdev->reset)
			fbi->cdev->reset(fbi);
	}
	check_result = comipfb_if_mipi_read_lcm_id(fbi, &(lcm_dev->panel_id_info));
	if ( check_result ){
		if (fbi->cdev->power)
			fbi->cdev->power(fbi, 0);
		printf("read lcm id failed \n");
		return -1;
	} else {
		return 0;
	}
}


static struct comipfb_if comipfb_if_mipi = {
        .init           = comipfb_if_mipi_init,
        .exit           = comipfb_if_mipi_exit,
        .dev_cmd        = comipfb_if_mipi_dev_cmds,
		.bl_change	= comipfb_if_mipi_bl_change,
		.te_trigger = comipfb_if_mipi_te_trigger,
};

struct comipfb_if* comipfb_if_get(struct comipfb_info *fbi)
{

	switch (fbi->cdev->interface_info) {
		case COMIPFB_MIPI_IF:
			return &comipfb_if_mipi;
		default:
			return NULL;
	}
}


