/* Copyright (C) 2014-2015leadcore.
* Author: leadcore Tech.
*
* This package is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#include <common.h>
#include "bq24296_charger.h"

#if defined(CONFIG_COMIP_SMART_V1_0)
#define CHARGER_I2C_ID		(I2C2)
#else
#define CHARGER_I2C_ID		(COM_I2C)
#endif
#define GPIO163_I2C2_SCK			(163)
#define GPIO164_I2C2_SDA			(164)

static inline int bq24296_read_byte(u8 *val,u8 reg)
{
	return comip_i2c_read(CHARGER_I2C_ID, BQ24296_I2C_ADDR, reg, val);
}

static inline int bq24296_write_byte(u8 val,u8 reg)
{
	return comip_i2c_write(CHARGER_I2C_ID, BQ24296_I2C_ADDR, reg, val);
}

static void bq24296_config_input_source_reg(unsigned int cin_dpmmV, unsigned int cin_limit)
{
	unsigned int vindpm = 0;
	u8 Vdpm = 0;
	u8 Iin_limit = 0;
	unsigned short    input_source_reg00=0;
	unsigned char hz_mode;

	vindpm = cin_dpmmV;

	if(vindpm < VINDPM_MIN_3880)
		vindpm = VINDPM_MIN_3880;
	else if (vindpm > VINDPM_MAX_5080)
		vindpm = VINDPM_MAX_5080;

	if (cin_limit <= IINLIM_100)
		Iin_limit = 0;
	else if (cin_limit > IINLIM_100 && cin_limit <= IINLIM_150)
		Iin_limit = 1;
	else if (cin_limit > IINLIM_150 && cin_limit <= IINLIM_500)
		Iin_limit = 2;
	else if (cin_limit > IINLIM_500 && cin_limit <= IINLIM_900)
		Iin_limit = 3;
	else if (cin_limit > IINLIM_900 && cin_limit <= IINLIM_1200)
		Iin_limit = 4;
	else if (cin_limit > IINLIM_1200 && cin_limit <= IINLIM_1500)
		Iin_limit = 5;
	else if (cin_limit > IINLIM_1500 && cin_limit <= IINLIM_2000)
		Iin_limit = 6;
	else if (cin_limit > IINLIM_2000 && cin_limit <= IINLIM_3000)
		Iin_limit = 7;
	else
		Iin_limit = 4;

	Vdpm = (vindpm -VINDPM_MIN_3880)/VINDPM_STEP_80;

	hz_mode = DIS_HIZ;

	input_source_reg00 = (hz_mode << BQ24296_EN_HIZ_SHIFT)
	                     | (Vdpm << BQ24296_VINDPM_SHIFT) |Iin_limit;

	bq24296_write_byte(input_source_reg00, INPUT_SOURCE_REG00);
	return;
}

static void bq24296_config_power_on_reg(void)
{
	unsigned int sys_min = 0;
	unsigned int chrg_config = 0;
	unsigned short power_on_config_reg01 = 0;
	u8 Sysmin = 0;
	unsigned char boost_lim;

	sys_min = 3300;

	if(sys_min < SYS_MIN_MIN_3000)
		sys_min = SYS_MIN_MIN_3000;
	else if (sys_min > SYS_MIN_MAX_3700)
		sys_min = SYS_MIN_MAX_3700;

	Sysmin = (sys_min -SYS_MIN_MIN_3000)/SYS_MIN_STEP_100;

	chrg_config = EN_CHARGER;

	boost_lim = BOOST_LIM_500;

	power_on_config_reg01 = WATCHDOG_TIMER_RST
	                        | (chrg_config << BQ24296_EN_CHARGER_SHIFT)
	                        | (Sysmin << BQ24296_SYS_MIN_SHIFT) | boost_lim;

	bq24296_write_byte(power_on_config_reg01, POWER_ON_CONFIG_REG01);
	return;
}

static void bq24296_config_current_reg(unsigned int bqurrentmA)
{
	unsigned int currentmA = 0;
	u8 Vichrg = 0;
	unsigned short charge_current_reg02 = 0;
	unsigned char enable_low_chg;

	currentmA = bqurrentmA;

	if (currentmA < ICHG_MIN)
		currentmA = ICHG_MIN;
	else if(currentmA > ICHG_MAX)
		currentmA = ICHG_MAX;
	Vichrg = (currentmA - ICHG_MIN)/ICHG_STEP_64;
	enable_low_chg = DIS_FORCE_20PCT;

	charge_current_reg02 = (Vichrg << BQ24296_ICHG_SHIFT) | enable_low_chg;

	bq24296_write_byte(charge_current_reg02, CHARGE_CURRENT_REG02);
	return;
}

static void bq24296_config_prechg_term_current_reg(void)
{
	unsigned int precurrentmA = 0;
	unsigned int term_currentmA = 0;
	u8 Prechg = 0;
	u8 Viterm = 0;
	unsigned short prechrg_term_current_reg03 = 0;
	unsigned short bqchip_version = BQ24192;

	precurrentmA = IPRECHRG_256;
	term_currentmA = ITERM_MIN_128;

	if(precurrentmA < IPRECHRG_MIN_128)
		precurrentmA = IPRECHRG_MIN_128;
	if(term_currentmA < ITERM_MIN_128)
		term_currentmA = ITERM_MIN_128;

	if((bqchip_version & BQ24192I)) {
		if(precurrentmA > IPRECHRG_640)
			precurrentmA = IPRECHRG_640;
	}

	if ((bqchip_version & (BQ24190|BQ24191|BQ24192))) {
		if (precurrentmA > IPRECHRG_MAX_2048)
			precurrentmA = IPRECHRG_MAX_2048;
	}

	if (term_currentmA > ITERM_MAX_2048)
		term_currentmA = ITERM_MAX_2048;

	Prechg = (precurrentmA - IPRECHRG_MIN_128)/IPRECHRG_STEP_128;
	Viterm = (term_currentmA-ITERM_MIN_128)/ITERM_STEP_128;

	prechrg_term_current_reg03 = (Prechg <<  BQ24296_IPRECHRG_SHIFT| Viterm);
	bq24296_write_byte(prechrg_term_current_reg03, PRECHARGE_TERM_CURRENT_REG03);
	return;
}

static void bq24296_config_voltage_reg(unsigned int max_voltagemV)
{
	unsigned int voltagemV = 0;
	u8 Voreg = 0;
	unsigned short charge_voltage_reg04 = 0;

	voltagemV = max_voltagemV;
	if (voltagemV < VCHARGE_MIN_3504)
		voltagemV = VCHARGE_MIN_3504;
	else if (voltagemV > VCHARGE_MAX_4400)
		voltagemV = VCHARGE_MAX_4400;

	Voreg = (voltagemV - VCHARGE_MIN_3504)/VCHARGE_STEP_16;

	charge_voltage_reg04 = (Voreg << BQ24296_VCHARGE_SHIFT) | BATLOWV_3000 |VRECHG_100;
	bq24296_write_byte(charge_voltage_reg04, CHARGE_VOLTAGE_REG04);
	return;
}

static void bq24296_config_term_timer_reg(void)
{
	unsigned short term_timer_reg05 = 0;
	unsigned char enable_iterm = EN_TERM;
	unsigned char enable_timer = EN_TIMER;
	unsigned int watchdog_timer = WATCHDOG_40;
	unsigned int chrg_timer = CHG_TIMER_8;

	term_timer_reg05 = (enable_iterm << BQ24296_EN_TERM_SHIFT)
	                   | watchdog_timer | (enable_timer << BQ24296_EN_TIMER_SHIFT)
	                   | chrg_timer;// | JEITA_ISET;

	bq24296_write_byte(term_timer_reg05, CHARGE_TERM_TIMER_REG05);
	return;
}

static void bq24296_config_thernal_regulation_reg(void)
{
	return;

}

static void bq24296_config_misc_operation_reg(void)
{
	unsigned short misc_operation_reg07 = 0;
	unsigned char enable_dpdm = DPDM_DIS;
	unsigned char enable_batfet = EN_BATFET;

	misc_operation_reg07 = (enable_dpdm << BQ24296_DPDM_EN_SHIFT)
	                       | TMR2X_EN |(enable_batfet<< BQ24296_BATFET_EN_SHIFT)
	                       | CHRG_FAULT_INT_EN |BAT_FAULT_INT_EN;

	bq24296_write_byte(misc_operation_reg07, MISC_OPERATION_REG07);
	return;
}

int special_charger_charging_init(u8 flag)
{
	u8 val=0,i=0;
#if defined(CONFIG_COMIP_SMART_V1_0)
	comip_mfp_config(GPIO163_I2C2_SCK, MFP_PIN_MODE_1);
	comip_mfp_config(GPIO164_I2C2_SDA, MFP_PIN_MODE_1);

	comip_i2c_init(CHARGER_I2C_ID, 0);
	mdelay(10);
#endif
	bq24296_config_power_on_reg();
	mdelay(1000);
	bq24296_config_input_source_reg(4360,500);
	bq24296_config_current_reg(500);
	bq24296_config_prechg_term_current_reg();
	bq24296_config_voltage_reg(4208);
	bq24296_config_term_timer_reg();
	bq24296_config_misc_operation_reg();
	/*
	for(i=0;i<10;i++)
	{
		bq24296_read_byte(&val,i);
		printf("bq24296: reg[0x%x]=0x%x\n",i,val);
	}
	*/
	return 0;
}

