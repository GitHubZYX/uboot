
#include <common.h>
#include <errno.h>
#include <mmc.h>
#include "error.h"

DECLARE_GLOBAL_DATA_PTR;

#if !defined(CONFIG_EFI_PARTITION)
const struct partition partitions[] = {
	CONFIG_COMIP_PARTITIONS
};
#endif

static struct partition* partition_find_by_name(const char *name)
{
	if (!name)
		return NULL;

#if defined(CONFIG_EFI_PARTITION)
	static struct partition part ;
	struct mmc *mmc = find_mmc_device(0);

	if (gpt_find_part_by_name(&mmc->block_dev, name, &part))
		return (struct partition*)&part ;
	else
		return NULL ;

#else
	int i;
	for (i = 0; i < ARRAY_SIZE(partitions); i++)
		if (!strcmp(name, partitions[i].name))
			return (struct partition*)&partitions[i];
#endif

	return NULL;
}

struct partition* partition_find_by_start(ulong start)
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

int flash_partition_start_find(const char *name)
{
#if defined(CONFIG_EFI_PARTITION)
	static struct partition part ;
	struct mmc *mmc = find_mmc_device(0);

	if (gpt_find_part_by_name(&mmc->block_dev, name, &part))
		return part.start;
	else
		return 1;
#else
	int i;
	for (i = 0; i < ARRAY_SIZE(partitions); i++)
		if (!strcmp(name, partitions[i].name))
			return partitions[i].start;
#endif
	return 1;
}


#if defined(CONFIG_COMIP_NAND)
#if defined(CONFIG_NAND_UBIFS)
const char* ubifs_partitions[] = {
	CONFIG_PARTITION_AMT,
	CONFIG_PARTITION_CACHE,
	CONFIG_PARTITION_SYSTEM,
	CONFIG_PARTITION_USERDATA,
};

const char* yaffs_partitions[] = {
};
#else
const char* ubifs_partitions[] = {
};

const char* yaffs_partitions[] = {
	CONFIG_PARTITION_AMT,
	CONFIG_PARTITION_CACHE,
	CONFIG_PARTITION_SYSTEM,
	CONFIG_PARTITION_USERDATA,
};
#endif

static ulong partition_is_yaffs2(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(yaffs_partitions); i++)
		if (!strcmp(name, yaffs_partitions[i]))
			return 1;

	return 0;
}

static ulong partition_is_ubifs(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ubifs_partitions); i++)
		if (!strcmp(name, ubifs_partitions[i]))
			return 1;

	return 0;
}
#endif

int flash_init(void)
{
	printf("flash init...\n");

#if defined(CONFIG_COMIP_MMC)
	struct mmc *mmc;

	mmc_initialize(gd->bd);

	mmc = find_mmc_device(0);
	if (!mmc) {
		printf("no mmc device at slot 0\n");
		pmic_power_off();
	}

	mmc_init(mmc);

	if(0 == mmc->read_bl_len){
		printf("mmc->read_bl_len == 0\n");
		pmic_power_off();
	}

	if(0 == mmc->block_dev.lba){
		printf("mmc->block_dev.lba == 0\n");
		mmc->block_dev.lba = 0xEE000;
		pmic_power_off();
	}

	flash_partition_fixup(mmc->capacity);

#elif defined(CONFIG_COMIP_NAND)
	if (nand_init()) {
		printf("nand init failed!\n");
		pmic_power_off();
	}
#endif

	printf("flash init end\n");

	return 0;
}

void flash_partition_fixup(u64 capacity)
{
	struct partition* pt;

	pt = partition_find_by_name(CONFIG_PARTITION_USERDATA);
	if (!pt) {
		printf("cann't find %s partition!!\n", CONFIG_PARTITION_USERDATA);
		return ;
	}

	capacity = (capacity / 1024) / 1024; //MB

	pt->length = (capacity  * 1024 * 2) - pt->start;  // block num
}


