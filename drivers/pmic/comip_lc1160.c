
#include <common.h>

#if (defined(CONFIG_PMIC_LC1160) || defined(CONFIG_PMIC_AUTO))

/* LC1160 I2C address. */
#define LC1160_I2C_ADDR			(0x33)

/* LC1160 register address. */
#define LC1160_REG_STARTUP_STATUS		(0x5c)
#define LC1160_REG_DC2DVSCR			(0x18)
#define LC1160_REG_DCDC1_OUT_0			(0x01)
#define LC1160_REG_DCDC1_OUT_1			(0x02)
#define LC1160_REG_DCDC2_OUT_0			(0x03)
#define LC1160_REG_DCDC2_OUT_1			(0x04)
#define LC1160_REG_DCDC2_OUT_2			(0x05)
#define LC1160_REG_DCDC2_OUT_3			(0x06)
#define LC1160_REG_DCDC3_OUT_0			(0x07)
#define LC1160_REG_DCDC3_OUT_1			(0x08)
#define LC1160_REG_DCDC3_OUT_2			(0x09)
#define LC1160_REG_DCDC3_OUT_3			(0x0a)
#define LC1160_REG_DCDC4_OUT_0			(0x0b)
#define LC1160_REG_DCDC4_OUT_1			(0x0c)
#define LC1160_REG_DCDC5_OUT_0			(0x0d)
#define LC1160_REG_DCDC5_OUT_1			(0x0e)
#define LC1160_REG_DCDC6_OUT_0			(0x0f)
#define LC1160_REG_DCDC6_OUT_1			(0x10)
#define LC1160_REG_DCDC9_OUT_0			(0x15)
#define LC1160_REG_DCDC9_OUT_1			(0x16)
#define LC1160_REG_DC1CR			(0x1a)
#define LC1160_REG_DC2CR			(0x1b)
#define LC1160_REG_DC3CR			(0x1c)
#define LC1160_REG_DC4CR			(0x1d)
#define LC1160_REG_DC5CR			(0x1e)
#define LC1160_REG_DC6CR			(0x1f)
#define LC1160_REG_DC9CR			(0x20)
#define LC1160_REG_DCLDOCONEN			(0x29)
#define LC1160_REG_LDOAONEN			(0x2a)
#define LC1160_REG_LDOA3CR                     (0x2e)
#define LC1160_REG_LDOA8CR			(0x34)
#define LC1160_REG_LDOA10CR			(0x36)
#define LC1160_REG_LDOA12CR			(0x38)
#define LC1160_REG_LDOD6CR			(0x43)
#define LC1160_REG_LDOD7CR			(0x44)
#define LC1160_REG_SPICR			(0x49)
#define LC1160_REG_CHGCSET			(0x58)
#define LC1160_REG_PCST			(0x5b)
#define LC1160_REG_SHDN_STATUS			(0x5d)
#define LC1160_REG_SHDN_EN			(0x5e)

#define LC1160_REG_ADCCR1			(0x87)
#define LC1160_REG_ADCCR2			(0x88)
#define LC1160_REG_ADCDAT0			(0x89)
#define LC1160_REG_ADCDAT1			(0x8a)
#define LC1160_REG_RTC_AL1_SEC     		(0x6d)
#define LC1160_REG_RTC_AL1_MIN     		(0x6e)
#define LC1160_REG_RTC_AL2_SEC     		(0x6f)
#define LC1160_REG_RTC_AL2_MIN     		(0x70)
#define LC1160_REG_RTC_AL2_HOUR    		(0x71)
#define LC1160_REG_RTC_AL2_DAY     		(0x72)
#define LC1160_REG_RTC_AL2_MONTH   		(0x73)
#define LC1160_REG_RTC_AL2_YEAR    		(0x74)
#define LC1160_REG_RTC_AL_UPDATE   		(0x75)
#define LC1160_REG_RTC_AL1_HOUR    		(0x7c)
#define LC1160_REG_RTC_AL1_DAY    	    	(0x7d)
#define LC1160_REG_RTC_AL1_MONTH    		(0x7e)
#define LC1160_REG_RTC_AL1_YEAR    		(0x7f)
#define LC1160_REG_RTC_DATA1    		(0x80)
#define LC1160_REG_RTC_DATA2    		(0x81)
#define LC1160_REG_RTC_DATA3    		(0x82)
#define LC1160_REG_RTC_DATA4    		(0x83)

#define LC1160_REG_BITMASK_SHDNFLAG		(0xc0)

/* LC1160_REG_DC1OVS0 */
#define LC1160_REG_BITMASK_DC1VOUT0		(0x3f)

/* LC1160_REG_DC1OVS1 */
#define LC1160_REG_BITMASK_DC1SLP		(0x80)

/* LC1160_REG_DC2OVS0 */
#define LC1160_REG_BITMASK_DC2VOUT0		(0x3f)
#define LC1160_REG_BITMASK_DC2EN		(0x40)

/* LC1160_REG_DC2OVS1 */
#define LC1160_REG_BITMASK_DC2SLP		(0x80)

/* LC1160_REG_DC3OVS0 */
#define LC1160_REG_BITMASK_DC3VOUT0		(0x3f)
#define LC1160_REG_BITMASK_DC3EN		(0x40)

/* LC1160_REG_DC3OVS1 */
#define LC1160_REG_BITMASK_DC3SLP		(0x80)

/* LC1160_REG_DC4OVS0 */
#define LC1160_REG_BITMASK_DC4VOUT0		(0x3f)

/* LC1160_REG_DC4OVS1 */
#define LC1160_REG_BITMASK_DC4SLP		(0x80)

/* LC1160_REG_DC5OVS0 */
#define LC1160_REG_BITMASK_DC5EN		(0x40)
#define LC1160_REG_BITMASK_DC5VOUT0		(0x1f)

/* LC1160_REG_DC5OVS1 */
#define LC1160_REG_BITMASK_DC5EN_SL		(0x40)
#define LC1160_REG_BITMASK_DC5SLP		(0x80)

