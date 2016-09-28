
/*
 * drivers/power/ncp1851_charger.c
 *
 * ncp1851/A charging driver
 *
 * Copyright (C) 2014-2015 onsemi/leadcore.
 * Author: leadcore Tech.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include "ncp1851_charger.h"

static inline int ncp1851_read_byte(u8 *val,u8 reg)
{
    return pmic_reg_read(NCP1851_I2C_ADDR, reg, val);
}

static inline int ncp1851_write_byte(u8 val,u8 reg)
{
    return pmic_reg_write(NCP1851_I2C_ADDR, reg, val);
}

static void ncp1851_ctrl1_set_reg(u8 enable_charger)
{
    u8 ctrl1_reg = 0;
    ctrl1_reg = ((enable_charger << CHG_EN_SHIFT)|(OTG_DIS << OTG_EN_SHIFT)
                     |(NTC_DIS << NTC_EN_SHIFT)|(TCHG_RST_DIS << TCHG_RST_SHIFT)
                     |(INT_UNMASK));
    ctrl1_reg = ctrl1_reg & 0x7F;
    ncp1851_write_byte(ctrl1_reg, REG_CTRL1_REGISTER);

    return;
}

static void ncp1851_ctrl2_set_reg(void)
{
    u8 ctrl2_reg = 0;

    ctrl2_reg = ((WDTO_DIS   << WDTO_DIS_SHIFT)
                     |(CHGTO_DIS << CHGTO_DIS_SHIFT)
                     |(PWR_PATH_EN  << PWR_PATH_SHIFT)|(TRANS_EN_REG)
                     |(IINSET_PIN_DIS << IINSET_PIN_EN_SHIFT)
                     |(IINLIM_EN << 1)|(AICL_DIS));

    ncp1851_write_byte(ctrl2_reg, REG_CTRL2_REGISTER);

    return;
}


/* Charge volysge set mV */
static void ncp1851_vbat_set_reg(int voltagemV)
{
    u8 vbat_reg = 0;

    if (voltagemV < CTRL_VBAT_MIN)
        voltagemV = CTRL_VBAT_MIN;

    if (voltagemV > CTRL_VBAT_MAX)
        voltagemV = CTRL_VBAT_MAX;

    vbat_reg = (voltagemV - CTRL_VBAT_MIN)/CTRL_VBAT_STEP;
    ncp1851_write_byte(vbat_reg, REG_VBAT_SET_REGISTER);

    return;
}

/* Charge current and termination current set mA */
static void ncp1851_ibat_set_reg(int currentmA)
{
    unsigned int term_currentmA = 0;
    u8 Vichrg = 0;
    u8 Viterm = 0,ibat_reg = 0;

    term_currentmA = 100;
    if (currentmA < ICHG_MIN)
        currentmA = ICHG_MIN;
    if (currentmA > ICHG_MAX)
        currentmA = ICHG_MAX;

    if (term_currentmA < IEOC_MIN)
        term_currentmA = IEOC_MIN;
    if (term_currentmA > IEOC_MAX)
        term_currentmA = IEOC_MAX;

    Vichrg = (currentmA - ICHG_MIN)/ICHG_STEP;
    Viterm = (term_currentmA - IEOC_MIN)/IEOC_STEP;

    ibat_reg = (Viterm << IEOC_SHIFT | Vichrg);
    ncp1851_write_byte(ibat_reg, REG_IBAT_SET_REGISTER);

    return;
}

/* input current set mA */
static void ncp1851_misc_set_reg(int cin_limit)
{
    u8 Iin_limit = 0;
    u8 iweak = 0,ctrl_vfet = 0,misc_reg = 0;

    if (cin_limit <= 100)
        Iin_limit = 0;
    else if ((cin_limit > 100) && (cin_limit <= 500))
        Iin_limit = 1;
    else if ((cin_limit > 500) && (cin_limit <= 900))
        Iin_limit = 2;
    else
        Iin_limit = 3;

    iweak = (100 - IWEAK_MIN)/IWEAK_STEP;
    ctrl_vfet = (3400 - CTRL_VFET_MIN)/CTRL_VFET_STEP;

    misc_reg = ((iweak << IWEAK_SHIFT)
                    |(ctrl_vfet << CTRL_VFET_SHIFT)|Iin_limit);

    ncp1851_write_byte(misc_reg, REG_MISC_SET_REGISTER);
    return;
}

int special_charger_charging_init(u8 flag)
{
    ncp1851_ctrl1_set_reg(CHG_EN);
    ncp1851_ctrl2_set_reg();
    ncp1851_vbat_set_reg(4200);
    ncp1851_misc_set_reg(500);
    ncp1851_ibat_set_reg(500);

    return 0;
}

