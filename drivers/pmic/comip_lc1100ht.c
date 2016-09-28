
#include <common.h>

#if defined(CONFIG_PMIC_LC1100HT) || defined(CONFIG_PMIC_AUTO)

/* LC1100HT I2C address. */
#define LC1100HT_I2C_ADDR					(0x70)

/* LC1100HT register address. */
#define LC1100HT_REG_PRESENT_PMU			(0x06)
#define LC1100HT_REG_PRESENT_CHARGER	(0x07)
#define LC1100HT_REG_UP_STATE				(0x08)
#define LC1100HT_REG_DOWN_STATE			(0x09)
#define LC1100HT_REG_UP_EN					(0x0a)
#define LC1100HT_REG_DOWN_EN				(0x0b)
#define LC1100HT_REG_BUCK_DVS				(0x1d)
#define LC1100HT_REG_BUCK1_OUT_0			(0x1e)
#define LC1100HT_REG_BUCK1_OUT_1			(0x1f)
#define LC1100HT_REG_BUCK1_OUT_2			(0x20)
#define LC1100HT_REG_BUCK1_OUT_3			(0x21)
#define LC1100HT_REG_BUCK2_OUT_0			(0x22)
#define LC1100HT_REG_BUCK2_OUT_1			(0x23)
#define LC1100HT_REG_BUCK2_OUT_2			(0x24)
#define LC1100HT_REG_BUCK2_OUT_3			(0x25)
#define LC1100HT_REG_BUCK3_OUT_0			(0x26)
#define LC1100HT_REG_BUCK4_OUT_0			(0x27)
#define LC1100HT_REG_BUCK_PWM			(0x2d)
#define LC1100HT_REG_ADC_CTL				(0x60)
#define LC1100HT_REG_ADC_DATA0			(0x61)
#define LC1100HT_REG_ADC_DATA1			(0x62)
#define LC1100HT_REG_ADC_CONVERSION_DONE	(0x63)
#define LC1100HT_REG_ALARM1_SECOND		(0x77)
#define LC1100HT_REG_ALARM1_MINUTE		(0x78)
#define LC1100HT_REG_ALARM1_HOUR			(0x79)
#define LC1100HT_REG_ALARM1_DATE			(0x7a)
#define LC1100HT_REG_ALARM1_MONTH		(0x7b)
#define LC1100HT_REG_ALARM1_YEAR			(0x7c)
#define LC1100HT_REG_ALARM1_WEEK			(0x7d)
#define LC1100HT_REG_ALARM2_SECOND		(0x7e)
#define LC1100HT_REG_ALARM2_MINUTE		(0x7f)
#define LC1100HT_REG_ALARM2_HOUR			(0x80)
#define LC1100HT_REG_ALARM2_DATE			(0x81)
#define LC1100HT_REG_ALARM2_MONTH		(0x82)
#define LC1100HT_REG_ALARM2_YEAR			(0x83)
#define LC1100HT_REG_ALARM2_WEEK			(0x84)

/* LC1100HT_REG_CHARGER_STATE */
#define LC1100HT_REG_BITMASK_CHG_INPUT_STATE	(0x03)

/* LC1100HT_REG_CHARGER_STATE */
#define LC1100HT_REG_BITMASK_BUCK1_OUT0			(0x1f)
#define LC1100HT_REG_BITMASK_BUCK1_OUT1			(0x1f)
#define LC1100HT_REG_BITMASK_BUCK1_OUT2			(0x1f)
#define LC1100HT_REG_BITMASK_BUCK1_OUT3			(0x1f)
#define LC1100HT_REG_BITMASK_BUCK2_OUT0			(0x1f)
#define LC1100HT_REG_BITMASK_BUCK2_OUT1			(0x1f)
#define LC1100HT_REG_BITMASK_BUCK2_OUT2			(0x1f)
#define LC1100HT_REG_BITMASK_BUCK2_OUT3			(0x1f)
#define LC1100HT_REG_BITMASK_BUCK3_OUT0			(0x1f)
#define LC1100HT_REG_BITMASK_BUCK4_OUT0			(0x1f)