/* LC1160_REG_DC6OVS0 */
#define LC1160_REG_BITMASK_DC6VOUT0		(0x1f)

/* LC1160_REG_DC6OVS1 */
#define LC1160_REG_BITMASK_DC6SLP		(0x80)

/* LC1160_REG_DC9OVS1 */
#define LC1160_REG_BITMASK_DC9SLP		(0x80)
#define LC1160_REG_BITMASK_DC9EN		(0x40)

/* LC1160_REG_DC2DVSCR */
#define LC1160_REG_BITMASK_DC2_DVS_SEL		(0x03)

/* LC1160_REG_DC1CR */
#define LC1160_REG_BITMASK_DC1_PWM_FORCE	(0x20)

/* LC1160_REG_DC2CR */
#define LC1160_REG_BITMASK_DC2_PWM_FORCE	(0x20)

/* LC1160_REG_DC3CR */
#define LC1160_REG_BITMASK_DC3_PWM_FORCE	(0x20)

/* LC1160_REG_DC4CR */
#define LC1160_REG_BITMASK_DC4_PWM_FORCE	(0x20)

/* LC1160_REG_DC5CR */
#define LC1160_REG_BITMASK_DC5_PWM_FORCE	(0x20)

/* LC1160_REG_DC6CR */
#define LC1160_REG_BITMASK_DC6_PWM_FORCE	(0x20)

/* LC1160_REG_DC9CR */
#define LC1160_REG_BITMASK_DC9_PWM_FORCE	(0x08)

/* LC1160_REG_DCLDOCONEN */
#define LC1160_REG_BITMASK_LDOA1CONEN		(0x04)
#define LC1160_REG_BITMASK_LDOA4CONEN		(0x08)
#define LC1160_REG_BITMASK_LDOA8CONEN		(0x10)

/* LC1160_REG_LDOA3CR */
#define LC1160_REG_BITMASK_LDOA3EN             (0x80)

/* LC1160_REG_LDOA8CR */
#define LC1160_REG_BITMASK_LDOA8EN		(0x80)

/*LC1160_REG_LDOA10CR*/
#define LC1160_REG_BITMASK_LDOA10OV		(0x1f)

/* LC1160_REG_LDOA10CR */
#define LC1160_REG_BITMASK_LDOA10EN		(0x80)

/* LC1160_REG_SPICR */
#define LC1160_REG_BITMASK_SPI_IIC_EN		(0x01)

/* LC1160_REG_ADCCR1 */
#define LC1160_REG_BITMASK_TS_PCB_VREF		(0xe0)
#define LC1160_REG_BITMASK_ADEND		(0x10)
#define LC1160_REG_BITMASK_ADSTART 		(0x08)
#define LC1160_REG_BITMASK_ADFORMAT 		(0x04)
#define LC1160_REG_BITMASK_ADCEN		(0x02)
#define LC1160_REG_BITMASK_LDOADCEN 		(0x01)

/* LC1160_REG_ADCCR2 */
#define LC1160_REG_BITMASK_TS_PCB_ON		(0x10)
#define LC1160_REG_BITMASK_ADMODE		(0x08)
#define LC1160_REG_BITMASK_ADSEL 		(0x07)

/* LC1160_REG_UP_STATE */
#define LC1160_REG_BITMASK_RSTINB_STARTUP 	(0x20)
#define LC1160_REG_BITMASK_ALARM2_STARTUP 	(0x10)
#define LC1160_REG_BITMASK_ALARM1_STARTUP 	(0x08)
#define LC1160_REG_BITMASK_ADPIN_STARTUP 	(0x04)
#define LC1160_REG_BITMASK_HF_PWR_STARTUP	(0x02)
#define LC1160_REG_BITMASK_KEYON_STARTUP 	(0x01)

/* LC1160_REG_SHDN_STATE */
#define LC1160_REG_BITMASK_RSTINB_SHDN 	(0x20)

/* PCST: Power-related Status Register */
#define LC1160_REG_BITMASK_KONMON		(0x20)
#define LC1160_REG_BITMASK_ADPIN		(0x01)

/* LC1160_REG_LDOA12CR */
#define LC1160_REG_BITMASK_LDOA12EN		(0x80)

/* LC1160_REG_LDOD6CR */
#define LC1160_REG_BITMASK_LDOD6EN		(0x80)

/* LC1160_REG_LDOD7CR */
#define LC1160_REG_BITMASK_LDOD7OV		(0x1f)

/* LC1160_REG_LDOD7CR */
#define LC1160_REG_BITMASK_LDOD7EN		(0x80)

/*Current Sinks Control Register*/
#define LC1160_REG_SINKCR           (0x51)
#define LC1160_VIBRATOREN           (1 << 2)

#define LC1160_REG_VIBI_FLASHIOUT   (0x53)
/*80mA*/
#define LC1160_VIBIOUT              (0x8)

/*Current Sinks Control Register*/
#define LC1160_REG_PWM_CTL          (0x95)
#define LC1160_PWM1_EN              (3 << 5)



/* DDR_PWR_PMU_CTL */
#define VOL_SW_A7					12
#define VOL_TW_A7					8
#define VOL_SW_GPU					4
#define VOL_TW_GPU					0

enum {
	LC1160_DCDC1 = 0,
	LC1160_DCDC2,
	LC1160_DCDC3,
	LC1160_DCDC4,
	LC1160_DCDC5,
	LC1160_DCDC6,
	LC1160_DCDC_MAX,
};

enum {
	LC1160_DCDC_VOUT0 = 0,
	LC1160_DCDC_VOUT1,
	LC1160_DCDC_VOUT2,
	LC1160_DCDC_VOUT3,
	LC1160_DCDC_VOUT_MAX,
};

static int lc1160_reg_read(u8 reg, u8 *value)
{
	return pmic_reg_read(LC1160_I2C_ADDR, reg, value);
}

