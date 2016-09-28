
#include <common.h>

#if (defined(CONFIG_PMIC_LC1132) || defined(CONFIG_PMIC_AUTO))

/* LC1132 I2C address. */
#define LC1132_I2C_ADDR				(0x33)

/* LC1132 register address. */
#define LC1132_REG_STARTUP_STATUS		(0x36)
#define LC1132_REG_DCEN				(0x00)
#define LC1132_REG_DCDC1_OUT_0			(0x01)
#define LC1132_REG_DCDC1_OUT_1			(0x02)
#define LC1132_REG_DCDC2_OUT_0			(0x03)
#define LC1132_REG_DCDC2_OUT_1			(0x04)
#define LC1132_REG_DCDC2_OUT_2			(0x05)
#define LC1132_REG_DCDC2_OUT_3			(0x06)
#define LC1132_REG_DCDC3_OUT_0			(0x07)
#define LC1132_REG_DCDC3_OUT_1			(0x08)
#define LC1132_REG_DCDC4_OUT_0			(0x09)
#define LC1132_REG_DCDC4_OUT_1			(0x0a)
#define LC1132_REG_DCMODE			(0x0b)
#define LC1132_REG_LDOAEN         		(0x0c)
#define LC1132_REG_LDOAONEN			(0x0e)
#define LC1132_REG_LDOAOCPEN			(0x21)
#define LC1132_REG_LDODOCPEN1			(0x22)
#define LC1132_REG_LDODOCPEN2			(0x23)
#define LC1132_REG_CHGCSET				(0x32)
#define LC1132_REG_PCST				(0x35)
#define LC1132_REG_SHDN_STATUS			(0x37)
#define LC1132_REG_ADCLDOEN			(0x38)
#define LC1132_REG_ADCCR			(0x39)
#define LC1132_REG_ADCCMD			(0x3a)
#define LC1132_REG_ADCEN			(0x3b)
#define LC1132_REG_ADCDAT0			(0x3c)
#define LC1132_REG_ADCDAT1			(0x3d)
#define LC1132_REG_ADCFORMAT			(0x3e)
#define LC1132_REG_LDOAFASTDISC           	(0x4e)
#define LC1132_REG_RTC_AL1_SEC     		(0x6d)
#define LC1132_REG_RTC_AL1_MIN     		(0x6e)
#define LC1132_REG_RTC_AL2_SEC     		(0x6f)
#define LC1132_REG_RTC_AL2_MIN     		(0x70)
#define LC1132_REG_RTC_AL2_HOUR    		(0x71)
#define LC1132_REG_RTC_AL2_DAY     		(0x72)
#define LC1132_REG_RTC_AL2_MONTH   		(0x73)
#define LC1132_REG_RTC_AL2_YEAR    		(0x74)
#define LC1132_REG_RTC_AL_UPDATE   		(0x75)
#define LC1132_REG_RTC_AL1_HOUR    		(0x7c)
#define LC1132_REG_RTC_AL1_DAY    	    	(0x7d)
#define LC1132_REG_RTC_AL1_MONTH    		(0x7e)
#define LC1132_REG_RTC_AL1_YEAR    		(0x7f)

/* LC1132_REG_UP_STATE */
#define LC1132_REG_BITMASK_RSTINB_STARTUP 	(0x20)
#define LC1132_REG_BITMASK_ALARM2_STARTUP 	(0x10)
#define LC1132_REG_BITMASK_ALARM1_STARTUP 	(0x08)
#define LC1132_REG_BITMASK_ADPIN_STARTUP 	(0x04)
#define LC1132_REG_BITMASK_HF_PWR_STARTUP	(0x02)
#define LC1132_REG_BITMASK_KEYON_STARTUP 	(0x01)

/* LC1132_REG_SHDN_STATE */
#define LC1132_REG_BITMASK_RSTINB_SHDN 	(0x20)

