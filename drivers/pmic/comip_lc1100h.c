
#include <common.h>

#if (defined(CONFIG_PMIC_LC1100H) || defined(CONFIG_PMIC_AUTO))

/* LC1100H I2C address. */
#define LC1100H_I2C_ADDR				(0x33)

/* LC1100H register address. */
#define LC1100H_REG_UP_STATE			(0x36)
#define LC1100H_REG_DCDC_DVS			(0x00)
#define LC1100H_REG_DCDC1_OUT_0			(0x01)
#define LC1100H_REG_DCDC1_OUT_1			(0x01)
#define LC1100H_REG_DCDC2_OUT_0			(0x02)
#define LC1100H_REG_DCDC2_OUT_1			(0x02)
#define LC1100H_REG_DCDC2_OUT_2			(0x21)
#define LC1100H_REG_DCDC2_OUT_3			(0x21)
#define LC1100H_REG_DCDC3_OUT_0			(0x03)
#define LC1100H_REG_DCDC4_OUT_0			(0x04)
#define LC1100H_REG_DC_MODE			(0x1c)
#define LC1100H_REG_PCST			(0x35)
#define LC1100H_REG_ADCLDOEN			(0x38)
#define LC1100H_REG_ADCCR			(0x39)
#define LC1100H_REG_ADCCMD			(0x3a)
#define LC1100H_REG_ADCEN			(0x3b)
#define LC1100H_REG_ADCDAT0			(0x3c)
#define LC1100H_REG_ADCDAT1			(0x3d)
#define LC1100H_REG_ADCFORMAT			(0x3e)
#define LC1100H_REG_RTC_AL1_SEC			(0x6d)
#define LC1100H_REG_RTC_AL1_MIN			(0x6e)
#define LC1100H_REG_RTC_AL1_HOUR		(0x7c)
#define LC1100H_REG_RTC_AL1_DAY			(0x7d)
#define LC1100H_REG_RTC_AL1_MONTH		(0x7e)
#define LC1100H_REG_RTC_AL1_YEAR		(0x7f)
#define LC1100H_REG_RTC_AL2_SEC			(0x6f)
#define LC1100H_REG_RTC_AL2_MIN			(0x70)
#define LC1100H_REG_RTC_AL2_HOUR		(0x71)
#define LC1100H_REG_RTC_AL2_DAY			(0x72)
#define LC1100H_REG_RTC_AL2_MONTH		(0x73)
#define LC1100H_REG_RTC_AL2_YEAR		(0x74)
#define LC1100H_REG_RTC_INT_EN			(0x76)
#define LC1100H_REG_RTC_INT_STATUS		(0x77)
#define LC1100H_REG_RTC_INT_RAW		(0x78)
#define LC1100H_REG_TESTEN			(0x57)

/* LC1100H_REG_UP_STATE */
#define LC1100H_REG_BITMASK_KEYON_STARTUP		(0x01)
#define LC1100H_REG_BITMASK_HF_PWR_STARTUP		(0x02)
#define LC1100H_REG_BITMASK_ADPIN_STARTUP		(0x04)
#define LC1100H_REG_BITMASK_ALARM_STARTUP		(0x08)
#define LC1100H_REG_BITMASK_ALARM1_WEEK			(0x7f)

/* LC1100H_RTC_INT_RAW */
#define LC1100H_REG_BITMASK_RTC_AL1_RAW			(0x20)
#define LC1100H_REG_BITMASK_RTC_AL2_RAW			(0x10)

/* LC1100H_REG_DCDC2_DVS_BITMASK . */
#define LC1100H_REG_BITMASK_DCDC2_DVS_SEL		(0x30)
#define LC1100H_REG_BITMASK_DCDC2_EN			(0x02)
#define LC1100H_REG_BITMASK_DCDC1_EN			(0x01)

/* LC1100H_REG_RTC_INT_EN_BITMASK . */
#define LC1100H_REG_BITMASK_RTC_AL1EN			(0x20)
#define LC1100H_REG_BITMASK_RTC_AL2EN			(0x10)

/* LC1100H_REG_DCDC_BITMASK . */
#define LC1100H_REG_BITMASK_DCDC1_OUT0			(0x0f)
#define LC1100H_REG_BITMASK_DCDC1_OUT1			(0xf0)
#define LC1100H_REG_BITMASK_DCDC2_OUT0			(0x0f)
#define LC1100H_REG_BITMASK_DCDC2_OUT1			(0xf0)
#define LC1100H_REG_BITMASK_DCDC2_OUT2			(0x0f)
#define LC1100H_REG_BITMASK_DCDC2_OUT3			(0xf0)
#define LC1100H_REG_BITMASK_DCDC3_OUT0			(0x07)
#define LC1100H_REG_BITMASK_DCDC4_OUT0			(0x0f)