static int lc1160_reg_write(u8 reg, u8 value)
{
	return pmic_reg_write(LC1160_I2C_ADDR, reg, value);
}

static int lc1160_reg_bit_write(u8 reg, u8 mask, u8 value)
{
	return pmic_reg_bit_write(LC1160_I2C_ADDR, reg, mask, value);
}

static int lc1160_vibrator_enable(struct pmic_info *info)
{
	u8 val = 0;

#ifndef CONFIG_PMIC_VIBRATOR
	return 0;
#endif
    /*enable vibrator*/
	lc1160_reg_read(LC1160_REG_VIBI_FLASHIOUT, &val); /* Low  4 bit */
	val = val &0xF0;
	val = val|LC1160_VIBIOUT;
	lc1160_reg_write(LC1160_REG_VIBI_FLASHIOUT, val);

	lc1160_reg_read(LC1160_REG_SINKCR, &val);
	val = val |LC1160_VIBRATOREN;
	lc1160_reg_write(LC1160_REG_SINKCR, val);

	lc1160_reg_read(LC1160_REG_PWM_CTL, &val);
	val = val |LC1160_PWM1_EN;
	lc1160_reg_write(LC1160_REG_PWM_CTL, val);

	udelay(100000);
    /*disable vibrator*/

	lc1160_reg_read(LC1160_REG_PWM_CTL, &val);
	val = val &0x07;
	lc1160_reg_write(LC1160_REG_PWM_CTL, val);

	lc1160_reg_read(LC1160_REG_SINKCR, &val);
	val = val &0x0B;
	lc1160_reg_write(LC1160_REG_SINKCR, val);

    return 0;
}

static int lc1160_lcdio_enable(struct pmic_info *info, int enable)
{
	if (enable) {
		lc1160_reg_bit_write(LC1160_REG_LDOA10CR, LC1160_REG_BITMASK_LDOA10OV, 0x0d);	/*set VDDIO to 1.8V*/
		lc1160_reg_bit_write(LC1160_REG_LDOA10CR, LC1160_REG_BITMASK_LDOA10EN, 0x01);	/*enable VDDIO*/
	} else {
		lc1160_reg_bit_write(LC1160_REG_LDOA10CR, LC1160_REG_BITMASK_LDOA10EN, 0x00);	/*disable VDDIO*/
	}

    return 0;
}

static int lc1160_lcdcore_enable(struct pmic_info *info, int enable)
{
	if (enable) {
		lc1160_reg_bit_write(LC1160_REG_LDOD7CR, LC1160_REG_BITMASK_LDOD7OV, 0x15);	/*set VDDCORE to 2.8v*/
		lc1160_reg_bit_write(LC1160_REG_LDOD7CR, LC1160_REG_BITMASK_LDOD7EN, 0x01);	/*enable VDDCORE*/
	} else {
		lc1160_reg_bit_write(LC1160_REG_LDOD7CR, LC1160_REG_BITMASK_LDOD7EN, 0x00);	/*disable VDDCORE*/
	}

    return 0;
}

static int lc1160_dcdc1_vout_set(u8 vout_id, int mv)
{
	const int lc1160_dcdc1_vout[] = {
		6500, 6625, 6750, 6875, 7000, 7125, 7250, 7375,7500,7625, 7750,7875, 8000, 8125,
		8250, 8375, 8500, 8625, 8750, 8875, 9000, 9125, 9250, 9375, 9500, 9625, 9750, 9875,
		10000, 10125, 10250, 10375, 10500, 10625, 10750, 10875, 11000, 11125, 11250, 11375,
		11500, 11625, 11750, 11875, 12000, 12125, 12250, 12375, 12500, 12625, 12750, 12875,
		13000, 13125, 13250, 13375, 13500, 13625, 13750, 13875, 14000, 14125, 14250, 14375
	};
	u8 reg;
	u8 i;

	if (vout_id >= LC1160_DCDC_VOUT2)
		return -1;

	for (i = 0; i < ARRAY_SIZE(lc1160_dcdc1_vout); i++) {
		if (lc1160_dcdc1_vout[i] == mv)
			break;
	}

	if (i == ARRAY_SIZE(lc1160_dcdc1_vout))
		return -1;

	if (vout_id == LC1160_DCDC_VOUT0) {
		reg = LC1160_REG_DCDC1_OUT_0;
	} else if (vout_id == LC1160_DCDC_VOUT1) {
		reg = LC1160_REG_DCDC1_OUT_1;
	}

	return lc1160_reg_bit_write(reg, LC1160_REG_BITMASK_DC1VOUT0, i);
}

static int lc1160_dcdc2_vout_set(u8 vout_id, int mv)
{
	const int lc1160_dcdc2_vout[] = {
		6500, 6625, 6750, 6875, 7000, 7125, 7250, 7375,7500,7625, 7750,7875, 8000, 8125,
		8250, 8375, 8500, 8625, 8750, 8875, 9000, 9125, 9250, 9375, 9500, 9625, 9750, 9875,
		10000, 10125, 10250, 10375, 10500, 10625, 10750, 10875, 11000, 11125, 11250, 11375,
		11500, 11625, 11750, 11875, 12000, 12125, 12250, 12375, 12500, 12625, 12750, 12875,
		13000, 13125, 13250, 13375, 13500, 13625, 13750, 13875, 14000, 14125, 14250, 14375
	};
	u8 reg;
	u8 i;

	if (vout_id >= LC1160_DCDC_VOUT_MAX)
		return -1;

	for (i = 0; i < ARRAY_SIZE(lc1160_dcdc2_vout); i++) {
		if (lc1160_dcdc2_vout[i] == mv)
			break;
	}

	if (i == ARRAY_SIZE(lc1160_dcdc2_vout))
		return -1;

	if (vout_id == LC1160_DCDC_VOUT0) {
		reg = LC1160_REG_DCDC2_OUT_0;
	} else if (vout_id == LC1160_DCDC_VOUT1) {
		reg = LC1160_REG_DCDC2_OUT_1;
	} else if (vout_id == LC1160_DCDC_VOUT2) {
		reg = LC1160_REG_DCDC2_OUT_2;
	} else if (vout_id == LC1160_DCDC_VOUT3) {
		reg = LC1160_REG_DCDC2_OUT_3;
	}

	return lc1160_reg_bit_write(reg, LC1160_REG_BITMASK_DC2VOUT0, i);
}