/* LC1100HT_REG_ADC */
#define LC1100HT_REG_BITMASK_ADC_CONV_START	(0x01)
#define LC1100HT_REG_BITMASK_ADC_INPUT_SEL		(0x1e)
#define LC1100HT_REG_BITMASK_ADC_FORMAT		(0x20)
#define LC1100HT_REG_BITMASK_ADC_CONV_DONE		(0x01)

/* LC1100HT_REG_UP_STATE */
#define LC1100HT_REG_BITMASK_PWR_ON_STARTUP	(0x01)
#define LC1100HT_REG_BITMASK_HF_PWR_STARTUP	(0x02)
#define LC1100HT_REG_BITMASK_CHARGER_STARTUP	(0x04)
#define LC1100HT_REG_BITMASK_MPL_STARTUP		(0x08)
#define LC1100HT_REG_BITMASK_RTC_ALARM1			(0x10)
#define LC1100HT_REG_BITMASK_RTC_ALARM2			(0x20)
#define LC1100HT_REG_BITMASK_RSTIN_N_STARTUP	(0x40)
#define LC1100HT_REG_BITMASK_BUCK1_DVS			(0x03)
#define LC1100HT_REG_BITMASK_BUCK1_DVS_SEL		(0x04)
#define LC1100HT_REG_BITMASK_BUCK2_DVS			(0x30)
#define LC1100HT_REG_BITMASK_BUCK2_DVS_SEL		(0x40)

/* LC1100HT_REG_DOWN_STATE */
#define LC1100HT_REG_BITMASK_RSTIN_N_SHUTDOWN	(0x10)

/* LC1100HT_REG_UP_STATE. */
#define LC1100HT_BIT_ALARM1_STARTUP				(4)

/* LC1100HT_REG_PRESENT_STATE. */
#define LC1100HT_BIT_POWER_ON_INPUT				(5)

/* LC1100HT_REG_PRESENT_STATE. */
#define LC1100HT_BIT_CHARGER_IN					(0x03)

enum {
	LC1100HT_BUCK1 = 0,
	LC1100HT_BUCK2,
	LC1100HT_BUCK3,
	LC1100HT_BUCK4,
	LC1100HT_BUCK_MAX,
};

enum {
	LC1100HT_BUCK_VOUT0 = 0,
	LC1100HT_BUCK_VOUT1,
	LC1100HT_BUCK_VOUT2,
	LC1100HT_BUCK_VOUT3,
	LC1100HT_BUCK_VOUT_MAX,
};

static int lc1100ht_reg_read(u8 reg, u8 *value)
{
	return pmic_reg_read(LC1100HT_I2C_ADDR, reg, value);
}

static int lc1100ht_reg_write(u8 reg, u8 value)
{
	return pmic_reg_write(LC1100HT_I2C_ADDR, reg, value);
}

static int lc1100ht_reg_bit_write(u8 reg, u8 mask, u8 value)
{
	return pmic_reg_bit_write(LC1100HT_I2C_ADDR, reg, mask, value);
}

static void lc1100ht_rtc_alarm_disable(unsigned char id)
{
	if (id == 0) {
		lc1100ht_reg_write(LC1100HT_REG_ALARM1_SECOND, 0);
		lc1100ht_reg_write(LC1100HT_REG_ALARM1_MINUTE, 0);
		lc1100ht_reg_write(LC1100HT_REG_ALARM1_HOUR, 0);
		lc1100ht_reg_write(LC1100HT_REG_ALARM1_DATE, 0);
		lc1100ht_reg_write(LC1100HT_REG_ALARM1_MONTH, 0);
		lc1100ht_reg_write(LC1100HT_REG_ALARM1_YEAR, 0);
		lc1100ht_reg_write(LC1100HT_REG_ALARM1_WEEK, 0);
	} else {
		lc1100ht_reg_write(LC1100HT_REG_ALARM2_SECOND, 0);
		lc1100ht_reg_write(LC1100HT_REG_ALARM2_MINUTE, 0);
		lc1100ht_reg_write(LC1100HT_REG_ALARM2_HOUR, 0);
		lc1100ht_reg_write(LC1100HT_REG_ALARM2_DATE, 0);
		lc1100ht_reg_write(LC1100HT_REG_ALARM2_MONTH, 0);
		lc1100ht_reg_write(LC1100HT_REG_ALARM2_YEAR, 0);
		lc1100ht_reg_write(LC1100HT_REG_ALARM2_WEEK, 0);
	}
}