#if !defined(CONFIG_EFI_PARTITION)
#if defined(CONFIG_COMIP_NAND)
int flash_partition_string(char *buffer)
{
	const char header[] = "mtdparts=comip-nand:";
	char temp[30];
	int len = 0;
	int i;

	strcpy(buffer, header);
	len += strlen(header);

	for (i = 0; i < ARRAY_SIZE(partitions); i++) {
		sprintf(temp, "%ldk@%ldk(%s)",
			partitions[i].length / (1024 / partitions[i].sector_size),
			partitions[i].start / (1024 / partitions[i].sector_size),
			partitions[i].name);
		strcat(&buffer[len], temp);
		len += strlen(temp);

		if (i != (ARRAY_SIZE(partitions) - 1))
			strcat(&buffer[len++], ",");
	}

	strcat(&buffer[len++], " ");

#if defined(CONFIG_NAND_UBIFS)
	for (i = 0; i < ARRAY_SIZE(partitions); i++) {
		if (partition_is_ubifs(partitions[i].name)) {
			sprintf(temp, "ubi.mtd=%s ", partitions[i].name);
			strcat(&buffer[len], temp);
			len += strlen(temp);
		}
	}

	strcat(&buffer[len++], " ");
#endif

	return len;
}
#else /* EMMC */
int flash_partition_string(char *buffer)
{
	const char header[] = "lcpart=mmcblk0=";
	char temp[30];
	int len = 0;
	int i;

	strcpy(buffer, header);
	len += strlen(header);

	for (i = 1; i < ARRAY_SIZE(partitions); i++) {
		sprintf(temp, "%s:%lx:%lx:%lx",
			partitions[i].name,
			partitions[i].start,
			partitions[i].length,
			partitions[i].sector_size);
		strcat(&buffer[len], temp);
		len += strlen(temp);

		if (i != (ARRAY_SIZE(partitions) - 1))
			strcat(&buffer[len++], ",");
	}

	strcat(&buffer[len++], " ");

	return len;
}
#endif
#else
int flash_partition_string(char *buffer)
{
	/* Do nothing. */
	return 0;
}
#endif

int flash_partition_read(const char *name, uchar *buffer, ulong size)
{
	struct partition* pt;
	u64 start;
	u64 length;
	int ret = -EIO;

	pt = partition_find_by_name(name);
	if (!pt) {
		put_error("40001");
		printf("cann't find %s partition!!\n", name);
		return -EINVAL;
	}

	start = pt->sector_size * pt->start;
	length = pt->sector_size * pt->length;

	if (length > size)
		length = size;

#if defined(CONFIG_COMIP_MMC)
	struct mmc *mmc = find_mmc_device(0);
	if (mmc->block_dev.block_read(0, start / mmc->read_bl_len,
			length / mmc->read_bl_len, buffer) > 0)
		ret = 0;
#elif defined(CONFIG_COMIP_NAND)
	ret = nand_read(buffer, start, length,
		partition_is_yaffs2(name) ? NANDF_OOB_YAFFS : 0);
#endif
	if (ret) {
		printf("read image(%s) failed(%d)!! start=0x%08llx, size=0x%08llx\n", name, ret, start, length);
		return ret;
	}

	return 0;
}

int flash_partition_read_all(const char *name, uchar *buffer, ulong size)
{
	struct partition* pt;
	u64 start;
	u64 length;
	int ret = -EIO;

	pt = partition_find_by_name(name);
	if (!pt) {
		put_error("40001");
		printf("cann't find %s partition!!\n", name);
		return -EINVAL;
	}

	start = pt->sector_size * pt->start;
	length = pt->sector_size * pt->length;

	if (length > size)
		length = size;

#if defined(CONFIG_COMIP_MMC)
	struct mmc *mmc = find_mmc_device(0);
	if (mmc->block_dev.block_read(0, start / mmc->read_bl_len,
			length / mmc->read_bl_len, buffer) > 0)
		ret = 0;
#elif defined(CONFIG_COMIP_NAND)
	ret = nand_read(buffer, start, length,
		(partition_is_yaffs2(name) ? NANDF_OOB_YAFFS : 0) | NANDF_IGNORE_ERR);
#endif
	if (ret) {
		printf("read image(%s) failed(%d)!! start=0x%08llx, size=0x%08llx\n", name, ret, start, length);
		return ret;
	}

	return 0;
}

int flash_get_block_size(void)
{
#if defined(CONFIG_COMIP_MMC)
	struct mmc *mmc = find_mmc_device(0);
	return mmc->read_bl_len;
#elif defined(CONFIG_COMIP_NAND)
	return 0;
#endif
}