static int lc1160_dcdc3_vout_set(u8 vout_id, int mv)
{
	const int lc1160_dcdc3_vout[] = {
		6500, 6625, 6750, 6875, 7000, 7125, 7250, 7375,7500,7625, 7750,7875, 8000, 8125,
		8250, 8375, 8500, 8625, 8750, 8875, 9000, 9125, 9250, 9375, 9500, 9625, 9750, 9875,
		10000, 10125, 10250, 10375, 10500, 10625, 10750, 10875, 11000, 11125, 11250, 11375,
		11500, 11625, 11750, 11875, 12000, 12125, 12250, 12375, 12500, 12625, 12750, 12875,
		13000, 13125, 13250, 13375, 13500, 13625, 13750, 13875, 14000, 14125, 14250, 14375
	};
	u8 reg;
	u8 i;

	if (vout_id >= LC1160_DCDC_VOUT_MAX)
		return -1;

	for (i = 0; i < ARRAY_SIZE(lc1160_dcdc3_vout); i++) {
		if (lc1160_dcdc3_vout[i] == mv)
			break;
	}

	if (i == ARRAY_SIZE(lc1160_dcdc3_vout))
		return -1;

	if (vout_id == LC1160_DCDC_VOUT0) {
		reg = LC1160_REG_DCDC3_OUT_0;
	} else if (vout_id == LC1160_DCDC_VOUT1) {
		reg = LC1160_REG_DCDC3_OUT_1;
	} else if (vout_id == LC1160_DCDC_VOUT2) {
		reg = LC1160_REG_DCDC3_OUT_2;
	} else if (vout_id == LC1160_DCDC_VOUT3) {
		reg = LC1160_REG_DCDC3_OUT_3;
	}

	return lc1160_reg_bit_write(reg, LC1160_REG_BITMASK_DC3VOUT0, i);
}

static int lc1160_dcdc4_vout_set(u8 vout_id, int mv)
{
	const int lc1160_dcdc4_vout[] = {
		8500, 8625, 8750, 8875, 9000, 9125, 9250, 9375, 9500, 9625, 9750, 9875, 10000, 10125, 10250,
		10375, 10500, 10625, 10750, 10875, 11000, 11125, 11250, 11375, 11500, 11625, 11750, 11875,
		12000, 12125, 12250, 12375, 12500, 12625, 12750, 12875, 13000, 13125, 13250, 13375, 13500,
		13625, 13750, 13875, 14000, 14125, 14250, 14375, 14500, 14625, 14750, 14875, 15000, 15125,
		15250, 15375, 15500, 15625, 15750, 15875, 16000, 16125, 16250, 16375
	};
	u8 reg;
	u8 i;

	if (vout_id >= LC1160_DCDC_VOUT2)
		return -1;

	for (i = 0; i < ARRAY_SIZE(lc1160_dcdc4_vout); i++) {
		if (lc1160_dcdc4_vout[i] == mv)
			break;
	}

	if (i == ARRAY_SIZE(lc1160_dcdc4_vout))
		return -1;

	if (vout_id == LC1160_DCDC_VOUT0) {
		reg = LC1160_REG_DCDC4_OUT_0;
	} else if (vout_id == LC1160_DCDC_VOUT1) {
		reg = LC1160_REG_DCDC4_OUT_1;
	}

	return lc1160_reg_bit_write(reg, LC1160_REG_BITMASK_DC4VOUT0, i);
}

static int lc1160_dcdc5_vout_set(u8 vout_id, int mv)
{
	const int lc1160_dcdc5_vout[] = {
		850, 875, 900, 925, 950, 975, 1000, 1025, 1050, 1075, 1100,
		1125, 1150, 1175, 1200, 1225, 1250, 1275, 1300, 1325, 1350,
		1375, 1400, 1425, 1450, 1475, 1500, 1525, 1550, 1575, 1600, 1625
	};
	u8 reg;
	u8 i;

	if (vout_id >= LC1160_DCDC_VOUT2)
		return -1;

	for (i = 0; i < ARRAY_SIZE(lc1160_dcdc5_vout); i++) {
		if (lc1160_dcdc5_vout[i] == mv)
			break;
	}

	if (i == ARRAY_SIZE(lc1160_dcdc5_vout))
		return -1;

	if (vout_id == LC1160_DCDC_VOUT0) {
		reg = LC1160_REG_DCDC5_OUT_0;
	} else if (vout_id == LC1160_DCDC_VOUT1) {
		reg = LC1160_REG_DCDC5_OUT_1;
	}

	return lc1160_reg_bit_write(reg, LC1160_REG_BITMASK_DC5VOUT0, i);
}

static int lc1160_dcdc6_vout_set(u8 vout_id, int mv)
{
	const int lc1160_dcdc6_vout[] = {
		1150, 1175, 1200, 1225, 1250, 1275, 1300, 1325,
		1350, 1375, 1400, 1425, 1450, 1475, 1500, 1525,
		1550, 1575, 1600, 1625, 1650, 1675, 1700, 1725,
		1750, 1775, 1800, 1825, 1850, 1875, 1900, 1925
	};
	u8 reg;
	u8 i;

	if (vout_id >= LC1160_DCDC_VOUT2)
		return -1;

	for (i = 0; i < ARRAY_SIZE(lc1160_dcdc6_vout); i++) {
		if (lc1160_dcdc6_vout[i] == mv)
			break;
	}

	if (i == ARRAY_SIZE(lc1160_dcdc6_vout))
		return -1;

	if (vout_id == LC1160_DCDC_VOUT0) {
		reg = LC1160_REG_DCDC6_OUT_0;
	} else if (vout_id == LC1160_DCDC_VOUT1) {
		reg = LC1160_REG_DCDC6_OUT_1;
	}

	return lc1160_reg_bit_write(reg, LC1160_REG_BITMASK_DC6VOUT0, i);
}