/* LC1132_REG_DCDC_EVS_SEL */
#define LC1132_REG_BITMASK_DCDC2_DVS_SEL	(0x30)
#define LC1132_REG_BITMASK_DCDC2_EN		(0x02)
#define LC1132_REG_BITMASK_DCDC1_EN		(0x01)

/* ADCCR: A/D Converter Control Register */
#define LC1132_REG_BITMASK_ADMODE		(0x10)
#define LC1132_REG_BITMASK_ADSEL		(0x03)

/* ADCCMD: A/D Converter Command Register */
#define LC1132_REG_BITMASK_ADEND		(0x10)
#define LC1132_REG_BITMASK_ADSTART		(0x01)

/* PCST: Power-related Status Register */
#define LC1132_REG_BITMASK_KONMON		(0x20)
#define LC1132_REG_BITMASK_ADPIN		(0x01)

enum {
	LC1132_DCDC1 = 0,
	LC1132_DCDC2,
	LC1132_DCDC3,
	LC1132_DCDC4,
	LC1132_DCDC_MAX,
};

enum {
	LC1132_DCDC_VOUT0 = 0,
	LC1132_DCDC_VOUT1,
	LC1132_DCDC_VOUT2,
	LC1132_DCDC_VOUT3,
	LC1132_DCDC_VOUT_MAX,
};

static int lc1132_reg_read(u8 reg, u8 *value)
{
	return pmic_reg_read(LC1132_I2C_ADDR, reg, value);
}

static int lc1132_reg_write(u8 reg, u8 value)
{
	return pmic_reg_write(LC1132_I2C_ADDR, reg, value);
}

static int lc1132_reg_bit_write(u8 reg, u8 mask, u8 value)
{
	return pmic_reg_bit_write(LC1132_I2C_ADDR, reg, mask, value);
}

static int lc1132_dcdc1_vout_set(u8 vout_id, int mv)
{
	const int lc1132_dcdc1_vout[] = {
		750, 775, 800, 825, 850, 875, 900, 925, 950, 975, 1000, 1025, 1050, 1075, 1100,
		1125, 1150, 1175, 1200, 1225, 1250, 1275, 1300, 1325, 1350, 1375, 1400
	};
	u8 reg;
	u8 i;

	if (vout_id >= LC1132_DCDC_VOUT2)
		return -1;

	for (i = 0; i < ARRAY_SIZE(lc1132_dcdc1_vout); i++) {
		if (lc1132_dcdc1_vout[i] == mv)
			break;
	}

	if (i == ARRAY_SIZE(lc1132_dcdc1_vout))
		return -1;

	if (vout_id == LC1132_DCDC_VOUT0) {
		reg = LC1132_REG_DCDC1_OUT_0;
	} else if (vout_id == LC1132_DCDC_VOUT1) {
		reg = LC1132_REG_DCDC1_OUT_1;
	}

	return lc1132_reg_write(reg, i);
}

static int lc1132_dcdc2_vout_set(u8 vout_id, int mv)
{
	const int lc1132_dcdc2_vout[] = {
		750, 775, 800, 825, 850, 875, 900, 925, 950, 975, 1000, 1025, 1050, 1075, 1100,
		1125, 1150, 1175, 1200, 1225, 1250, 1275, 1300, 1325, 1350, 1375, 1400
	};
	u8 reg;
	u8 i;

	if (vout_id >= LC1132_DCDC_VOUT_MAX)
		return -1;

	for (i = 0; i < ARRAY_SIZE(lc1132_dcdc2_vout); i++) {
		if (lc1132_dcdc2_vout[i] == mv)
			break;
	}

	if (i == ARRAY_SIZE(lc1132_dcdc2_vout))
		return -1;

	if (vout_id == LC1132_DCDC_VOUT0) {
		reg = LC1132_REG_DCDC2_OUT_0;
	} else if (vout_id == LC1132_DCDC_VOUT1) {
		reg = LC1132_REG_DCDC2_OUT_1;
	} else if (vout_id == LC1132_DCDC_VOUT2) {
		reg = LC1132_REG_DCDC2_OUT_2;
	} else if (vout_id == LC1132_DCDC_VOUT3) {
		reg = LC1132_REG_DCDC2_OUT_3;
	}

	return lc1132_reg_write(reg, i);
}