/* ADCCR: A/D Converter Control Register */
#define LC1100H_REG_BITMASK_ADMODE			(0x10)
#define LC1100H_REG_BITMASK_ADSEL			(0x03)

/* ADCCMD: A/D Converter Command Register */
#define LC1100H_REG_BITMASK_ADEND			(0x10)
#define LC1100H_REG_BITMASK_ADSTART			(0x01)

/* PCST: Power-related Status Register */
#define LC1100H_REG_BITMASK_KONMON			(0x20)
#define LC1100H_REG_BITMASK_ADPIN			(0x01)

enum {
	LC1100H_DCDC1 = 0,
	LC1100H_DCDC2,
	LC1100H_DCDC3,
	LC1100H_DCDC4,
	LC1100H_DCDC_MAX,
};

enum {
	LC1100H_DCDC_VOUT0 = 0,
	LC1100H_DCDC_VOUT1,
	LC1100H_DCDC_VOUT2,
	LC1100H_DCDC_VOUT3,
	LC1100H_DCDC_VOUT_MAX,
};

static int lc1100h_reg_read(u8 reg, u8 *value)
{
	return pmic_reg_read(LC1100H_I2C_ADDR, reg, value);
}

static int lc1100h_reg_write(u8 reg, u8 value)
{
	return pmic_reg_write(LC1100H_I2C_ADDR, reg, value);
}

static int lc1100h_reg_bit_write(u8 reg, u8 mask, u8 value)
{
	return pmic_reg_bit_write(LC1100H_I2C_ADDR, reg, mask, value);
}

static int lc1100h_dcdc1_vout_set(u8 vout_id, int mv)
{
	const int lc1100h_dcdc1_vout[] = {
		900, 950, 1000, 1050, 1100, 1150, 1200,
		1250, 1300, 1800, 1110, 1120, 1130, 1140
	};
	u8 reg;
	u8 mask;
	u8 i;

	if (vout_id >= LC1100H_DCDC_VOUT2)
		return -1;

	for (i = 0; i < ARRAY_SIZE(lc1100h_dcdc1_vout); i++) {
		if (lc1100h_dcdc1_vout[i] == mv)
			break;
	}

	if (i == ARRAY_SIZE(lc1100h_dcdc1_vout))
		return -1;

	if (vout_id == LC1100H_DCDC_VOUT0) {
		reg = LC1100H_REG_DCDC1_OUT_0;
		mask = LC1100H_REG_BITMASK_DCDC1_OUT0;
	} else if (vout_id == LC1100H_DCDC_VOUT1) {
		reg = LC1100H_REG_DCDC1_OUT_1;
		mask = LC1100H_REG_BITMASK_DCDC1_OUT1;
	}

	return lc1100h_reg_bit_write(reg, mask, i);
}

static int lc1100h_dcdc2_vout_set(u8 vout_id, int mv)
{
	const int lc1100h_dcdc2_vout[] = {
		900, 950, 1000, 1050, 1100, 1150, 1200,1250,
		1300, 1800, 1210, 1220, 1230, 1240, 1130, 1180
	};
	u8 reg;
	u8 mask;
	u8 i;

	if (vout_id >= LC1100H_DCDC_VOUT_MAX)
		return -1;

	for (i = 0; i < ARRAY_SIZE(lc1100h_dcdc2_vout); i++) {
		if (lc1100h_dcdc2_vout[i] == mv)
			break;
	}

	if (i == ARRAY_SIZE(lc1100h_dcdc2_vout))
		return -1;

	if (vout_id == LC1100H_DCDC_VOUT0) {
		reg = LC1100H_REG_DCDC2_OUT_0;
		mask = LC1100H_REG_BITMASK_DCDC2_OUT0;
	} else if (vout_id == LC1100H_DCDC_VOUT1) {
		reg = LC1100H_REG_DCDC2_OUT_1;
		mask = LC1100H_REG_BITMASK_DCDC2_OUT1;
	} else if (vout_id == LC1100H_DCDC_VOUT2) {
		reg = LC1100H_REG_DCDC2_OUT_2;
		mask = LC1100H_REG_BITMASK_DCDC2_OUT2;
	} else if (vout_id == LC1100H_DCDC_VOUT3) {
		reg = LC1100H_REG_DCDC2_OUT_3;
		mask = LC1100H_REG_BITMASK_DCDC2_OUT2;
	}

	return lc1100h_reg_bit_write(reg, mask, i);
}