int flash_block_read(const char *name, ulong offstart, uchar *buffer, ulong size)
{
	struct partition* pt;
	ALLOC_CACHE_ALIGN_BUFFER(char, tmp, 512);
	u64 part_start;
	u64 part_length;
	int ret = 0;

	debug("read %s from start:0x%lx  size:0x%lx to %p\n", name, offstart, size, buffer);

	pt = partition_find_by_name(name);
	if (!pt) {
		put_error("40001");
		printf("cann't find %s partition!!\n", name);
		return -EINVAL;
	}

	part_start = pt->sector_size * pt->start;
	part_length = pt->sector_size * pt->length;

	if ((offstart + size) > part_length) {
		put_error("40002");
		printf("read (%s) is too large(0x%08lx + 0x%08lx > 0x%08llx)!!\n",
			name, offstart, size, part_length);
		return -EINVAL;
	}


#if defined(CONFIG_COMIP_MMC)
	struct mmc *mmc = find_mmc_device(0);
	if (!buffer || offstart%mmc->read_bl_len) {
		printf("buffer:%p,start addr:0x%lx,read page:0x%x\n",
			buffer, offstart, mmc->read_bl_len);
		return -EINVAL;
	}

	/* Get real read info. */
	part_length = size % mmc->read_bl_len;
	part_start = part_start + offstart;

	if (size / mmc->read_bl_len) {
		if (mmc->block_dev.block_read(0, part_start / mmc->read_bl_len,
			size / mmc->read_bl_len, buffer) > 0)
			ret = 0;
		else
			ret = -EIO;
	}

	/* Read remain data. */
	if (part_length && 0 == ret) {
		debug("read remain data:%lld\n", part_length);
		if (mmc->block_dev.block_read(0,
			(part_start + size) / mmc->read_bl_len, 1, tmp) > 0)
			ret = 0;
		memcpy(buffer + size - part_length, tmp, part_length);
	}
#elif defined(CONFIG_COMIP_NAND)
	#error need debug
	part_start = part_start + offstart;
	ret = nand_read(buffer, part_start, size,
		(partition_is_yaffs2(name) ? NANDF_OOB_YAFFS : 0) | NANDF_IGNORE_ERR);
#endif
	if (ret) {
		printf("read image(%s) failed(%d)!! offstart=0x%08lx, size=0x%08lx\n",
			name, ret, offstart, size);
		return ret;
	}

	return 0;
}