static int lc1160_dcdc_vout_set(u8 dcdc_id, u8 vout_id, int mv)
{
	int ret;

	if (dcdc_id >= LC1160_DCDC_MAX)
		return -1;

	if (dcdc_id == LC1160_DCDC1) {
		ret = lc1160_dcdc1_vout_set(vout_id, mv);
	} else if (dcdc_id == LC1160_DCDC2) {
		ret = lc1160_dcdc2_vout_set(vout_id, mv);
	} else if (dcdc_id == LC1160_DCDC3) {
		ret = lc1160_dcdc3_vout_set(vout_id, mv);
	} else if (dcdc_id == LC1160_DCDC4) {
		ret = lc1160_dcdc4_vout_set(vout_id, mv);
	} else if (dcdc_id == LC1160_DCDC5) {
		ret = lc1160_dcdc5_vout_set(vout_id, mv);
	} else if (dcdc_id == LC1160_DCDC6) {
		ret = lc1160_dcdc6_vout_set(vout_id, mv);
	}

	return ret;
}

static void lc1160_rtc_alarm_disable(u8 id)
{
	if(id == 0) {
		lc1160_reg_write(LC1160_REG_RTC_AL1_SEC, 0);
		lc1160_reg_write(LC1160_REG_RTC_AL1_MIN, 0);
		lc1160_reg_write(LC1160_REG_RTC_AL1_HOUR, 0);
		lc1160_reg_write(LC1160_REG_RTC_AL1_DAY, 0);
		lc1160_reg_write(LC1160_REG_RTC_AL1_MONTH, 0);
		lc1160_reg_write(LC1160_REG_RTC_AL1_YEAR, 0);
	} else {
		lc1160_reg_write(LC1160_REG_RTC_AL2_SEC, 0);
		lc1160_reg_write(LC1160_REG_RTC_AL2_MIN, 0);
		lc1160_reg_write(LC1160_REG_RTC_AL2_HOUR, 0);
		lc1160_reg_write(LC1160_REG_RTC_AL2_DAY, 0);
		lc1160_reg_write(LC1160_REG_RTC_AL2_MONTH, 0);
		lc1160_reg_write(LC1160_REG_RTC_AL2_YEAR, 0);
	}
}

static u16 lc1160_read_adc(u8 val, u16 *out)
{
	u8 data0 = 0, data1 = 0;
	u8 ad_state = 0, timeout;

	/* Enable ADC power */
	lc1160_reg_bit_write(LC1160_REG_ADCCR1, LC1160_REG_BITMASK_LDOADCEN, 0x01);

	/* Set ADC channle */
	lc1160_reg_bit_write(LC1160_REG_ADCCR2,
	                         LC1160_REG_BITMASK_ADSEL, (u8)val);

	/* Set ADC mode && formate */
	lc1160_reg_bit_write(LC1160_REG_ADCCR2,
	                         LC1160_REG_BITMASK_ADMODE, 0x00); /* not continue mode */
	lc1160_reg_bit_write(LC1160_REG_ADCCR1,
	                         LC1160_REG_BITMASK_ADFORMAT, 0x00); /* 12bit conver */

	/* Enable ADC */
	lc1160_reg_bit_write(LC1160_REG_ADCCR1,
	                         LC1160_REG_BITMASK_ADCEN, 0x01);

	/* Start ADC */
	lc1160_reg_bit_write(LC1160_REG_ADCCR1,
	                         LC1160_REG_BITMASK_ADSTART, 0x01);
	timeout = 3;
	do {
		udelay(50); //1TODO: Need check or change to wait implement
		timeout--;
		lc1160_reg_read(LC1160_REG_ADCCR1, &ad_state);
		ad_state = !!(ad_state & LC1160_REG_BITMASK_ADEND);
	} while(ad_state != 1 && timeout != 0);

	if (timeout == 0) {
		printf("ADC convert timeout!\n");
		data0 = data1 = 0xff; /* Indicate ADC convert failed */
	} else {
		lc1160_reg_read(LC1160_REG_ADCDAT0, &data0); /* Low  4 bit */
		lc1160_reg_read(LC1160_REG_ADCDAT1, &data1); /* High 8 bit */
	}
	*out = ((u16)data1 << 4) | ((u16)data0);

	/* Disable ADC */
	lc1160_reg_bit_write(LC1160_REG_ADCCR1, LC1160_REG_BITMASK_LDOADCEN, 0x00);

	return 0;
}

static int lc1160_battery_adc2voltage(void)
{
	u16 data = 0;

	lc1160_read_adc(0, &data);

	return (int)(((int)data * 2800 * 4) / 4096);
}

static int lc1160_voltage_get(void)
{
	int cnt = 0;
	int voltage = 0;
	int i;

	for (i = 0; i < 3; i++) {
		int temp = lc1160_battery_adc2voltage();
		if ((temp > 2500) && (temp < 5500)) {
			voltage += temp;
			cnt++;
		}
	}
	if (cnt == 0) {
		printf("Get voltage failed!\n");
		return -1;
	}
	return voltage / cnt;
}

static int lc1160_charger_state_get(void)
{
	u8 state = 0;
	lc1160_reg_read(LC1160_REG_PCST, &state);
	return !!(state & LC1160_REG_BITMASK_ADPIN);
}

