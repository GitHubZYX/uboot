/*
 * (C) Copyright Leadcore
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

#define AW2013_I2C_ID		(COM_I2C)
#define AW2013_I2C_ADDR		(0x45)

/* register map */
#define LEDGCR			0x01
#define LEDSEL			0x30
#define LED0CTRL 		0x31
#define LED1CTRL 		0x32
#define LED2CTRL 		0x33
#define LED0LEVEL		0x34
#define LED1LEVEL		0x35
#define LED2LEVEL		0x36
#define LED0T0			0x37
#define LED0T1			0x38
#define LED0T2			0x39
#define LED1T0			0x3A
#define LED1T1			0x3B
#define LED1T2			0x3C
#define LED2T0			0x3D
#define LED2T1			0x3E
#define LED2T2			0x3F

/*
 *	Red->0, Green->1, Blue->2.
 */
#define LED_SWITCH(index)	(0x1 << index)
#define LED_CTRL(index)		(LED0CTRL + index)
#define LED_LEVEL(index)	(LED0LEVEL + index)
#define LED_T0(index)		(LED0T0 + index * 3)
#define LED_T1(index)		(LED0T1 + index * 3)
#define LED_T2(index)		(LED0T2 + index * 3)

static const unsigned char blink_time[10][3] = {
	/* Hold time,	fall time,	delay time. */
	{0x3,		0x3,		0x3},	/* 1s */
	{0x3,		0x4,		0x3},	/* 2s */
	{0x3,		0x4,		0x4},	/* 3s */
	{0x3,		0x5,		0x0},	/* 4s */
	{0x3,		0x5,		0x4},	/* 5s */
	{0x3,		0x5,		0x5},	/* 6s */
	{0x3,		0x5,		0x5},	/* 7s */
	{0x3,		0x5,		0x6},	/* 8s */
	{0x3,		0x6,		0x4},	/* 9s */
	{0x3,		0x6,		0x5},	/* 10s */
};

static inline int aw2013_reg_read(u8 reg, u8 *value)
{
	return comip_i2c_read(AW2013_I2C_ID, AW2013_I2C_ADDR, reg, value);
}

static inline int aw2013_reg_write(u8 reg, u8 value)
{
	return comip_i2c_write(AW2013_I2C_ID, AW2013_I2C_ADDR, reg, value);
}

/*
 * Delays are in milliseconds
 */
int led_set_blink(enum led_colors color,
		unsigned long delay_on, unsigned long delay_off)
{
	gd->blink_delay_on[color] = delay_on;
	gd->blink_delay_off[color] = delay_off;

	return 0;
}

int led_set_brightness(enum led_colors color,
		enum led_brightness brightness)
{
	u8 val;

	if (brightness > 0) {
		aw2013_reg_write(LEDGCR, 1);

		if (gd->blink_delay_off[color] == 0)
			aw2013_reg_write(LED_CTRL(color), 0x1);
		else {
			aw2013_reg_write(LED_CTRL(color), 0x51);
			aw2013_reg_write(LED_T0(color), 0x3);
			aw2013_reg_write(LED_T1(color),
				blink_time[(gd->blink_delay_off[color] / 1000) - 1][1] << 4);
			aw2013_reg_write(LED_T2(color),
				blink_time[(gd->blink_delay_off[color] / 1000) - 1][2] << 4);
		}

		aw2013_reg_write(LED_LEVEL(color), brightness);
		aw2013_reg_read(LEDSEL, &val);
		aw2013_reg_write(LEDSEL, val | LED_SWITCH(color));
	} else {
		aw2013_reg_read(LEDSEL, &val);
		aw2013_reg_write(LEDSEL, val & (~(LED_SWITCH(color))));
		aw2013_reg_write(LEDGCR, 0);
	}

	return 0;
}

