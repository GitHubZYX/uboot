
#include <common.h>

#define FAN53555_I2C_ID				(COM_I2C)
#define FAN53555_I2C_ADDR			(0x60)

/* Register address. */
#define FAN53555_REG_VSEL0			(0x00)
#define FAN53555_REG_VSEL1			(0x01)
#define FAN53555_REG_CONTROL			(0x02)
#define FAN53555_REG_ID1			(0x03)
#define FAN53555_REG_ID2			(0x04)
#define FAN53555_REG_MONITOR			(0x05)

/* Register bit. */
/* FAN53555_REG_VSEL0* & FAN53555_REG_VSEL1* */
#define FAN53555_MASK_BUCK_EN			(0x80)
#define FAN53555_MASK_MODE			(0x40)
#define FAN53555_MASK_NSEL			(0x3f)

/* FAN53555_REG_CONTROL */
#define FAN53555_MASK_OUTPUT_DISCHARGE		(0x80)
#define FAN53555_MASK_SLEW			(0x70)

struct fan53555_reg {
	u8 reg;
	u8 val;
	u8 mask;
};

static int fan53555_reg_read(u8 reg, u8 *value)
{
	return comip_i2c_read(FAN53555_I2C_ID, FAN53555_I2C_ADDR, reg, value);
}

static int fan53555_reg_write(u8 reg, u8 value)
{
	return comip_i2c_write(FAN53555_I2C_ID, FAN53555_I2C_ADDR, reg, value);
}

static int fan53555_reg_bit_write(u8 reg, u8 mask, u8 value)
{
	u8 valo, valn;
	int ret;

	if (!mask)
		return -1;

	ret = fan53555_reg_read(reg, &valo);
	if (!ret) {
		valn = valo & ~mask;
		valn |= (value << (ffs(mask) - 1));
		ret = fan53555_reg_write(reg, valn);
	}

	return ret;
}

static u8 fan53555_vsel_get(int mv)
{
	u8 vsel;

	vsel = (mv - 603) * 1000 / 12826;
	if (vsel > FAN53555_MASK_NSEL)
		vsel = FAN53555_MASK_NSEL;

	return vsel;
}

static int fan53555_vout_set(u8 vout_id, int mv)
{
	u8 reg;
	u8 vsel = fan53555_vsel_get(mv);

	switch (vout_id) {
	case 0:
		reg = FAN53555_REG_VSEL0;
		break;
	case 1:
		reg = FAN53555_REG_VSEL1;
		break;
	default:
		return -1;
	}

	fan53555_reg_bit_write(reg, FAN53555_MASK_NSEL, vsel);

	return 0;
}

int fan53555_init(struct pmic_info *info)
{
	const struct fan53555_reg reg_init[] = {
		{FAN53555_REG_CONTROL, FAN53555_MASK_OUTPUT_DISCHARGE, 0x01},
	};
	int ret;
	u8 val;
	u8 i;

	ret = fan53555_reg_read(FAN53555_REG_ID1, &val);
	if (ret) {
		printf("FAN53555: no device\n");
		return ret;
	}

	printf("FAN53555: id1 0x%02x\n", val);

	for (i = 0; i < ARRAY_SIZE(reg_init); i++)
		fan53555_reg_bit_write(reg_init[i].reg, reg_init[i].mask, reg_init[i].val);

	REG32(AP_PWR_ARMDV_CTL) = BUCK2_VOUT_SEL;

	/* In one line dvfs mode, set all vout to maximum value. */
	fan53555_vout_set(0, info->buck2_vout2);
	fan53555_vout_set(1, info->buck2_vout2);

	return 0;
}