static int lc1100h_dcdc3_vout_set(u8 vout_id, int mv)
{
	const int lc1100h_dcdc3_vout[] = {
		900, 950, 1000, 1050, 1800,1150, 1200, 1250,
	};
	u8 reg;
	u8 mask;
	u8 i;

	if (vout_id >= LC1100H_DCDC_VOUT1)
		return -1;

	for (i = 0; i < ARRAY_SIZE(lc1100h_dcdc3_vout); i++) {
		if (lc1100h_dcdc3_vout[i] == mv)
			break;
	}

	if (i == ARRAY_SIZE(lc1100h_dcdc3_vout))
		return -1;

	reg = LC1100H_REG_DCDC3_OUT_0;
	mask = LC1100H_REG_BITMASK_DCDC3_OUT0;

	return lc1100h_reg_bit_write(reg, mask, i);
}

static int lc1100h_dcdc4_vout_set(u8 vout_id, int mv)
{
	const int lc1100h_dcdc4_vout[] = {
		1000, 1050, 1100, 1150, 1500, 1250,
		1350, 1200, 1210, 1220, 1230, 1240
	};
	u8 reg;
	u8 mask;
	u8 i;

	if (vout_id >= LC1100H_DCDC_VOUT1)
		return -1;

	for (i = 0; i < ARRAY_SIZE(lc1100h_dcdc4_vout); i++) {
		if (lc1100h_dcdc4_vout[i] == mv)
			break;
	}

	if (i == ARRAY_SIZE(lc1100h_dcdc4_vout))
		return -1;

	reg = LC1100H_REG_DCDC4_OUT_0;
	mask = LC1100H_REG_BITMASK_DCDC4_OUT0;

	return lc1100h_reg_bit_write(reg, mask, i);
}

static int lc1100h_dcdc_vout_set(u8 dcdc_id, u8 vout_id, int mv)
{
	int ret;

	if (dcdc_id >= LC1100H_DCDC_MAX)
		return -1;

	if (dcdc_id == LC1100H_DCDC1) {
		ret = lc1100h_dcdc1_vout_set(vout_id, mv);
	} else if (dcdc_id == LC1100H_DCDC2) {
		ret = lc1100h_dcdc2_vout_set(vout_id, mv);
	} else if (dcdc_id == LC1100H_DCDC3) {
		ret = lc1100h_dcdc3_vout_set(vout_id, mv);
	} else if (dcdc_id == LC1100H_DCDC4) {
		ret = lc1100h_dcdc4_vout_set(vout_id, mv);
	}

	return ret;
}

static void lc1100h_rtc_alarm_disable(u8 id)
{
	if(id == 0) {
		lc1100h_reg_write(LC1100H_REG_RTC_AL1_SEC, 0);
		lc1100h_reg_write(LC1100H_REG_RTC_AL1_MIN, 0);
		lc1100h_reg_write(LC1100H_REG_RTC_AL1_HOUR, 0);
		lc1100h_reg_write(LC1100H_REG_RTC_AL1_DAY, 0);
		lc1100h_reg_write(LC1100H_REG_RTC_AL1_MONTH, 0);
		lc1100h_reg_write(LC1100H_REG_RTC_AL1_YEAR, 0);
		lc1100h_reg_bit_write(LC1100H_REG_RTC_INT_STATUS, 0x20, 1);
	} else {
		lc1100h_reg_write(LC1100H_REG_RTC_AL2_SEC, 0);
		lc1100h_reg_write(LC1100H_REG_RTC_AL2_MIN, 0);
		lc1100h_reg_write(LC1100H_REG_RTC_AL2_HOUR, 0);
		lc1100h_reg_write(LC1100H_REG_RTC_AL2_DAY, 0);
		lc1100h_reg_write(LC1100H_REG_RTC_AL2_MONTH, 0);
		lc1100h_reg_write(LC1100H_REG_RTC_AL2_YEAR, 0);
		lc1100h_reg_bit_write(LC1100H_REG_RTC_INT_STATUS, 0x10, 1);
	}
}