static int lc1160_battery_voltage_check(struct pmic_info *info)
{
	int voltage;
	int ret = 0;
	int retries = 1000;

	special_charger_charging_init(1);

	voltage = lc1160_voltage_get();
	if (voltage < CONFIG_CHARGER_POWER_OFF_LEVEL) {
		led_set_blink(RED, 1000, 1000);
		led_set_brightness(RED, LED_FULL);
	}

	do {
		voltage = lc1160_voltage_get();
		printf("Main battery voltage = %d mv\n", voltage);
		if (lc1160_charger_state_get()) {
			special_charger_charging_init(0);
			if (voltage < CONFIG_CHARGER_POWER_OFF_LEVEL) {
				if (!(retries % 20))
					printf("Low battery voltage and USB plug in, voltage %d, retries = %d\n", voltage, retries);
				mdelay(1000); /* 200msec */
			} else {
				printf("Meet low battery voltage and USB plug in, voltage %d\n", voltage);
				ret = -1;
			}
		} else {
			ret = -1;
			if (voltage < CONFIG_NO_CHARGER_POWER_OFF_LEVEL) {
				printf("Low battery voltage and USB plug out, goto power off\n");
				lc1160_rtc_alarm_disable(PMIC_RTC_ALARM_POWEROFF);
				led_set_brightness(RED, LED_OFF);
				pmic_power_off();
			}
		}
		if (0 == retries)
			printf("Low battery voltage but timeout!\n");
	} while (!ret && retries--);
	led_set_brightness(RED, LED_OFF);

	return 0;
}


static int lc1160_adc_adc2voltage(int ch)
{
	u16 data = 0;
	int gain = ch? 2800 : (2800*4);

	lc1160_read_adc(ch, &data);

	return (int)(((int)data * gain) / 4096);
}

static int lc1160_adc_conversion(int channel_no, int max, int min)
{
	int cnt = 0;
	int voltage = 0;
	int i;
	int temp = 0;

	for (i = 0; i < 3; i++) {
		temp = lc1160_adc_adc2voltage(channel_no);
		if ((temp > min) && (temp < max)) {
			voltage += temp;
			cnt++;
		}
	}
	if (cnt == 0) {
		printf("Get voltage failed!ch:%d vol:%d{%d-%d}\n",channel_no, temp, min, max);
		return -1;
	}
	return voltage / cnt;
}

/*
 * <0 fail
 * >0 value
 */
int pmic_get_adc_conversion(int channel_no)
{
	int ret;
	int max=0,min=0;

	switch (channel_no) {
	case 0:
		max = 5500;
		min = 2500;
		break;
	case 1:
	case 2:
	case 3:
		max = 5000;
		min = 0;
		break;
	default:
		max = 0;
		min = 0;
		break;
	}

	ret = lc1160_adc_conversion(channel_no, max, min);

	return ret;
}

static int lc1160_dcdc1_init(struct pmic_info *info)
{
	lc1160_dcdc_vout_set(LC1160_DCDC1, 0, info->buck1_vout1);
	lc1160_dcdc_vout_set(LC1160_DCDC1, 1, info->buck1_vout0);

	return 0;
}

static int lc1160_dcdc2_init(struct pmic_info *info)
{
	int ret;
	u32 val;

#if (CONFIG_USE_EXT_BUCK2 == 0)
#if (BUCK2_VOUT_SEL == 3)
	lc1160_dcdc_vout_set(LC1160_DCDC2, 1, info->buck2_vout1);
	lc1160_dcdc_vout_set(LC1160_DCDC2, 2, info->buck2_vout2);
	lc1160_dcdc_vout_set(LC1160_DCDC2, 3, info->buck2_vout3);
#else
	/* In one line dvfs mode, set all vout to maximum value. */
	lc1160_dcdc_vout_set(LC1160_DCDC2, 0, info->buck2_vout2);
	lc1160_dcdc_vout_set(LC1160_DCDC2, 1, info->buck2_vout2);
	lc1160_dcdc_vout_set(LC1160_DCDC2, 2, info->buck2_vout2);
#endif

	val = __raw_readl(DDR_PWR_PMU_CTL);
	val = (val & ~(0x3<< VOL_SW_A7))|(BUCK2_VOUT_SEL << VOL_SW_A7);
	val = (val & ~(0x3<< VOL_TW_A7))|(0x0 << VOL_TW_A7);
	__raw_writel(val, DDR_PWR_PMU_CTL);

	ret = lc1160_reg_bit_write(LC1160_REG_DC2DVSCR, LC1160_REG_BITMASK_DC2_DVS_SEL, 1);
	if (ret) {
		printf("[LC1160]set dcdc2 mode failed : %d!\n", ret);
		return -1;
	}

#if (BUCK2_VOUT_SEL == 3)
	lc1160_dcdc_vout_set(LC1160_DCDC2, 0, info->buck2_vout0);
#endif

#else
	/* Disable dcdc2. */
	ret = lc1160_reg_bit_write(LC1160_REG_DCDC2_OUT_0, LC1160_REG_BITMASK_DC2EN, 0);
	if (ret) {
		printf("[LC1160]disable dcdc2 failed : %d!\n", ret);
		return -1;
	}
#endif

	return 0;
}

static int lc1160_dcdc3_init(struct pmic_info *info)
{
	u32 val;

	lc1160_dcdc_vout_set(LC1160_DCDC3, 0, info->buck3_vout0);
	lc1160_dcdc_vout_set(LC1160_DCDC3, 1, info->buck3_vout1);
	lc1160_dcdc_vout_set(LC1160_DCDC3, 2, info->buck3_vout2);
	lc1160_dcdc_vout_set(LC1160_DCDC3, 3, info->buck3_vout3);

	val = __raw_readl(DDR_PWR_PMU_CTL);
	val = (val & ~(0x3<< VOL_SW_GPU))|(0x3 << VOL_SW_GPU);
	val = (val & ~(0x3<< VOL_TW_GPU))|(0x0 << VOL_TW_GPU);
	__raw_writel(val, DDR_PWR_PMU_CTL);

	return 0;
}

