/* driver/video/comip/comipfb_dev.c
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

#include <linux/list.h>
#include "comipfb.h"
#include "comipfb_dev.h"


struct comipfb_dev_info {
	struct comipfb_dev* dev;
	struct list_head list;
};

static LIST_HEAD(comipfb_dev_list);

int comipfb_dev_register(struct comipfb_dev* dev)
{
	struct comipfb_dev_info *info;

	info = calloc(1, sizeof(struct comipfb_dev_info));
	if (!info)
		return -ENOMEM;

	INIT_LIST_HEAD(&info->list);
	info->dev = dev;

	list_add_tail(&info->list, &comipfb_dev_list);

	return 0;
}

int comipfb_dev_unregister(struct comipfb_dev* dev)
{
	struct comipfb_dev_info *info;


	list_for_each_entry(info, &comipfb_dev_list, list) {
		if (info->dev == dev) {
			list_del_init(&info->list);
			free(info);
		}
	}

	return 0;
}

struct comipfb_dev* comipfb_dev_get(struct comipfb_info *fbi)
{
	struct comipfb_dev_info *info;
	struct comipfb_dev *dev = NULL;
	struct comipfb_dev *dev_t = NULL;
	int val = -1;

	if (fbi->pdata->detect_dev) {
		val = fbi->pdata->detect_dev();
		if (val < 0)
			printf( "Warning: Detect lcd device failed\n");
		else
			printf( "Lcd device detect val = %d\n", val);
	}

	list_for_each_entry(info, &comipfb_dev_list, list) {
		if ((fbi->pdata->lcdc_support_interface & info->dev->interface_info) > 0) {
			if ((fbi->pdata->detect_dev == NULL) && (comipfb_read_lcm_id(fbi, info->dev) == 0)) {
				dev = info->dev;
				break;
			}
			if (val == info->dev->lcd_id)
				dev_t = info->dev;
		}
	}

	if (!dev) {
		if (dev_t)
			dev = dev_t;
		else {
			info = list_first_entry(&comipfb_dev_list, typeof(*info), list);
			dev = info->dev;
		}
	}

	return dev;
}

