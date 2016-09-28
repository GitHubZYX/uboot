
#include <common.h>

#define PMIC_I2C_ID		(COM_I2C)

DECLARE_GLOBAL_DATA_PTR;

extern int lc1100ht_probe(struct pmic_info *info);
extern int lc1100h_probe(struct pmic_info *info);
extern int lc1132_probe(struct pmic_info *info);
extern int lc1160_probe(struct pmic_info *info);
extern int ncp6335_init(void);
extern int fan53555_init(void);

static struct pmic_info* pmic_get_info(void)
{
	struct pmic_info* info = (struct pmic_info*)(&gd->pmic_info);

	if (sizeof(struct pmic_info) > sizeof(gd->pmic_info)) {
		printf("PMIC information struct is too large!\n");
		return NULL;
	}

	return info;
}

void pmic_reboot(void)
{
	printf("reboot...\n");

	REG32(AP_PWR_SFRST_CTL) = 0xff;

	while(1);
}

void pmic_power_off(void)
{
	printf("power off\n");

	/* PS Hold low. */
	REG32(AP_PWR_PWEN_CTL) = 0x0;

	/* Wait for power off... */
	while (1);
}

int pmic_reg_read(u8 slave_addr, u8 reg, u8 *value)
{
	return comip_i2c_read(PMIC_I2C_ID, slave_addr, reg, value);
}

int pmic_reg_write(u8 slave_addr, u8 reg, u8 value)
{
	return comip_i2c_write(PMIC_I2C_ID, slave_addr, reg, value);
}

int pmic_reg_bit_write(u8 slave_addr, u8 reg, u8 mask, u8 value)
{
	u8 valo, valn;
	int ret;

	if (!mask)
		return -1;

	ret = pmic_reg_read(slave_addr, reg, &valo);
	if (!ret) {
		valn = valo & ~mask;
		valn |= (value << (ffs(mask) - 1));
		ret = pmic_reg_write(slave_addr, reg, valn);
	}

	return ret;
}

int pmic_init(void)
{
	struct pmic_info *info = pmic_get_info();
	int ret = -1;

	comip_i2c_init(PMIC_I2C_ID, 0);

#if defined(CONFIG_PMIC_AUTO)
	ret = lc1100ht_probe(info);
	if (ret)
		ret = lc1100ht_probe(info);
	if (ret)
		ret = lc1132_probe(info);
	if (ret)
		ret = lc1160_probe(info);
#elif defined(CONFIG_PMIC_LC1100HT)
	ret = lc1100ht_probe(info);
#elif defined(CONFIG_PMIC_LC1100H)
	ret = lc1100h_probe(info);
#elif defined(CONFIG_PMIC_LC1132)
	ret = lc1132_probe(info);
#elif defined(CONFIG_PMIC_LC1160)
	ret = lc1160_probe(info);
#endif

#ifdef CONFIG_NCP6335
	ncp6335_init();
#endif

#ifdef CONFIG_FAN53555
	fan53555_init();
#endif

	if (!ret && info && info->init) {
		ret = info->init(info);
		if (ret)
			return ret;
	}

#if CONFIG_LOW_VOLTAGE_CHARGING
	if (!ret && info && info->battery_voltage_check) {
		ret = info->battery_voltage_check(info);
		if (ret)
			return ret;
	}
#endif

	return ret;
}

int pmic_power_up_reason_get(void)
{
	struct pmic_info *info = pmic_get_info();

	if (info)
		return info->power_on_type;

	return -1;
}

int pmic_reboot_reason_get(void)
{
	struct pmic_info *info = pmic_get_info();

	if (info)
		return info->reboot_type;

	return -1;
}

void pmic_reboot_reason_set(u8 type)
{
	struct pmic_info *info = pmic_get_info();
	printf("pmic_reboot_reason_set ->0x%x \n",type);
	if (info&&info->reboot_reason_set)
		info->reboot_reason_set(type);
}

int pmic_power_on_key_check(void)
{
	struct pmic_info *info = pmic_get_info();

	if (info && info->power_on_key_check)
		return info->power_on_key_check(info);

	return -1;
}

int pmic_vibrator_enable_set(void)
{
	struct pmic_info *info = pmic_get_info();

	if (info && info->vibrator_enable)
		return info->vibrator_enable(info);

	return -1;
}

int pmic_lcdio_enable(int enable)
{
	struct pmic_info *info = pmic_get_info();

	if (info && info->lcdio_enable)
		return info->lcdio_enable(info, enable);

	return -1;
}

int pmic_lcdcore_enable(int enable)
{
	struct pmic_info *info = pmic_get_info();

	if (info && info->lcdcore_enable)
		return info->lcdcore_enable(info, enable);

	return -1;
}

