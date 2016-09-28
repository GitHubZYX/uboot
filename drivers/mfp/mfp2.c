
#include <common.h>

int comip_mfp_config(mfp_pin id, mfp_pin_mode mode)
{
	unsigned long reg;
	unsigned int val;

	if ((id >= MFP_PIN_MAX)
		|| (mode >= MFP_PIN_MODE_MAX))
		return -1;

	reg = MFP_REG(id);
	val = readl(reg);
	val = (val & (~MFP_AF_MASK)) | MFP_AF(mode);
	writel(val, reg);

	return 0;
}

int comip_mfp_config_pull(mfp_pin id, mfp_pull_mode mode)
{
	unsigned long reg;
	unsigned int val;

	if ((id >= MFP_PIN_MAX)
		|| (mode > MFP_PULL_MODE_MAX))
		return -1;

	reg = MFP_REG(id);
	val = readl(reg);
	val = (val & (~MFP_PULL_MASK)) | MFP_PULL(mode);
	writel(val, reg);

	return 0;
}

int comip_mfp_config_ds(mfp_pin id, mfp_ds_mode mode)
{
	unsigned long reg;
	unsigned int val;

	if ((id >= MFP_PIN_MAX)
		|| (mode > MFP_DS_MODE_MAX))
		return -1;

	reg = MFP_REG(id);
	val = readl(reg);
	val = (val & (~MFP_DS_MASK)) | MFP_DS(mode);
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

int comip_mfp_config_ds_array(struct mfp_ds_cfg *ds, int size)
{
	int i;

	if (!ds || !size)
		return -1;

	for (i = 0; i < size; i++)
		comip_mfp_config_pull(ds[i].id, ds[i].mode);

	return 0;
}