static int lc1160_dcdc4_init(struct pmic_info *info)
{
	lc1160_dcdc_vout_set(LC1160_DCDC4, 0, info->buck4_vout1);
	lc1160_dcdc_vout_set(LC1160_DCDC4, 1, info->buck4_vout0);

	return 0;
}

static int lc1160_dcdc5_init(struct pmic_info *info)
{
	lc1160_dcdc_vout_set(LC1160_DCDC5, 0, info->buck5_vout1);
	lc1160_dcdc_vout_set(LC1160_DCDC5, 1, info->buck5_vout0);

	return 0;
}

static int lc1160_dcdc6_init(struct pmic_info *info)
{
	lc1160_dcdc_vout_set(LC1160_DCDC6, 0, info->buck6_vout1);
	lc1160_dcdc_vout_set(LC1160_DCDC6, 1, info->buck6_vout0);

	return 0;
}

static int lc1160_dcdc_pwm_set(void)
{
	/* auto pwm/pfm mode. */
	lc1160_reg_bit_write(LC1160_REG_DC1CR, LC1160_REG_BITMASK_DC1_PWM_FORCE, 0x0);
	lc1160_reg_bit_write(LC1160_REG_DC2CR, LC1160_REG_BITMASK_DC2_PWM_FORCE, 0x0);
	lc1160_reg_bit_write(LC1160_REG_DC3CR, LC1160_REG_BITMASK_DC3_PWM_FORCE, 0x0);
	lc1160_reg_bit_write(LC1160_REG_DC4CR, LC1160_REG_BITMASK_DC4_PWM_FORCE, 0x0);
	lc1160_reg_bit_write(LC1160_REG_DC5CR, LC1160_REG_BITMASK_DC5_PWM_FORCE, 0x0);
	lc1160_reg_bit_write(LC1160_REG_DC6CR, LC1160_REG_BITMASK_DC6_PWM_FORCE, 0x0);
	lc1160_reg_bit_write(LC1160_REG_DC9CR, LC1160_REG_BITMASK_DC9_PWM_FORCE, 0x0);

	return 0;
}

static int lc1160_misc_init(void)
{
	/* RF power register could be write by SPI &IIC */
	lc1160_reg_bit_write(LC1160_REG_SPICR, LC1160_REG_BITMASK_SPI_IIC_EN, 0x01);
	/* low power mode enable when system in sleep mode */
	lc1160_reg_bit_write(LC1160_REG_DCDC1_OUT_1, LC1160_REG_BITMASK_DC1SLP, 0x1);
	lc1160_reg_bit_write(LC1160_REG_DCDC2_OUT_1, LC1160_REG_BITMASK_DC2SLP, 0x1);
	lc1160_reg_bit_write(LC1160_REG_DCDC3_OUT_1, LC1160_REG_BITMASK_DC3SLP, 0x1);
	lc1160_reg_bit_write(LC1160_REG_DCDC4_OUT_1, LC1160_REG_BITMASK_DC4SLP, 0x1);
	lc1160_reg_bit_write(LC1160_REG_DCDC5_OUT_1, LC1160_REG_BITMASK_DC5SLP, 0x1);
	lc1160_reg_bit_write(LC1160_REG_DCDC6_OUT_1, LC1160_REG_BITMASK_DC6SLP, 0x1);
	lc1160_reg_bit_write(LC1160_REG_DCDC9_OUT_1, LC1160_REG_BITMASK_DC9SLP, 0x1);
	/* enable dcdc2 when sleep.*/
	lc1160_reg_bit_write(LC1160_REG_DCDC2_OUT_1, LC1160_REG_BITMASK_DC2EN, 0x01);
	/* enable dcdc9 when sleep.*/
	lc1160_reg_bit_write(LC1160_REG_DCDC9_OUT_1, LC1160_REG_BITMASK_DC9EN, 0x01);

	/* Close DCDC5.*/
	lc1160_reg_bit_write(LC1160_REG_DCDC5_OUT_0, LC1160_REG_BITMASK_DC5EN, 0x00);
	lc1160_reg_bit_write(LC1160_REG_DCDC5_OUT_1, LC1160_REG_BITMASK_DC5EN, 0x00);
	/* Normal open ALDO8.*/
	lc1160_reg_bit_write(LC1160_REG_LDOA8CR, LC1160_REG_BITMASK_LDOA8EN, 0x01);
	lc1160_reg_bit_write(LC1160_REG_DCLDOCONEN, LC1160_REG_BITMASK_LDOA8CONEN, 0x00);
	/* close ALDO3.*/
	lc1160_reg_bit_write(LC1160_REG_LDOA3CR, LC1160_REG_BITMASK_LDOA3EN, 0x00);
	/* Setting Adapter/USB CC current = 500 ma.*/
	lc1160_reg_bit_write(LC1160_REG_CHGCSET, 0x0f, 0x05);

	return 0;
}

