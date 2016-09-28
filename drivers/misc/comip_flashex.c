
#include <common.h>
#include <errno.h>
#include <mmc.h>

#include <linux/types.h>

DECLARE_GLOBAL_DATA_PTR;

#if !defined(CONFIG_EFI_PARTITION)
const struct partition partitions[] = {
	CONFIG_COMIP_PARTITIONS
};
#endif

struct tag* comip_set_boot_params(struct tag *params) { return NULL; }

int flash_init(void)
{
	printf("flash init...\n");

#if defined(CONFIG_COMIP_MMC)
	struct mmc *mmc;

	mmc_initialize(gd->bd);

	mmc = find_mmc_device(0);
	if (!mmc) {
		printf("no mmc device at slot 0\n");
	}

	mmc_init(mmc);
	mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BOOT_BUS_CONDITIONS, 1);


	if(0 == mmc->read_bl_len){
		printf("mmc->read_bl_len == 0\n");
	}

	if(0 == mmc->block_dev.lba){
		printf("mmc->block_dev.lba == 0\n");
		mmc->block_dev.lba = 0xEE000;
	}
#elif defined(CONFIG_COMIP_NAND)
	if (nand_init()) {
		printf("nand init failed!\n");
	}
#endif

	printf("flash init end\n");
	return 0;
}


struct mmc * flashex_get_device(void)
{
	struct mmc *mmc = NULL;

	mmc = find_mmc_device(0);
	if (!mmc) {
		printf("can't find mmc device.\n");
		while(1);
	}
	return mmc;
}

#ifdef CONFIG_TL_PTSZ_CHECK
static struct partition* partition_find_by_stblk(ulong start)
{
#if defined(CONFIG_EFI_PARTITION)
	/* do nothing */
#else
	int i;
	for (i = 0; i < ARRAY_SIZE(partitions); i++)
		if (start == partitions[i].start)
			return (struct partition*)&partitions[i];
#endif
	return NULL;
}
#endif

static struct partition* partition_find_by_name(const char *name)
{
	if (!name)
		return NULL;

	int i;
	for (i = 0; i < ARRAY_SIZE(partitions); i++)
		if (!strcmp(name, partitions[i].name))
			return (struct partition*)&partitions[i];

	return NULL;
}

static int flashex_boot_partition_switch(u8 flag)
{
	struct mmc *mmc = flashex_get_device();
	u8 part_num;

	if (flag) {
		part_num = (mmc->part_config & PART_BOOT_ENABLE_MASK) >> 3;
		if((PART_BOOT_1 != part_num) && (PART_BOOT_2 != part_num)) {
			/* Fix me, force to switch to Boot partition? */
			part_num = PART_BOOT_1;
			//return -EINVAL;
		}
	} else {
		part_num = PART_USER;
	}

	if (mmc_switch_part(0, part_num)) {
		return -EINVAL;
	}

	return 0;
}

int flashex_partiton_find_by_name(char *name, u32 *pblk_start, u32 *pblk_len, u32 *pblk_size)
{
	struct partition* pt;

	pt = partition_find_by_name(name);
	if (!pt) {
		printf("cann't find %s partition!!\n", name);
		return -EINVAL;
	}

	*pblk_start = pt->start;
	*pblk_len = pt->length;
	*pblk_size = pt->sector_size;

	return 0;
}

int flashex_partition_check_switch(u64 start, u64 length)
{
	/* SML now send only raw data.
	 * Zip image will be sent as raw data for more than once.
	 * So the start addr and length might not equal to any partiton start.
	 *
	 * CONFIG_TL_PTSZ_CHECK means:
	 * Whether target loader to check partition size is necessary.
	 * It is rarely wrong not to define it.
	 */
#ifdef CONFIG_TL_PTSZ_CHECK
	struct mmc *mmc = flashex_get_device();
	ulong start_blk = start / (mmc->read_bl_len);
	u64 limit = 0;

	struct partition *pt = partition_find_by_stblk(start_blk);
	if (NULL == pt) {
		printf("can't find the partition!\n");
		return -EINVAL;
	}
	printf("found partition: start(0x%llx) => start_blk(0x%lx)\n", start, start_blk);

	limit = (pt->sector_size) * (pt->length);
	if (length > limit) {
		printf("length(0x%llx) is larger than partition size(0x%llx)!\n", length, limit);
		return -EINVAL;
	}
#endif

	if (!start)
		return flashex_boot_partition_switch(1);
	else
		return flashex_boot_partition_switch(0);
}