static int lc1132_dcdc3_vout_set(u8 vout_id, int mv)
{
	const int lc1132_dcdc3_vout[] = {
		1050, 1100, 1150, 1200, 1210, 1220, 1230, 1240,
		1250, 1350, 1500, 1600, 1650, 1700, 1750, 1800
	};
	u8 reg;
	u8 i;

	if (vout_id >= LC1132_DCDC_VOUT2)
		return -1;

	for (i = 0; i < ARRAY_SIZE(lc1132_dcdc3_vout); i++) {
		if (lc1132_dcdc3_vout[i] == mv)
			break;
	}

	if (i == ARRAY_SIZE(lc1132_dcdc3_vout))
		return -1;

	if (vout_id == LC1132_DCDC_VOUT0) {
		reg = LC1132_REG_DCDC3_OUT_0;
	} else if (vout_id == LC1132_DCDC_VOUT1) {
		reg = LC1132_REG_DCDC3_OUT_1;
	}

	return lc1132_reg_write(reg, i);
}

static int lc1132_dcdc4_vout_set(u8 vout_id, int mv)
{
	const int lc1132_dcdc4_vout[] = {
		900, 950, 1000, 1050, 1100, 1150, 1200, 1210,
		1220, 1230, 1240, 1250, 1350, 1500, 1700, 1800
	};
	u8 reg;
	u8 i;

	if (vout_id >= LC1132_DCDC_VOUT2)
		return -1;

	for (i = 0; i < ARRAY_SIZE(lc1132_dcdc4_vout); i++) {
		if (lc1132_dcdc4_vout[i] == mv)
			break;
	}

	if (i == ARRAY_SIZE(lc1132_dcdc4_vout))
		return -1;

	if (vout_id == LC1132_DCDC_VOUT0) {
		reg = LC1132_REG_DCDC4_OUT_0;
	} else if (vout_id == LC1132_DCDC_VOUT1) {
		reg = LC1132_REG_DCDC4_OUT_1;
	}

	return lc1132_reg_write(reg, i);
}

static int lc1132_dcdc_vout_set(u8 dcdc_id, u8 vout_id, int mv)
{
	int ret;

	if (dcdc_id >= LC1132_DCDC_MAX)
		return -1;

	if (dcdc_id == LC1132_DCDC1) {
		ret = lc1132_dcdc1_vout_set(vout_id, mv);
	} else if (dcdc_id == LC1132_DCDC2) {
		ret = lc1132_dcdc2_vout_set(vout_id, mv);
	} else if (dcdc_id == LC1132_DCDC3) {
		ret = lc1132_dcdc3_vout_set(vout_id, mv);
	} else if (dcdc_id == LC1132_DCDC4) {
		ret = lc1132_dcdc4_vout_set(vout_id, mv);
	}

	return ret;
}

static void lc1132_rtc_alarm_disable(u8 id)
{
	if(id == 0) {
		lc1132_reg_write(LC1132_REG_RTC_AL1_SEC, 0);
		lc1132_reg_write(LC1132_REG_RTC_AL1_MIN, 0);
		lc1132_reg_write(LC1132_REG_RTC_AL1_HOUR, 0);
		lc1132_reg_write(LC1132_REG_RTC_AL1_DAY, 0);
		lc1132_reg_write(LC1132_REG_RTC_AL1_MONTH, 0);
		lc1132_reg_write(LC1132_REG_RTC_AL1_YEAR, 0);
	} else {
		lc1132_reg_write(LC1132_REG_RTC_AL2_SEC, 0);
		lc1132_reg_write(LC1132_REG_RTC_AL2_MIN, 0);
		lc1132_reg_write(LC1132_REG_RTC_AL2_HOUR, 0);
		lc1132_reg_write(LC1132_REG_RTC_AL2_DAY, 0);
		lc1132_reg_write(LC1132_REG_RTC_AL2_MONTH, 0);
		lc1132_reg_write(LC1132_REG_RTC_AL2_YEAR, 0);
	}
}