static u16 lc1100h_read_adc(u8 val, u16 *out)
{
	u8 data0 = 0, data1 = 0;
	u8 ad_state = 0, timeout;


	/* Enable ADC power */
	lc1100h_reg_write(LC1100H_REG_ADCLDOEN, 0x01);


	/* Set ADC channle */
	lc1100h_reg_bit_write(LC1100H_REG_ADCCR,
	                         LC1100H_REG_BITMASK_ADSEL, (u8)val);

	/* Set ADC mode && formate */
	lc1100h_reg_bit_write(LC1100H_REG_ADCCR,
	                         LC1100H_REG_BITMASK_ADMODE, 0x00); /* not continue mode */
	lc1100h_reg_write(LC1100H_REG_ADCFORMAT, 0x00); /* 12bit conver */

	/* Enable ADC  */
	lc1100h_reg_write(LC1100H_REG_ADCEN, 0x01);

	/* Start ADC */
	lc1100h_reg_bit_write(LC1100H_REG_ADCCMD,
	                         LC1100H_REG_BITMASK_ADSTART, 0x01);
	timeout = 3;
	do {
		udelay(50); //1TODO: Need check or change to wait implement
		timeout--;
		lc1100h_reg_read(LC1100H_REG_ADCCMD, &ad_state);
		ad_state = !!(ad_state & LC1100H_REG_BITMASK_ADEND);
	} while(ad_state != 1 && timeout != 0);

	if (timeout == 0) {
		printf("ADC convert timeout!\n");
		data0 = data1 = 0xff; /* Indicate ADC convert failed */
	} else {
		lc1100h_reg_read(LC1100H_REG_ADCDAT0, &data0); /* Low  4 bit */
		lc1100h_reg_read(LC1100H_REG_ADCDAT1, &data1); /* High 8 bit */
	}
	*out = ((u16)data1 << 4) | ((u16)data0);

	/* Disable ADC */
	lc1100h_reg_write(LC1100H_REG_ADCLDOEN, 0x00);

	return 0;
}

static int lc1100h_battery_adc2voltage(void)
{
	u16 data = 0;

	lc1100h_read_adc(0, &data);

	return (int)(((int)data * 2800 * 4) / 4096);
}