int flashex_erase(u64 start, u64 len)
{
	struct mmc *mmc = flashex_get_device();
	ulong blk_size = mmc->write_bl_len;
	ulong start_blk = start / blk_size;
	ulong blkcnt = 0;
	ulong cnt = 0;
	u64 rest_size = 0;

	u8 part_num = PART_USER;
	int ret = 0;

	if (!len || (start % blk_size))
		goto out;


	if ( len > mmc->capacity) {
		printf("fix len to capacity(0x%llx).\n", mmc->capacity);
		len = mmc->capacity;
	}

	if (!start) { /* to erase boot partition */
		part_num = (mmc->part_config & PART_BOOT_ENABLE_MASK) >> 3;
		if((PART_BOOT_1 != part_num) && (PART_BOOT_2 != part_num)) {
			goto out;
		}

		if (len > mmc->boot_part_size) {
			rest_size = len - mmc->boot_part_size;
			len = mmc->boot_part_size;
			printf("fix len to 0x%x and erase boot part firstly.\n", mmc->boot_part_size);
		}
	}

	if (mmc_switch_part(0, part_num)) {
		goto out;
	}

erase_rest:
	blkcnt = (len % blk_size) ? ((len / blk_size) + 1) : (len / blk_size);
	cnt = mmc->block_dev.block_erase(0, start_blk, blkcnt);
	if (cnt != blkcnt) {
		goto out;
	}

	if (rest_size) {
		mmc_switch_part(0, PART_USER);
		len = rest_size;
		printf("erase rest_size(0x%llx) secondly!\n", len);
		rest_size = 0;
		goto erase_rest;
	}

	return ret;
out:
	ret = -EIO;
	return ret;
}

int flashex_write(u64 start, void *buf, u64 len)
{
	struct mmc *mmc = find_mmc_device(0);
	ulong blk_size = mmc->write_bl_len;
	ulong start_blk = start / blk_size;
	ulong blkcnt = 0;
	ulong cnt = 0;

	if (0 == len)
		return 0;

	blkcnt = (len % blk_size) ? ((len / blk_size) + 1) : (len / blk_size);
	cnt = mmc->block_dev.block_write(0, start_blk, blkcnt, buf);
	if (cnt != blkcnt)
		return -EIO;

	return 0;
}

int flashex_read(u64 start, void *buf, u32 len)
{
	struct mmc *mmc = find_mmc_device(0);
	ulong blk_size = mmc->read_bl_len;
	ulong start_blk = start / blk_size;
	ulong blkcnt = 0;
	ulong cnt = 0;

	blkcnt = (len % blk_size) ? ((len / blk_size) + 1) : (len / blk_size);
	cnt = mmc->block_dev.block_read(0, start_blk, blkcnt, buf);
	if (cnt != blkcnt)
		return -EIO;

	return 0;
}

int flash_get_devinfo(unsigned int *mid, unsigned int *did)
{
	int ret = -1;
#if defined(CONFIG_COMIP_MMC)
	struct mmc *mmc = find_mmc_device(0);
	if (mmc) {
		printf("vendor:[%s] product:[%s] revision[%s]\n",
			mmc->block_dev.vendor,
			mmc->block_dev.product,
			mmc->block_dev.revision);
		*mid = mmc->cid[0];
		*did = mmc->cid[1];
		ret = 0;
	} else {
		printf("mmc not found!!!\n");
	}
#endif

	return ret;
}