static int lc1100ht_vol2hex_common_500_2000(int mv)
{
	const int offset = 1;
	const int step = 50;
	const int start = 800;
	int ret;

	/* Parameter && Special value dispose */
	if (mv < 500 || mv > 2000) {
		printf("invalid parameter\n");
		return -1;
	}

	if (mv == 500)
		return 0;

	ret = mv % step;
	ret |= start % step;
	if (ret != 0) {
		printf("why data is not regular match(%d:%d)(%d:%d)\n",
		       mv, ret, start, step);
		return -1;
	}

	ret = mv / step;
	ret -= start / step;
	return ret < 0 ? ret : ret + offset;
}

static int lc1100ht_buck2_vout_set(unsigned char id, int mv)
{
	unsigned char reg;
	unsigned char val;
	int ret;

	if (id > LC1100HT_BUCK_VOUT_MAX)
		return -1;

	reg = LC1100HT_REG_BUCK2_OUT_0 + id;

	ret = lc1100ht_vol2hex_common_500_2000(mv);
	if (ret < 0)
		return -1;

	val = (unsigned char)ret;
	ret = lc1100ht_reg_write(reg, val);
	if (ret) {
		printf("[LC1100HT]set vout register failed : %d!\n", ret);
		return -1;
	}

	return 0;
}

static int lc1100ht_buck2_vout_select(unsigned char id)
{
	unsigned char val;
	int ret;

	if (id > LC1100HT_BUCK_VOUT_MAX)
		return -1;

	ret = lc1100ht_reg_read(LC1100HT_REG_BUCK_DVS, &val);
	if (ret) {
		printf("[LC1100HT]read dvs register failed : %d!\n", ret);
		return -1;
	}

	REG32(AP_PWR_ARMDV_CTL) = id;

	val = (id << 4) | (val & 0x0f);
	ret = lc1100ht_reg_write(LC1100HT_REG_BUCK_DVS, val);
	if (ret) {
		printf("[LC1100HT]set dvs register failed : %d!\n", ret);
		return -1;
	}

	return 0;
}

static int lc1100ht_buck1_vout_set(unsigned char id, int mv)
{
	unsigned char reg;
	unsigned char val;
	int ret;

	if (id > LC1100HT_BUCK_VOUT_MAX)
		return -1;

	reg = LC1100HT_REG_BUCK1_OUT_0 + id;
	ret = lc1100ht_vol2hex_common_500_2000(mv);
	if (ret < 0)
		return -1;

	val = (unsigned char)ret;
	ret = lc1100ht_reg_write(reg, val);
	if (ret) {
		printf("[LC1100HT]set buck1-vout3 register failed : %d!\n", ret);
		return -1;
	}

	return 0;
}

static int lc1100ht_buck1_dvs_set(void)
{
	unsigned char val;
	int ret;

	ret = lc1100ht_reg_read(LC1100HT_REG_BUCK_DVS, &val);
	if (ret) {
		printf("[LC1100HT]read dvs register failed : %d!\n", ret);
		return -1;
	}

	val &= ~LC1100HT_REG_BITMASK_BUCK1_DVS_SEL;
	ret = lc1100ht_reg_write(LC1100HT_REG_BUCK_DVS, val);
	if (ret) {
		printf("[LC1100HT]set buck1 dvs register failed : %d!\n", ret);
		return -1;
	}

	return 0;
}

static int lc1100ht_buck1_init(struct pmic_info *info)
{
	lc1100ht_buck1_vout_set(LC1100HT_BUCK_VOUT0, info->buck1_vout0);
	lc1100ht_buck1_vout_set(LC1100HT_BUCK_VOUT1, info->buck1_vout1);

	return 0;
}