int flash_block_write(const char *name, ulong offstart, uchar *buffer, ulong size)
{
	struct partition* pt;
	u64 part_start;
	u64 part_length;
	int ret = -EIO;

	pt = partition_find_by_name(name);
	if (!pt) {
		put_error("40001");
		printf("cann't find %s partition!!\n", name);
		return -EINVAL;
	}
	debug("write:offstart:%lx,size:%lx\n", offstart, size);

	part_start = pt->sector_size * pt->start;
	part_length = pt->sector_size * pt->length;

	if ((offstart + size) > part_length) {
		put_error("40002");
		printf("write (%s) is too large(0x%08lx + 0x%08lx > 0x%08llx)!!\n",
			name, offstart, size, part_length);
		return -EINVAL;
	}

#if defined(CONFIG_COMIP_MMC)
	struct mmc *mmc = find_mmc_device(0);
	/* If data is less than 1 page, write 1 more page.
	 * So next page maybe destroyed. */
	if (size % mmc->write_bl_len)
		part_length = 1;
	else
		part_length = 0;

	/* Get real write info. */
	part_start = part_start + offstart;

	if (mmc->block_dev.block_write(0, part_start / mmc->write_bl_len,
			size / mmc->write_bl_len + part_length, buffer) > 0)
		ret = 0;
#elif defined(CONFIG_COMIP_NAND)
	#error need debug
	part_start = part_start + offstart;
	ret = nand_write(buffer, part_start, size,
			partition_is_yaffs2(name) ? NANDF_OOB_YAFFS : 0);
#endif
	if (ret) {
		printf("write image(%s) failed(%d)!! offstart=0x%08lx, size=0x%08lx\n",
			name, ret, offstart, size);
		return ret;
	}

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

#if defined(CONFIG_FALSH_WRITE)

#if defined(CONFIG_COMIP_FASTBOOT)
extern int check_sparse_image(void *data);
extern int  write_unsparse(uchar *source, u64 size,
			u64 start, u64 partition_size,
			ulong flags);
#endif

int flash_partition_write(const char *name, uchar *buffer, ulong size)
{
	struct partition* pt;
	u64 start;
	u64 length;
	int ret = -EIO;

	pt = partition_find_by_name(name);
	if (!pt) {
		put_error("40001");
		printf("cann't find %s partition!!\n", name);
		return -EINVAL;
	}

	start = pt->sector_size * pt->start;
	length = pt->sector_size * pt->length;

	if (size > length) {
		printf("write image(%s) is too large(0x%08lx > 0x%08llx)!!\n", name, size, length);
		return -EINVAL;
	}

#if defined(CONFIG_COMIP_MMC)
	struct mmc *mmc = find_mmc_device(0);
	if (mmc->block_dev.block_write(0, start / mmc->read_bl_len,
			size / mmc->read_bl_len + 1, buffer) > 0)
		ret = 0;
#elif defined(CONFIG_COMIP_NAND)
	ret = nand_write(buffer, start, size,
			partition_is_yaffs2(name) ? NANDF_OOB_YAFFS : 0);
#endif
	if (ret) {
		printf("write image(%s) failed(%d)!! start=0x%08llx, size=0x%08lx\n", name, ret, start, size);
		return ret;
	}

	return 0;
}

int flash_partition_erase_write(const char *name, uchar *buffer, ulong size)
{
	struct partition* pt;
	u64 start;
	u64 partition_size;
	u64 file_size;
	u64 temp_a;
	u64 temp_b;
	int ret = -EIO;

	pt = partition_find_by_name(name);
	if (!pt) {
		put_error("40001");
		printf("cann't find %s partition!!\n", name);
		return -EINVAL;
	}

	file_size=size;
	start = pt->sector_size * pt->start;
	temp_a=pt->sector_size;
	temp_b=pt->length;
	partition_size = temp_a*temp_b ;

	printf("pt->sector_size=0x%lx,  pt->length=0x%lx\n",pt->sector_size,pt->length);
	printf("flash_partition_erase_write:partition_size=0x%llx\n",partition_size);
	printf("flash_partition_erase_write:start=0x%llx\n",start);
	if (file_size > partition_size) {
		put_error("40002");
		printf("write image(%s) is too large(0x%llx > 0x%llx)!!\n", name, file_size, partition_size);
		return -EINVAL;
	}

#if defined(CONFIG_COMIP_MMC)
	struct mmc *mmc = find_mmc_device(0);

#if defined(CONFIG_COMIP_FASTBOOT)
	if (check_sparse_image(buffer))
		ret = write_unsparse(buffer, file_size, start, partition_size, 0);
	else
#endif
	{
		if (mmc->block_dev.block_write(0, start / mmc->read_bl_len,
				file_size / mmc->read_bl_len + 1, buffer) > 0){
			ret = 0;
		}
	}
#elif defined(CONFIG_COMIP_NAND)
	ret = nand_erase(start, partition_size);
	if (!ret) {
		#if defined (CONFIG_COMIP_FASTBOOT)
		if (check_sparse_image(buffer))
			ret = write_unsparse(buffer, file_size, start, partition_size,
				partition_is_yaffs2(name) ? NANDF_OOB_YAFFS : 0);
		else
		#endif
		{
			ret = nand_write(buffer, start, file_size,
				partition_is_yaffs2(name) ? NANDF_OOB_YAFFS : 0);
		}
	}
#endif
	if (ret) {
		printf("write image(%s) failed(%d)!! start=0x%llx, size=0x%llx\n", name, ret, start, file_size);
		return ret;
	}

	return 0;
}

int flash_partition_erase(const char *name)
{
	struct partition* pt;
	u64 start;
	u64 length;
	int ret = -EIO;

	pt = partition_find_by_name(name);
	if (!pt) {
		put_error("40001");
		printf("cann't find %s partition!!\n", name);
		return -EINVAL;
	}

	start = pt->sector_size * pt->start;
	length = pt->sector_size * pt->length;

#if defined(CONFIG_COMIP_MMC)
	struct mmc *mmc = find_mmc_device(0);
	if (mmc->block_dev.block_erase(0, start / mmc->read_bl_len, length) > 0)
		ret = 0;
#elif defined(CONFIG_COMIP_NAND)
	ret = nand_erase(start, length);
#endif
	if (ret) {
		printf("erase image(%s) failed(%d)!! start=0x%08llx, size=0x%08llx\n", name, ret, start, length);
		return ret;
	}

	return 0;
}
#endif