static u16 lc1132_read_adc(u8 val, u16 *out)
{
	u8 data0 = 0, data1 = 0;
	u8 ad_state = 0, timeout;

	/* Enable ADC power */
	lc1132_reg_write(LC1132_REG_ADCLDOEN, 0x01);

	/* Set ADC channle */
	lc1132_reg_bit_write(LC1132_REG_ADCCR,
	                         LC1132_REG_BITMASK_ADSEL, (u8)val);

	/* Set ADC mode && formate */
	lc1132_reg_bit_write(LC1132_REG_ADCCR,
	                         LC1132_REG_BITMASK_ADMODE, 0x00); /* not continue mode */
	lc1132_reg_write(LC1132_REG_ADCFORMAT, 0x00); /* 12bit conver */

	/* Enable ADC */
	lc1132_reg_write(LC1132_REG_ADCEN, 0x01);

	/* Start ADC */
	lc1132_reg_bit_write(LC1132_REG_ADCCMD,
	                         LC1132_REG_BITMASK_ADSTART, 0x01);
	timeout = 3;
	do {
		udelay(50); //1TODO: Need check or change to wait implement
		timeout--;
		lc1132_reg_read(LC1132_REG_ADCCMD, &ad_state);
		ad_state = !!(ad_state & LC1132_REG_BITMASK_ADEND);
	} while(ad_state != 1 && timeout != 0);

	if (timeout == 0) {
		printf("ADC convert timeout!\n");
		data0 = data1 = 0xff; /* Indicate ADC convert failed */
	} else {
		lc1132_reg_read(LC1132_REG_ADCDAT0, &data0); /* Low  4 bit */
		lc1132_reg_read(LC1132_REG_ADCDAT1, &data1); /* High 8 bit */
	}
	*out = ((u16)data1 << 4) | ((u16)data0);

	/* Disable ADC */
	lc1132_reg_write(LC1132_REG_ADCLDOEN, 0x00);

	return 0;
}

static int lc1132_battery_adc2voltage(void)
{
	u16 data = 0;

	lc1132_read_adc(0, &data);

	return (int)(((int)data * 2800 * 4) / 4096);
}