static int lc1100ht_buck2_init(struct pmic_info *info)
{
#if (BUCK2_VOUT_SEL == 3)
	lc1100ht_buck2_vout_set(LC1100HT_BUCK_VOUT0, info->buck2_vout0);
	lc1100ht_buck2_vout_set(LC1100HT_BUCK_VOUT1, info->buck2_vout1);
	lc1100ht_buck2_vout_set(LC1100HT_BUCK_VOUT2, info->buck2_vout2);
	lc1100ht_buck2_vout_set(LC1100HT_BUCK_VOUT3, info->buck2_vout3);
#else
	/* In one line dvfs mode, set all vout to maximum value. */
	lc1100ht_buck2_vout_set(LC1100HT_BUCK_VOUT0, info->buck2_vout2);
	lc1100ht_buck2_vout_set(LC1100HT_BUCK_VOUT1, info->buck2_vout2);
	lc1100ht_buck2_vout_set(LC1100HT_BUCK_VOUT2, info->buck2_vout2);
#endif

	return 0;
}

static int lc1100ht_buck_pwm_set(void)
{
	int ret;

	/* Disable monitor, force pwm mode. */
#if (defined(CONFIG_BOARD_LC1810_EVB) || defined(CONFIG_COMIP_YL8150S_EVB))
	ret = lc1100ht_reg_write(LC1100HT_REG_BUCK_PWM, 0x0f);
#else
	ret = lc1100ht_reg_write(LC1100HT_REG_BUCK_PWM, 0x00);
#endif
	if (ret) {
		printf("[LC1100HT]set buck pwm register failed : %d!\n", ret);
		return -1;
	}

	return 0;
}

static int lc1100ht_up_enable(void)
{
	unsigned char val;
	int ret;

	ret = lc1100ht_reg_read(LC1100HT_REG_DOWN_EN, &val);
	if (ret) {
		printf("[LC1100HT]read up enable register failed : %d!\n", ret);
		return -1;
	}

	if (!(val & LC1100HT_REG_BITMASK_RSTIN_N_SHUTDOWN)) {
		val |= LC1100HT_REG_BITMASK_RSTIN_N_SHUTDOWN;
		ret = lc1100ht_reg_write(LC1100HT_REG_DOWN_EN, val);
		if (ret) {
			printf("[LC1100HT]write up enable register failed : %d!\n", ret);
			return -1;
		}
	}

	return 0;
}

static int lc1100ht_ps_hold_init(void)
{
	int ret;

	ret = lc1100ht_reg_write(LC1100HT_REG_DOWN_EN, 0x1f);
	if (ret) {
		printf("[LC1100HT] write showdown register failed : %d!\n", ret);
		return -1;
	}
	return 0;
}

static int lc1100ht_read_adc(void)
{
	u8 ad_state = 0, timeout;
	u8 data0 = 0, data1 = 0;
	u16 adc;

	lc1100ht_reg_bit_write(LC1100HT_REG_ADC_CTL, LC1100HT_REG_BITMASK_ADC_FORMAT, 0);
	lc1100ht_reg_bit_write(LC1100HT_REG_ADC_CTL, LC1100HT_REG_BITMASK_ADC_INPUT_SEL, 0);
	lc1100ht_reg_bit_write(LC1100HT_REG_ADC_CTL, LC1100HT_REG_BITMASK_ADC_CONV_START, 1);

	timeout = 5;
	do {
		udelay(100);
		timeout--;
		lc1100ht_reg_read(LC1100HT_REG_ADC_CONVERSION_DONE, &ad_state);
		ad_state = !!(ad_state & LC1100HT_REG_BITMASK_ADC_CONV_DONE);
	} while(ad_state != 1 && timeout != 0);

	if (timeout == 0) {
		printf("ADC convert timeout!\n");
		return -1;
	} else {
		lc1100ht_reg_read(LC1100HT_REG_ADC_DATA1, &data0);
		lc1100ht_reg_read(LC1100HT_REG_ADC_DATA0, &data1);
	}
	adc = ((u16)data1 << 4) | ((u16)data0 >> 4);

	return (int)((((int)adc * 10 + 5 ) * 5500) / 40950);
}

