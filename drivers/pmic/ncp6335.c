
#include <common.h>

#define NCP6335_I2C_ID				(COM_I2C)
#define NCP6335_I2C_ADDR			(0x1c)

/* Register address. */
#define NCP6335_REG_INTACK			(0x00)
#define NCP6335_REG_INTSEN			(0x01)
#define NCP6335_REG_INTMASK			(0x02)
#define NCP6335_REG_PID				(0x03)
#define NCP6335_REG_RID				(0x04)
#define NCP6335_REG_FID				(0x05)
#define NCP6335_REG_PROGVSEL1			(0x10)
#define NCP6335_REG_PROGVSEL0			(0x11)
#define NCP6335_REG_PGOOD			(0x12)
#define NCP6335_REG_TIME			(0x13)
#define NCP6335_REG_COMMAND			(0x14)
#define NCP6335_REG_MODULE			(0x15)
#define NCP6335_REG_LIMCONF			(0x16)

/* Register bit. */
/* NCP6335_REG_PROGVSEL* & NCP6335_REG_RESERVED* */
#define NCP6335_MASK_PROGVSEL_ENVSEL		(0x80)
#define NCP6335_MASK_PROGVSEL_VSEL		(0x7f)

/* NCP6335_REG_PGOOD */
#define NCP6335_MASK_PGOOD_PGDCDC		(0x01)
#define NCP6335_MASK_PGOOD_PGDVS		(0x02)
#define NCP6335_MASK_PGOOD_TOR			(0x0c)
#define NCP6335_MASK_PGOOD_DISCHG		(0x10)

struct ncp6335_reg {
	u8 reg;
	u8 val;
	u8 mask;
};

static int ncp6335_reg_read(u8 reg, u8 *value)
{
	return comip_i2c_read(NCP6335_I2C_ID, NCP6335_I2C_ADDR, reg, value);
}

static int ncp6335_reg_write(u8 reg, u8 value)
{
	return comip_i2c_write(NCP6335_I2C_ID, NCP6335_I2C_ADDR, reg, value);
}

static int ncp6335_check(void)
{
	int ret;
	u8 val;

	ret = ncp6335_reg_read(NCP6335_REG_FID, &val);
	if (ret) {
		printf("NCP6335: no device\n");
		return 0;
	}

	printf("NCP6335: fid 0x%02x\n", val);
	return 1;
}

#if CONFIG_USE_EXT_BUCK2
static int ncp6335_reg_bit_write(u8 reg, u8 mask, u8 value)
{
	u8 valo, valn;
	int ret;

	if (!mask)
		return -1;

	ret = ncp6335_reg_read(reg, &valo);
	if (!ret) {
		valn = valo & ~mask;
		valn |= (value << (ffs(mask) - 1));
		ret = ncp6335_reg_write(reg, valn);
	}

	return ret;
}

static u8 ncp6335_vsel_get(int mv)
{
	u8 vsel;

	vsel = (mv - 600) * 100 / 625;
	if (vsel > NCP6335_MASK_PROGVSEL_VSEL)
		vsel = NCP6335_MASK_PROGVSEL_VSEL;

	return vsel;
}

static int ncp6335_vout_set(u8 vout_id, int mv)
{
	u8 reg;
	u8 vsel = ncp6335_vsel_get(mv);

	switch (vout_id) {
	case 0:
		reg = NCP6335_REG_PROGVSEL0;
		break;
	case 1:
		reg = NCP6335_REG_PROGVSEL1;
		break;
	default:
		return -1;
	}

	ncp6335_reg_write(reg, NCP6335_MASK_PROGVSEL_ENVSEL | vsel);

	return 0;
}

int ncp6335_init(struct pmic_info *info)
{
	const struct ncp6335_reg reg_init[] = {
		{NCP6335_REG_PGOOD, NCP6335_MASK_PGOOD_DISCHG, 0x01},
	};
	u8 i;

	if (!ncp6335_check())
		return -1;

	for (i = 0; i < ARRAY_SIZE(reg_init); i++)
		ncp6335_reg_bit_write(reg_init[i].reg, reg_init[i].mask, reg_init[i].val);

	REG32(AP_PWR_ARMDV_CTL) = BUCK2_VOUT_SEL;

	/* In one line dvfs mode, set all vout to maximum value. */
	ncp6335_vout_set(0, info->buck2_vout2);
	ncp6335_vout_set(1, info->buck2_vout2);

	return 0;
}
#else /*CONFIG_USE_EXT_BUCK2*/
int ncp6335_init(struct pmic_info *info)
{
	if (!ncp6335_check())
		return -1;

	printf("NCP6335: disabled\n");

	ncp6335_reg_write(NCP6335_REG_PROGVSEL0, 0x00);
	ncp6335_reg_write(NCP6335_REG_PROGVSEL1, 0x00);

	return 0;
}
#endif