static int lc1132_voltage_get(void)
{
	int cnt = 0;
	int voltage = 0;
	int i;

	for (i = 0; i < 3; i++) {
		int temp = lc1132_battery_adc2voltage();
		if ((temp > 2500) && (temp < 4500)) {
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

static int lc1132_charger_state_get(void)
{
	u8 state = 0;
	lc1132_reg_read(LC1132_REG_PCST, &state);
	return !!(state & LC1132_REG_BITMASK_ADPIN);
}

static int lc1132_battery_voltage_check(struct pmic_info *info)
{
	int voltage;
	int ret = 0;
	int retries = 1000;

	do {
		voltage = lc1132_voltage_get();
		if (lc1132_charger_state_get()) {
			if (voltage < CONFIG_CHARGER_POWER_OFF_LEVEL) {
				if (!(retries % 100))
					printf("Low battery voltage and USB plug in, voltage %d, retries = %d\n", voltage, retries);
				mdelay(200); /* 200msec */
			} else {
				printf("Meet low battery voltage and USB plug in, voltage %d\n", voltage);
				ret = -1;
			}
		} else {
			ret = -1;
			if (voltage < CONFIG_NO_CHARGER_POWER_OFF_LEVEL) {
				printf("Low battery voltage and USB plug out, goto power off\n");
				lc1132_rtc_alarm_disable(PMIC_RTC_ALARM_POWEROFF);
				pmic_power_off();
			}
		}
		if (0 == retries)
			printf("Low battery voltage but timeout!\n");
	} while (!ret && retries--);

	return 0;
}

static int lc1132_dcdc1_init(struct pmic_info *info)
{
	lc1132_dcdc_vout_set(LC1132_DCDC1, 0, info->buck1_vout1);
	lc1132_dcdc_vout_set(LC1132_DCDC1, 1, info->buck1_vout0);

	return 0;
}

static int lc1132_dcdc2_init(struct pmic_info *info)
{
	int ret;

#if (CONFIG_USE_EXT_BUCK2 == 0)
#if (BUCK2_VOUT_SEL == 3)
	lc1132_dcdc_vout_set(LC1132_DCDC2, 1, info->buck2_vout1);
	lc1132_dcdc_vout_set(LC1132_DCDC2, 2, info->buck2_vout2);
	lc1132_dcdc_vout_set(LC1132_DCDC2, 3, info->buck2_vout3);
#else
	/* In one line dvfs mode, set all vout to maximum value. */
	lc1132_dcdc_vout_set(LC1132_DCDC2, 0, info->buck2_vout2);
	lc1132_dcdc_vout_set(LC1132_DCDC2, 1, info->buck2_vout2);
	lc1132_dcdc_vout_set(LC1132_DCDC2, 2, info->buck2_vout2);
#endif

	REG32(AP_PWR_ARMDV_CTL) = BUCK2_VOUT_SEL;

	ret = lc1132_reg_bit_write(LC1132_REG_DCEN, LC1132_REG_BITMASK_DCDC2_DVS_SEL, 1);
	if (ret) {
		printf("[LC1132]set dcdc2 mode failed : %d!\n", ret);
		return -1;
	}

#if (BUCK2_VOUT_SEL == 3)
	lc1132_dcdc_vout_set(LC1132_DCDC2, 0, info->buck2_vout0);
#endif

#else
	/* Disable dcdc2. */
	ret = lc1132_reg_bit_write(LC1132_REG_DCEN, LC1132_REG_BITMASK_DCDC2_EN, 0);
	if (ret) {
		printf("[LC1132]disable dcdc2 failed : %d!\n", ret);
		return -1;
	}
#endif

	return 0;
}

static int lc1132_dcdc3_init(struct pmic_info *info)
{
	lc1132_dcdc_vout_set(LC1132_DCDC3, 0, info->buck3_vout1);
	lc1132_dcdc_vout_set(LC1132_DCDC3, 1, info->buck3_vout0);

	return 0;
}

static int lc1132_dcdc4_init(struct pmic_info *info)
{
	lc1132_dcdc_vout_set(LC1132_DCDC4, 0, info->buck4_vout1);
	lc1132_dcdc_vout_set(LC1132_DCDC4, 1, info->buck4_vout0);

	return 0;
}


static int lc1132_dcdc_pwm_set(void)
{
	int ret;

	/* auto pwm/pfm mode. */
	ret = lc1132_reg_write(LC1132_REG_DCMODE, 0xf0);
	if (ret) {
		printf("[LC1132]set dc mode register failed : %d!\n", ret);
		return -1;
	}
	return 0;
}

static int lc1132_misc_init(void)
{
	lc1132_reg_write(LC1132_REG_LDOAFASTDISC, 0x00);
	lc1132_reg_write(LC1132_REG_LDOAOCPEN, 0x00);
	lc1132_reg_write(LC1132_REG_LDODOCPEN1, 0x00);
	lc1132_reg_write(LC1132_REG_LDODOCPEN2, 0x00);
	lc1132_reg_write(0x53, 0x56);
	lc1132_reg_write(0x54, 0x52);
	lc1132_reg_write(0x55, 0x53);
	lc1132_reg_write(0x56, 0x53);
	/* Normal open ALDO4.*/
	lc1132_reg_bit_write(LC1132_REG_LDOAEN, 0x08, 0x01);
	lc1132_reg_bit_write(LC1132_REG_LDOAONEN, 0x08, 0x00);
	/* Setting Adapter/USB CC current = 500 ma.*/
	lc1132_reg_bit_write(LC1132_REG_CHGCSET, 0x0f, 0x04);

	return 0;
}

static int lc1132_init(struct pmic_info *info)
{
	unsigned char reg;
	unsigned char val;
	unsigned char mask;
	int reboot_type;
	int startup_type;
	int power_on_type;
	int ret;

	ret = lc1132_reg_read(LC1132_REG_STARTUP_STATUS, &val);
	if (ret) {
		printf("[LC1132]read startup register failed : %d!\n", ret);
		return -1;
	}

	startup_type = val;

	/* Get reboot type. */
	mask = PMIC_RTC_ALARM_REBOOT ? LC1132_REG_BITMASK_ALARM2_STARTUP : LC1132_REG_BITMASK_ALARM1_STARTUP;
	reg = PMIC_RTC_ALARM_POWEROFF ? LC1132_REG_RTC_AL2_SEC : LC1132_REG_RTC_AL1_SEC;
	ret = lc1132_reg_read(reg, &val);
	if (ret) {
		printf("[LC1132]read reboot register failed : %d!\n", ret);
		return -1;
	}

	reboot_type = (val & 0x7f);
	if ((reboot_type == REBOOT_NONE) && (startup_type & mask)) {
		lc1132_rtc_alarm_disable(PMIC_RTC_ALARM_REBOOT);
		pmic_power_off();
	}

	if (startup_type & LC1132_REG_BITMASK_ALARM1_STARTUP)
		power_on_type = PU_REASON_RTC_ALARM;
	else if (startup_type & LC1132_REG_BITMASK_ALARM2_STARTUP)
		power_on_type = PU_REASON_RTC_ALARM;
	else if (startup_type & LC1132_REG_BITMASK_HF_PWR_STARTUP)
		power_on_type = PU_REASON_LOW_PWR_RESET;
	else if (startup_type & LC1132_REG_BITMASK_RSTINB_STARTUP)
		power_on_type = PU_REASON_HARDWARE_RESET;
	else if (startup_type & LC1132_REG_BITMASK_ADPIN_STARTUP)
		power_on_type = PU_REASON_USB_CHARGER;
	else if (startup_type & LC1132_REG_BITMASK_KEYON_STARTUP)
		power_on_type = PU_REASON_PWR_KEY_PRESS;
	else
		power_on_type = PU_REASON_PWR_KEY_PRESS;

	if (!startup_type) {
		ret = lc1132_reg_read(LC1132_REG_SHDN_STATUS, &val);
		if (ret) {
			printf("[LC1132]read shutdown register failed : %d!\n", ret);
			return -1;
		}
		if (LC1132_REG_BITMASK_RSTINB_SHDN & val)
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

	lc1132_misc_init();
	lc1132_dcdc_pwm_set();
	lc1132_dcdc1_init(info);
	lc1132_dcdc2_init(info);
	lc1132_dcdc3_init(info);
	lc1132_dcdc4_init(info);

	return 0;
}

static int lc1132_power_on_key_check(struct pmic_info *info)
{
	unsigned char val;
	int ret;

	/* Check power key. */
	if ((info->power_on_type == PU_REASON_PWR_KEY_PRESS)
		&& (info->reboot_type != REBOOT_POWER_KEY)){
		ret = lc1132_reg_read(LC1132_REG_PCST, &val);
		if (ret)
			printf("[LC1132]read present_pmu register failed : %d!\n", ret);

		/* Detect charger in. */
		if(!ret && !(val & LC1132_REG_BITMASK_KONMON)) {
			if (!ret && !(val & LC1132_REG_BITMASK_ADPIN))
				pmic_power_off();
		}
	}
   
    return 0;
}

int lc1132_probe(struct pmic_info *info)
{
	info->init = lc1132_init;
	info->battery_voltage_check = lc1132_battery_voltage_check;
	info->power_on_key_check = lc1132_power_on_key_check;

	return 0;
}

#endif /* CONFIG_COMIP_LC1132*/