static int lc1100ht_voltage_get(void)
{
	int cnt = 0;
	int voltage = 0;
	int i;

	for (i = 0; i < 3; i++) {
		int temp = lc1100ht_read_adc();
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

static int lc1100ht_charger_state_get(void)
{
	u8 state = 0;
	lc1100ht_reg_read(LC1100HT_REG_PRESENT_CHARGER, &state);
	return !!(state & LC1100HT_REG_BITMASK_CHG_INPUT_STATE);
}

static int lc1100ht_battery_voltage_check(struct pmic_info *info)
{
	int voltage;
	int ret = 0;
	int retries = 1000;

	do {
		voltage = lc1100ht_voltage_get();
		if (lc1100ht_charger_state_get()) {
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

static int lc1100ht_init(struct pmic_info *info)
{
	unsigned char reg;
	unsigned char val;
	unsigned char bit;
	int reboot_type;
	int startup_type;
	int power_on_type;
	int ret;

	ret = lc1100ht_up_enable();
	if (ret)
		return -1;

	ret = lc1100ht_ps_hold_init();
	if (ret)
		return -1;

	ret = lc1100ht_reg_read(LC1100HT_REG_UP_STATE, &val);
	if (ret) {
		printf("[LC1100HT]read startup register failed : %d!\n", ret);
		return -1;
	}

	startup_type = val;

	ret = lc1100ht_reg_read(LC1100HT_REG_ALARM2_SECOND, &val);
	if (ret) {
		printf("[LC1100HT]read reboot register failed : %d!\n", ret);
		return -1;
	}

	/* Get reboot type. */
	bit = LC1100HT_BIT_ALARM1_STARTUP + PMIC_RTC_ALARM_REBOOT;
	reg = PMIC_RTC_ALARM_POWEROFF ? LC1100HT_REG_ALARM2_SECOND : LC1100HT_REG_ALARM1_SECOND;
	ret = lc1100ht_reg_read(reg, &val);
	if (ret) {
		printf("[LC1100HT]read reboot register failed : %d!\n", ret);
		return -1;
	}

	reboot_type = (val & (0x7f));

	if ((reboot_type == REBOOT_NONE) && (startup_type & (1 << bit))) {
		lc1100ht_rtc_alarm_disable(PMIC_RTC_ALARM_REBOOT);
		pmic_power_off();
	}

	if (startup_type & LC1100HT_REG_BITMASK_RTC_ALARM1)
		power_on_type = PU_REASON_RTC_ALARM;
	else if (startup_type & LC1100HT_REG_BITMASK_RTC_ALARM2)
		power_on_type = PU_REASON_RTC_ALARM;
	else if (startup_type & LC1100HT_REG_BITMASK_RSTIN_N_STARTUP)
		power_on_type = PU_REASON_HARDWARE_RESET;
	else if (startup_type & LC1100HT_REG_BITMASK_CHARGER_STARTUP)
		power_on_type = PU_REASON_USB_CHARGER;
	else if (startup_type & LC1100HT_REG_BITMASK_PWR_ON_STARTUP)
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

	lc1100ht_buck_pwm_set();
	lc1100ht_buck1_init(info);
	lc1100ht_buck1_dvs_set();
	lc1100ht_buck2_init(info);
	lc1100ht_buck2_vout_select(BUCK2_VOUT_SEL);

	return 0;
}

static int lc1100ht_power_on_key_check(struct pmic_info *info)
{
	unsigned char val;
	int ret;

	/* Check power key. */
	if (info->power_on_type == PU_REASON_PWR_KEY_PRESS) {
		ret = lc1100ht_reg_read(LC1100HT_REG_PRESENT_PMU, &val);
		if (ret)
			printf("[LC1100HT]read present_pmu register failed : %d!\n", ret);

		/* Detect charger in. */
		if(!ret && !(val & (1L << LC1100HT_BIT_POWER_ON_INPUT))) {
			ret = lc1100ht_reg_read(LC1100HT_REG_PRESENT_CHARGER, &val);
			if (ret)
				printf("[LC1100HT]read present_charger register failed : %d!\n", ret);

			if (!ret && !(val & LC1100HT_BIT_CHARGER_IN))
				pmic_power_off();
		}
	}

	return 0;
}

int lc1100ht_probe(struct pmic_info *info)
{
	info->init = lc1100ht_init;
	info->battery_voltage_check = lc1100ht_battery_voltage_check;
	info->power_on_key_check = lc1100ht_power_on_key_check;

	return 0;
}

#endif /* CONFIG_PMIC_LC1100HT*/
