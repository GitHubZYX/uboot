/*
 * Copyright (C) 2010 Texas Instruments
 * Author: Balaji T K
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _BQ24158_CHARGER_H
#define _BQ24158_CHARGER_H

/* not a bq generated event,we use this to reset the
 * the timer from the twl driver.
 */
#define BQ2415x_RESET_TIMER      0x38

#define BQ24158_I2C_ADDR        (0x6A)
#define FAN5405_I2C_ADDR        (0x6A)



/* BQ24153 / BQ24156 / BQ24158 */
/* Status/Control Register */
#define REG_STATUS_CONTROL      0x00
#define TIMER_RST           (1 << 7)
#define ENABLE_STAT_PIN     (1 << 6)
#define BOOST_MODE_SHIFT      3
#define BOOST_MODE           (1)
#define CHARGE_MODE          (0)

/* Control Register */
#define REG_CONTROL_REGISTER      0x01
#define INPUT_CURRENT_LIMIT_SHIFT   6
#define ENABLE_ITERM_SHIFT          3
#define ENABLE_CHARGER_SHIFT        2
#define ENABLE_CHARGER            (0)
#define DISABLE_CHARGER           (1)

/* Control/Battery Voltage Register */
#define REG_BATTERY_VOLTAGE      0x02
#define VOLTAGE_SHIFT            2
#define OTG_PL_EN                (1)
#define OTG_PL_DIS               (0)
#define OTG_EN                   (1)
#define OTG_DIS                  (0)

/* Vender/Part/Revision Register */
#define REG_PART_REVISION       0x03

/* Battery Termination/Fast Charge Current Register */
#define REG_BATTERY_CURRENT      0x04
#define BQ24156_CURRENT_SHIFT      3
#define BQ24153_CURRENT_SHIFT      4
#define BQ24158_CURRENT_SHIFT      4

/* Special Charger Voltage/Enable Pin Status Register */
#define REG_SPECIAL_CHARGER_VOLTAGE    0x05
#define LOW_CHG_SHIFT        5
#define LOW_CHG_EN           (1)
#define LOW_CHG_DIS          (0)

/* Safety Limit Register */
#define REG_SAFETY_LIMIT      0x06
#define MAX_CURRENT_SHIFT      4

#define BQ24153 (1 << 3)
#define BQ24156 (1 << 6)
#define BQ24158 (1 << 8)

#define BQ2415x_WATCHDOG_TIMEOUT      (20000)


#endif /* __BQ2415x_CHARGER_H */