static int lc1160_init(struct pmic_info *info)
{
	unsigned char val, data;
	unsigned char mask;
	int reboot_type;
	int startup_type;
	int power_on_type;
	int ret;

	/*Shutdown by KEYON disable*/
	lc1160_reg_read(LC1160_REG_SHDN_EN, &val);
	lc1160_reg_write(LC1160_REG_SHDN_EN, (val & 0xef));

	/*Save shutdown reason*/
	lc1160_reg_read(LC1160_REG_RTC_DATA1, &data);
	if(!(data & LC1160_REG_BITMASK_SHDNFLAG)) {
		lc1160_reg_read(LC1160_REG_SHDN_STATUS, &val);
		lc1160_reg_write(LC1160_REG_RTC_DATA1, (val | LC1160_REG_BITMASK_SHDNFLAG));
	}

	ret = lc1160_reg_read(LC1160_REG_STARTUP_STATUS, &val);
	if (ret) {
		printf("[LC1160]read startup register failed : %d!\n", ret);
		return -1;
	}

	startup_type = val;

	/* Get reboot type. */
	mask = PMIC_RTC_ALARM_REBOOT ? LC1160_REG_BITMASK_ALARM2_STARTUP : LC1160_REG_BITMASK_ALARM1_STARTUP;
	ret = lc1160_reg_read(LC1160_REG_RTC_DATA2, &val);
	if (ret) {
		printf("[LC1160]read reboot register failed : %d!\n", ret);
		return -1;
	}

	reboot_type = (val & 0x7f);
	printf("lc1160_init read_reboot_reson->0x%x\n",reboot_type);
	if ((reboot_type == REBOOT_NONE) && (startup_type & mask)) {
		lc1160_rtc_alarm_disable(PMIC_RTC_ALARM_REBOOT);
		pmic_power_off();
	}

	if (startup_type & LC1160_REG_BITMASK_ALARM1_STARTUP)
		power_on_type = PU_REASON_RTC_ALARM;
	else if (startup_type & LC1160_REG_BITMASK_ALARM2_STARTUP)
		power_on_type = PU_REASON_RTC_ALARM;
	else if (startup_type & LC1160_REG_BITMASK_HF_PWR_STARTUP)
		power_on_type = PU_REASON_LOW_PWR_RESET;
	else if (startup_type & LC1160_REG_BITMASK_RSTINB_STARTUP)
		power_on_type = PU_REASON_HARDWARE_RESET;
	else if (startup_type & LC1160_REG_BITMASK_ADPIN_STARTUP)
		power_on_type = PU_REASON_USB_CHARGER;
	else if (startup_type & LC1160_REG_BITMASK_KEYON_STARTUP)
		power_on_type = PU_REASON_PWR_KEY_PRESS;
	else
		power_on_type = PU_REASON_PWR_KEY_PRESS;

	if (!startup_type) {
		ret = lc1160_reg_read(LC1160_REG_SHDN_STATUS, &val);
		if (ret) {
			printf("[LC1160]read shutdown register failed : %d!\n", ret);
			return -1;
		}
		if (LC1160_REG_BITMASK_RSTINB_SHDN & val)
			power_on_type = PU_REASON_HARDWARE_RESET;
	}

	reboot_type &= REBOOT_REASON_MASK;
	if (reboot_type == REBOOT_RTC_ALARM)
		power_on_type = PU_REASON_RTC_ALARM;
	else if (reboot_type == REBOOT_POWER_KEY)
		power_on_type = PU_REASON_PWR_KEY_PRESS;
	else if (reboot_type == REBOOT_RECOVERY)
		power_on_type = PU_REASON_REBOOT_RECOVERY;
	else if (reboot_type == REBOOT_FOTA)
		power_on_type = PU_REASON_REBOOT_FOTA;
	else if (reboot_type == REBOOT_CRITICAL)
		power_on_type = PU_REASON_REBOOT_CRITICAL;
	else if (reboot_type == REBOOT_UNKNOWN)
		power_on_type = PU_REASON_REBOOT_UNKNOWN;
	else if (reboot_type == REBOOT_NORMAL)
		power_on_type = PU_REASON_REBOOT_NORMAL;

	info->startup_type = startup_type;
	info->power_on_type = power_on_type;
	info->reboot_type = reboot_type;

	printf("startup_type=0x%x,power_on_type=0x%x,reboot_type=0x%x\n",startup_type,power_on_type,reboot_type);

	lc1160_misc_init();
	lc1160_dcdc_pwm_set();
	lc1160_dcdc1_init(info);
	lc1160_dcdc2_init(info);
	lc1160_dcdc3_init(info);
	lc1160_dcdc4_init(info);
	lc1160_dcdc5_init(info);
	lc1160_dcdc6_init(info);
#if	defined(CONFIG_COMIP_TARGETLOADER)
	/* DLDO6 open */
	lc1160_reg_bit_write(LC1160_REG_LDOD6CR, LC1160_REG_BITMASK_LDOD6EN, 0x1);
	/* ALDO12 open */
	lc1160_reg_bit_write(LC1160_REG_LDOA12CR, LC1160_REG_BITMASK_LDOA12EN, 0x1);
#endif
	return 0;
}

void  comip_usb_power_on(void)
{
	/* DLDO6 open */
	lc1160_reg_bit_write(LC1160_REG_LDOD6CR, LC1160_REG_BITMASK_LDOD6EN, 0x1);
	/* ALDO12 open */
	lc1160_reg_bit_write(LC1160_REG_LDOA12CR, LC1160_REG_BITMASK_LDOA12EN, 0x1);
}

static int lc1160_power_on_key_check(struct pmic_info *info)
{
	unsigned char val;
	int ret;

	/* Check power key. */
	if ((info->power_on_type == PU_REASON_PWR_KEY_PRESS)
		&& (info->reboot_type != REBOOT_POWER_KEY)){
		ret = lc1160_reg_read(LC1160_REG_PCST, &val);
		if (ret)
			printf("[LC1160]read present_pmu register failed : %d!\n", ret);

		/* Detect charger in. */
		if(!ret && !(val & LC1160_REG_BITMASK_KONMON)) {
			if (!ret && !(val & LC1160_REG_BITMASK_ADPIN))
				pmic_power_off();
		}
	}

    return 0;
}

static  void  lc1160_reboot_reason_set(u8 type)
{
	lc1160_reg_write(LC1160_REG_RTC_DATA2, type);
}

int lc1160_probe(struct pmic_info *info)
{
	info->init = lc1160_init;
	info->reboot_reason_set=lc1160_reboot_reason_set;
	info->battery_voltage_check = lc1160_battery_voltage_check;
	info->power_on_key_check = lc1160_power_on_key_check;
	info->vibrator_enable = lc1160_vibrator_enable;
	info->lcdio_enable = lc1160_lcdio_enable;
	info->lcdcore_enable = lc1160_lcdcore_enable;

	return 0;
}

#endif /* CONFIG_COMIP_LC1160*/