static int lc1100h_voltage_get(void)
{
	int cnt = 0;
	int voltage = 0;
	int i;

	for (i = 0; i < 3; i++) {
		int temp = lc1100h_battery_adc2voltage();
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

static int lc1100h_charger_state_get(void)
{
	u8 state = 0;
	lc1100h_reg_read(LC1100H_REG_PCST, &state);
	return !!(state & LC1100H_REG_BITMASK_ADPIN);
}

static int lc1100h_battery_voltage_check(struct pmic_info *info)
{
	int voltage;
	int ret = 0;
	int retries = 1000;

	do {
		voltage = lc1100h_voltage_get();
		if (lc1100h_charger_state_get()) {
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
				pmic_power_off();
			}
		}
		if (0 == retries)
			printf("Low battery voltage but timeout!\n");
	} while (!ret && retries--);

	return 0;
}

static int lc1100h_dcdc1_init(struct pmic_info *info)
{
	lc1100h_dcdc_vout_set(LC1100H_DCDC1, 0, info->buck1_vout0);
	lc1100h_dcdc_vout_set(LC1100H_DCDC1, 1, info->buck1_vout1);

	return 0;
}

static int lc1100h_dcdc2_init(struct pmic_info *info)
{
	int ret;

#if (CONFIG_USE_EXT_BUCK2 == 0)
#if (BUCK2_VOUT_SEL == 3)
	lc1100h_dcdc_vout_set(LC1100H_DCDC2, 1, info->buck2_vout1);
	lc1100h_dcdc_vout_set(LC1100H_DCDC2, 2, info->buck2_vout2);
	lc1100h_dcdc_vout_set(LC1100H_DCDC2, 3, info->buck2_vout3);
#else
	/* In one line dvfs mode, set all vout to maximum value. */
	lc1100h_dcdc_vout_set(LC1100H_DCDC2, 0, info->buck2_vout2);
	lc1100h_dcdc_vout_set(LC1100H_DCDC2, 1, info->buck2_vout2);
	lc1100h_dcdc_vout_set(LC1100H_DCDC2, 2, info->buck2_vout2);
#endif

	REG32(AP_PWR_ARMDV_CTL) = BUCK2_VOUT_SEL;

	ret = lc1100h_reg_bit_write(LC1100H_REG_DCDC_DVS, LC1100H_REG_BITMASK_DCDC2_DVS_SEL, 1);
	if (ret) {
		printf("[LC1100H]set dcdc2 mode failed : %d!\n", ret);
		return -1;
	}

#if (BUCK2_VOUT_SEL == 3)
	lc1100h_dcdc_vout_set(LC1100H_DCDC2, 0, info->buck2_vout0);
#endif

#else
	/* Disable dcdc2. */
	ret = lc1100h_reg_bit_write(LC1100H_REG_DCDC_DVS, LC1100H_REG_BITMASK_DCDC2_EN, 0);
	if (ret) {
		printf("[LC1100H]disable dcdc2 failed : %d!\n", ret);
		return -1;
	}

#endif

	return 0;
}

static int lc1100h_dcdc_pwm_set(void)
{
	int ret;

	/* auto pwm/pfm mode. */
	ret = lc1100h_reg_write(LC1100H_REG_DC_MODE, 0xf0);
	if (ret) {
		printf("[LC1100H]set dc mode register failed : %d!\n", ret);
		return -1;
	}
	return 0;
}

static int lc1100h_misc_init(void)
{
	lc1100h_reg_write(LC1100H_REG_TESTEN, 0x0f);
	return 0;
}

static int lc1100h_init(struct pmic_info *info)
{
	unsigned char reg;
	unsigned char val;
	unsigned char mask;
	int reboot_type;
	int startup_type;
	int power_on_type;
	int ret;

	ret = lc1100h_reg_read(LC1100H_REG_UP_STATE, &val);
	if (ret) {
		printf("[LC1100H]read startup register failed : %d!\n", ret);
		return -1;
	}

	startup_type = val;

	/* Get reboot type. */
	reg = PMIC_RTC_ALARM_POWEROFF ? LC1100H_REG_RTC_AL2_SEC : LC1100H_REG_RTC_AL1_SEC;
	ret = lc1100h_reg_read(reg, &val);
	if (ret) {
		printf("[LC1100H]read reboot register failed : %d!\n", ret);
		return -1;
	}

	reboot_type = (val & 0x7f);

	if ((reboot_type == REBOOT_NONE) && (startup_type & LC1100H_REG_BITMASK_ALARM_STARTUP)) {
		mask = PMIC_RTC_ALARM_REBOOT ? LC1100H_REG_BITMASK_RTC_AL2_RAW : LC1100H_REG_BITMASK_RTC_AL1_RAW;
		ret = lc1100h_reg_read(LC1100H_REG_RTC_INT_RAW, &val);
		if (ret) {
			printf("[LC1100H]read startup register failed : %d!\n", ret);
			return -1;
		}

		if (val & mask) {
			lc1100h_rtc_alarm_disable(PMIC_RTC_ALARM_REBOOT);
			pmic_power_off();
		}
	}

	if (startup_type & LC1100H_REG_BITMASK_ALARM_STARTUP)
		power_on_type = PU_REASON_RTC_ALARM;
	else if (startup_type & LC1100H_REG_BITMASK_HF_PWR_STARTUP)
		power_on_type = PU_REASON_LOW_PWR_RESET;
	else if (startup_type & LC1100H_REG_BITMASK_ADPIN_STARTUP)
		power_on_type = PU_REASON_USB_CHARGER;
	else if (startup_type & LC1100H_REG_BITMASK_KEYON_STARTUP)
		power_on_type = PU_REASON_PWR_KEY_PRESS;
	else
		power_on_type = PU_REASON_PWR_KEY_PRESS;

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

	lc1100h_misc_init();
	lc1100h_dcdc_pwm_set();
	lc1100h_dcdc1_init(info);
	lc1100h_dcdc2_init(info);

	return 0;
}

static int lc1100h_power_on_key_check(struct pmic_info *info)
{
	unsigned char val;
	int ret;

	if (info->power_on_type == PU_REASON_PWR_KEY_PRESS) {
		ret = lc1100h_reg_read(LC1100H_REG_PCST, &val);
		if (ret)
			printf("[LC1100H]read present_pmu register failed : %d!\n", ret);

		/* Detect charger in. */
		if(!ret && !(val & LC1100H_REG_BITMASK_KONMON)) {
			if (!ret && !(val & LC1100H_REG_BITMASK_ADPIN))
				pmic_power_off();
		}
	}

    return 0;
}

int lc1100h_probe(struct pmic_info *info)
{
	info->init = lc1100h_init;
	info->battery_voltage_check = lc1100h_battery_voltage_check;
	info->power_on_key_check = lc1100h_power_on_key_check;

	return 0;
}

#endif /* CONFIG_COMIP_LC1100H*/
