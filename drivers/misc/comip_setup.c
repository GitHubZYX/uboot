
#include <common.h>

DECLARE_GLOBAL_DATA_PTR;

static struct tag* comip_set_cmdline_params(struct tag *params,unsigned long boot_mode)
{
#if CONFIG_BOOTARGS_ENABLE
	unsigned int size = 0x0;
	char *buf = (char *)params->u.cmdline.cmdline;

	switch (boot_mode){
	case BOOT_MODE_RECOVERY:
		size = strlen(CONFIG_RECOVERY_BOOTARGS);
		strcpy(buf, CONFIG_RECOVERY_BOOTARGS);
		break;
	case BOOT_MODE_NORMAL:
	case BOOT_MODE_AMT1:
	case BOOT_MODE_AMT3:
	default:
		if(pmic_power_up_reason_get() == PU_REASON_USB_CHARGER) {
			size = strlen(CONFIG_BOOTARGS_CHARGE_ONLY);
			strcpy(buf, CONFIG_BOOTARGS_CHARGE_ONLY);
		} else {
			size = strlen(CONFIG_BOOTARGS);
			strcpy(buf, CONFIG_BOOTARGS);
		}
		break;

	}

	size += flash_partition_string(&buf[size]);

	/* CMDLINE. */
	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size = (size + 3 + sizeof(struct tag_header)) >> 2;
	params = tag_next(params);
#endif

	return params;
}

struct tag* comip_set_boot_params(struct tag *params)
{
	struct tag_mem_ext *mem;
	struct tag_buck2_voltage *vol;
	struct tag_buck1_voltage *vol_buck1;
	struct tag_boot_info *binfo;
	struct tag_chip_id *chip_id;
	struct tag_mmc_tuning_addr *tuning_addr;
	struct pmic_info* info = (struct pmic_info*)(&gd->pmic_info);

	/* CMDLINE. */
	params = comip_set_cmdline_params(params,gd->boot_mode);

	/* Memory. */
	mem = (struct tag_mem_ext *)&params->u;
	params->hdr.tag = ATAG_MEM_EXT;
	params->hdr.size = tag_size(tag_mem_ext);

	mem->dram_base = gd->dram_base;
	mem->dram_size = gd->dram_size;
	mem->dram2_base = gd->dram2_base;
	mem->dram2_size = gd->dram2_size;
	mem->on2_fix = gd->on2_fix;
	mem->on2_size = gd->on2_size;
	mem->on2_base = gd->on2_base;
	mem->fb_fix = gd->fb_fix;
	mem->fb_size = gd->fb_size;
	mem->fb_base = gd->fb_base;
	mem->fb_num = gd->fb_num;

	params = tag_next(params);

	/* BUCK2 Voltage. */
	vol = (struct tag_buck2_voltage *)&params->u;
	params->hdr.tag = ATAG_BUCK2_VOLTAGE;
	params->hdr.size = tag_size(tag_buck2_voltage);
	vol->vout0 = info->buck2_vout0;
	vol->vout1 = info->buck2_vout1;
	vol->vout2 = info->buck2_vout2;
#if (BUCK2_VOUT_SEL == 3)
	vol->vout3 = info->buck2_vout3;
#else
	vol->vout3 = 0;
#endif
	params = tag_next(params);

	/* Boot information. */
	binfo = (struct tag_boot_info *)&params->u;
	memset(binfo, 0, sizeof(struct tag_boot_info));
	params->hdr.tag = ATAG_BOOT_INFO;
	params->hdr.size = tag_size(tag_boot_info);
	binfo->startup_reason = info->startup_type;
	binfo->shutdown_reason = info->shutdown_type;
	binfo->pu_reason = info->power_on_type;
	binfo->reboot_reason = info->reboot_type;
	binfo->boot_mode = gd->boot_mode;
	params = tag_next(params);

	/* BUCK1 Voltage. */
	vol_buck1 = (struct tag_buck1_voltage *)&params->u;
	memset(vol_buck1, 0, sizeof(struct tag_buck1_voltage));
	params->hdr.tag = ATAG_BUCK1_VOLTAGE;
	params->hdr.size = tag_size(tag_buck1_voltage);
	vol_buck1->vout0 = info->buck1_vout0;
	vol_buck1->vout1 = info->buck1_vout1;
	params = tag_next(params);

	/* Chip id. */
	chip_id = (struct tag_chip_id *)&params->u;
	memset(chip_id, 0, sizeof(struct tag_chip_id));
	params->hdr.tag = ATAG_CHIP_ID;
	params->hdr.size = tag_size(tag_chip_id);
	chip_id->chip_id = gd->chip_id;
	chip_id->rom_id = gd->rom_id;
	chip_id->bb_id = gd->bb_id;
	chip_id->sn_id = gd->sn_id;
	params = tag_next(params);

	/* Emmc tuning read addr. */
	tuning_addr = (struct tag_mmc_tuning_addr*)&params->u;
	memset(tuning_addr, 0, sizeof(struct tag_mmc_tuning_addr));
	tuning_addr->tuning_addr = flash_partition_start_find(CONFIG_PARTITION_KERNEL);
	if (tuning_addr->tuning_addr == 1) {
		printf("cann't find %s partition!!\n", CONFIG_PARTITION_KERNEL);
		return params;
	}

	params->hdr.tag = ATAG_MMC_TUNING_ADDR;
	params->hdr.size = tag_size(tag_mmc_tuning_addr);
	params = tag_next(params);

	return params;
}

