
#include <common.h>

static unsigned long mfp_regs[] = {
	MUX_PIN_MUX0,			/* 0. */
	MUX_PIN_MUX1,			/* 1. */
	MUX_PIN_MUX2,			/* 2. */
	MUX_PIN_MUX3,			/* 3. */
	MUX_PIN_MUX4,			/* 4. */
	MUX_PIN_MUX5,			/* 5. */
	MUX_PIN_MUX6,			/* 6. */
	MUX_PIN_MUX7,			/* 7. */
	MUX_PIN_MUX8,			/* 8. */
	MUX_PIN_MUX9,			/* 9. */
	MUX_PIN_MUX10,			/* 10. */
	MUX_PIN_MUX11,			/* 11. */
	MUX_PIN_MUX12,			/* 12. */
	MUX_PIN_MUX13,			/* 13. */
	MUX_PIN_MUX14,			/* 14. */
	MUX_PIN_MUX15,			/* 15. */
};

static unsigned long mfp_pull_regs[] = {
	MUX_PIN_PULL_EN0,		/* 0. */
	MUX_PIN_PULL_EN1,		/* 1. */
	MUX_PIN_PULL_EN2,		/* 2. */
	MUX_PIN_PULL_EN3,		/* 3. */
	MUX_PIN_PULL_EN4,		/* 4. */
	MUX_PIN_PULL_EN5,		/* 5. */
	MUX_PIN_PULL_EN6,		/* 6. */
	MUX_PIN_PULL_EN7,		/* 7. */
	MUX_PIN_SDMMC0_PAD_CTRL,	/* 8. */
	MUX_PIN_SDMMC1_PAD_CTRL,	/* 9. */
	MUX_PIN_SDMMC2_PAD_CTRL,	/* 10. */
	MUX_PIN_SDMMC3_PAD_CTRL,	/* 11. */
};

int comip_mfp_config(mfp_pin id, mfp_pin_mode mode)
{
	struct mfp *mfp;
	unsigned long reg;
	unsigned int reg_id;
	unsigned int reg_bit;
	unsigned int val;

	if ((id >= MFP_PIN_MAX)
		|| (mode >= MFP_PIN_MODE_MAX))
		return -1;

	mfp = (struct mfp *)&mfp_pin_list;
	if (mfp[id].flags & MFP_PIN_GPIO_ONLY)
		return 0;

	if (mode == mfp[id].val)
		return -1;

	if ((mode == MFP_PIN_MODE_GPIO)
		&& (mfp[id].val >= MFP_PIN_MODE_MAX))
		return -1;

	reg_id = mfp[id].mux / 16;
	reg_bit = (mfp[id].mux % 16) << 1;
	if (reg_id > ARRAY_SIZE(mfp_regs))
		return -1;

	reg = mfp_regs[reg_id];
	val = readl(reg);
	val &= ~(0x3 << reg_bit);
	if (mode == MFP_PIN_MODE_GPIO)
		val |= (mfp[id].val << reg_bit);
	else
		val |= (mode << reg_bit);
	writel(val, reg);

	return 0;
}

int comip_mfp_config_pull(mfp_pull_id id, mfp_pull_mode mode)
{
	struct mfp_pull *pull;
	unsigned long reg;
	unsigned int val;

	if ((id >= mfp_pull_list_size)
		|| (mode > MFP_PULL_MODE_MAX))
		return -1;

	pull = &mfp_pull_list[id];
	if ((pull->id != id) || !(pull->caps & (1 << mode)))
		return -1;

	if ((mode != MFP_PULL_DISABLE)
		&& ((pull->caps & MFP_PULL_ALL_CAP) == MFP_PULL_ALL_CAP)) {
		reg = mfp_pull_regs[pull->pull_reg_id];
		val = readl(reg);
		if (mode == MFP_PULL_UP)
			val &= ~(1 << pull->pull_bit);
		else
			val |= (1 << pull->pull_bit);
		writel(val, reg);
	}

	reg = mfp_pull_regs[pull->en_reg_id];
	val = readl(reg);
	if (mode == MFP_PULL_DISABLE)
		val |= (1 << pull->en_bit);
	else
		val &= ~(1 << pull->en_bit);
	writel(val, reg);

	return 0;
}

int comip_mfp_config_array(struct mfp_pin_cfg *pin, int size)
{
	int i;

	if (!pin || !size)
		return -1;

	for (i = 0; i < size; i++)
		comip_mfp_config(pin[i].id, pin[i].mode);

	return 0;
}

int comip_mfp_config_pull_array(struct mfp_pull_cfg *pull, int size)
{
	int i;

	if (!pull || !size)
		return -1;

	for (i = 0; i < size; i++)
		comip_mfp_config_pull(pull[i].id, pull[i].mode);

	return 0;
}

