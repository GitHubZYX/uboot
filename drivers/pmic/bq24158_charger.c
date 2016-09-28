 /* Copyright (C) 2014-2015leadcore.
 * Author: leadcore Tech.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include "bq24158.h"


static inline int bq2415x_read_byte(u8 *val,u8 reg)
{
    return pmic_reg_read(BQ24158_I2C_ADDR, reg, val);
}

static inline int bq2415x_write_byte(u8 val,u8 reg)
{
    return pmic_reg_write(BQ24158_I2C_ADDR, reg, val);
}

static void bq2415x_config_status_reg(void)
{
    u8 status_reg = 0;
    status_reg = (TIMER_RST | ENABLE_STAT_PIN);
    bq2415x_write_byte(status_reg, REG_STATUS_CONTROL);
    return;
}

static void bq2415x_config_control_reg(int cin_limit)
{
    u8 Iin_limit = 0,control_reg = 0;

    if (cin_limit <= 100)
        Iin_limit = 0;
    else if (cin_limit > 100 && cin_limit <= 500)
        Iin_limit = 1;
    else if (cin_limit > 500 && cin_limit <= 800)
        Iin_limit = 2;
    else
        Iin_limit = 3;

    control_reg = ((Iin_limit << INPUT_CURRENT_LIMIT_SHIFT)
                | (0 << ENABLE_ITERM_SHIFT)
                | (ENABLE_CHARGER << ENABLE_CHARGER_SHIFT));
    bq2415x_write_byte(control_reg, REG_CONTROL_REGISTER);
    return;
}

static void bq2415x_config_voltage_reg(int voltagemV)
{
    u8 Voreg = 0,voltage_reg = 0;

    if (voltagemV < 3500)
        voltagemV = 3500;
    else if (voltagemV > 4440)
        voltagemV = 4440;

    Voreg = (voltagemV - 3500)/20;
    voltage_reg = (Voreg << VOLTAGE_SHIFT)|(OTG_PL_EN << 1);
    bq2415x_write_byte(voltage_reg, REG_BATTERY_VOLTAGE);
    return;
}

static void bq2415x_config_current_reg(int currentmA)
{
    unsigned int term_currentmA = 0;
    u8 Vichrg = 0,current_reg = 0;
    u8 shift = 0;
    u8 Viterm = 0;

    term_currentmA = 100;

    if (currentmA < 550)
        currentmA = 550;

    shift = BQ24158_CURRENT_SHIFT;
    if (currentmA > 1250)
       currentmA = 1250;

    if (term_currentmA > 350)
        term_currentmA = 350;

    Vichrg = (currentmA - 550)/100;
    Viterm = (term_currentmA - 50)/50;

    current_reg = (Vichrg << shift | Viterm);
    bq2415x_write_byte(current_reg, REG_BATTERY_CURRENT);
    return;
}

static void bq2415x_config_special_charger_reg(int dpmvoltagemV)
{
    u8 Vsreg = 2; /* 160/80 */
    u8 special_charger_reg = 0;

    if(dpmvoltagemV < 4200){
        dpmvoltagemV = 4200;
    }
    if(dpmvoltagemV > 4760){
        dpmvoltagemV = 4760;
    }
    Vsreg = (dpmvoltagemV - 4200)/80;
    special_charger_reg = Vsreg;
    bq2415x_write_byte(special_charger_reg,REG_SPECIAL_CHARGER_VOLTAGE);
    return;
}

static void bq2415x_config_safety_reg(unsigned int max_currentmA,
                unsigned int max_voltagemV)
{
    u8 Vmchrg = 0;
    u8 Vmreg = 0;
    u8 limit_reg = 0;

    if (max_currentmA < 550)
        max_currentmA = 550;
    else if (max_currentmA > 1550)
        max_currentmA = 1550;

    if (max_voltagemV < 4200)
        max_voltagemV = 4200;
    else if (max_voltagemV > 4440)
        max_voltagemV = 4440;


    Vmchrg = (max_currentmA - 550)/100;
    Vmreg = (max_voltagemV - 4200)/20;
    limit_reg = ((Vmchrg << MAX_CURRENT_SHIFT) | Vmreg);
    bq2415x_write_byte(limit_reg, REG_SAFETY_LIMIT);
    return;
}

int special_charger_charging_init(u8 flag)
{
    if(flag){
        /*only set once*/
        bq2415x_config_safety_reg(1250,4350);
    }
    bq2415x_config_voltage_reg(4200);
    bq2415x_config_control_reg(500);
    bq2415x_config_current_reg(550);
    bq2415x_config_special_charger_reg(4200);
    bq2415x_config_status_reg();

    return 0;
}

