
#include <common.h>

DECLARE_GLOBAL_DATA_PTR;

#define DDRONFLASH_MAGIC	0xc8c46b5c

#define DDRONFLASH_VERSION	15

struct ddr_on_flash_t {
	__le32 magic;
	__le32 version;
	__le32 checksum;
	__le32 total;
	__le32 init_data;
	__le32 init_data_len;
	__le32 ddr0_trn_data;
	__le32 ddr0_trn_data_len;
	__le32 ddr1_trn_data;
	__le32 ddr1_trn_data_len;
};

extern struct memctl_data *memctl_init_datas;
extern unsigned int memctl_init_datas_length;

extern const unsigned int memctl_dqs_length;
extern struct memctl_data * memctl1_dqs;
extern struct memctl_data * memctl0_dqs;

extern int flash_block_write(const char *name, ulong offstart, uchar *buffer, ulong size);
extern int flash_block_read(const char *name, ulong offstart, uchar *buffer, ulong size);
extern int flash_get_block_size(void);

#if 0 //debug
int dump_datas(struct memctl_data *data, unsigned int len)
{
	unsigned int i;
	/* Config timing and other registers. */
	for (i = 0; i < len; i++)
		printf("{%2x}[%8x]=%8x\n", i, data[i].reg, data[i].val);

	return 0;
}

int dump_all(char * str)
{
	printf("dump all %s\n", str);
	printf("memctl_init_datas %p\n",memctl_init_datas);
	dump_datas(memctl_init_datas, memctl_init_datas_length / sizeof(struct memctl_data));

	printf("memctl0_dqs:%p\n", memctl0_dqs);
	dump_datas(memctl0_dqs,
				memctl_dqs_length/sizeof(struct memctl_data));
	printf("memctl1_dqs:%p\n" ,memctl1_dqs);
	dump_datas(memctl1_dqs,
			memctl_dqs_length/sizeof(struct memctl_data));
	return 0;
}
#endif

unsigned int ddr_get_checksum(unsigned char *init_data, unsigned int length)
{
	unsigned int checksum = 0;
	unsigned int *data = (unsigned int *)init_data;
	unsigned int len = length/sizeof(unsigned int);
	int i;

	for (i = 0; i < len; i++)
		checksum += data[i];

	printf("checksum:%x\n", checksum);
	return checksum;
}

int ddr_need_training(unsigned int checksum)
{
	struct ddr_on_flash_t * ddr_on_flash = ((struct ddr_on_flash_t *)(&gd->g_ddr_on_flash));
	int blksz = flash_get_block_size();

	if (flash_block_read(CONFIG_PARTITION_DDR, 0, (uchar *)ddr_on_flash, sizeof(struct ddr_on_flash_t))) {
		printf("read config header error!\n");
		return 1;
	}

	if (ddr_on_flash->magic != DDRONFLASH_MAGIC)
		return 1;

	if (ddr_on_flash->version != DDRONFLASH_VERSION)
		return 1;

	if (ddr_on_flash->checksum != checksum)
		return 1;

	if (ddr_on_flash->init_data % blksz
		|| ddr_on_flash->ddr0_trn_data % blksz
		|| ddr_on_flash->ddr1_trn_data % blksz)
		return 1;

	if (ddr_on_flash->total != (ddr_on_flash->init_data_len
		+ ddr_on_flash->ddr0_trn_data_len
		+ ddr_on_flash->ddr1_trn_data_len
		+ sizeof(struct ddr_on_flash_t)))
		return 1;

	if (memctl_init_datas_length != ddr_on_flash->init_data_len)
		return 1;

	if (gd->dram_size)
		if (ddr_on_flash->ddr0_trn_data_len != memctl_dqs_length)
			return 1;
	if (gd->memctl1_cs0_size || gd->memctl1_cs1_size)
		if (ddr_on_flash->ddr1_trn_data_len != memctl_dqs_length)
			return 1;

	return 0;
}

int ddr_restore_config_data(void)
{
	int ret;
	struct ddr_on_flash_t * ddr_on_flash = ((struct ddr_on_flash_t *)(&gd->g_ddr_on_flash));

	debug("restore:init:0x%x->%p,dqs0:0x%x->%p,dqs1:0x%x->%p\n",
		ddr_on_flash->init_data, memctl_init_datas,
		ddr_on_flash->ddr0_trn_data, memctl0_dqs,
		ddr_on_flash->ddr1_trn_data, memctl1_dqs);

	ret = flash_block_read(CONFIG_PARTITION_DDR, ddr_on_flash->init_data,
		(unsigned char *)memctl_init_datas, memctl_init_datas_length);
	if (ret) {
		printf("read init data error %d\n", ret);
		return ret;
	}

	if (ddr_on_flash->ddr0_trn_data) {
		ret = flash_block_read(CONFIG_PARTITION_DDR, ddr_on_flash->ddr0_trn_data,
			(unsigned char *)memctl0_dqs, memctl_dqs_length);
		if (ret) {
			printf("read ddr0 training data error %d\n", ret);
			return ret;
		}
	}

	if (ddr_on_flash->ddr1_trn_data) {
		ret = flash_block_read(CONFIG_PARTITION_DDR, ddr_on_flash->ddr1_trn_data,
			(unsigned char *)memctl1_dqs, memctl_dqs_length);
		if (ret) {
			printf("read ddr1 training data error %d\n", ret);
			return ret;
		}
	}

	return 0;
}

int ddr_save_config_data(unsigned int checksum)
{
	int ret;
	struct ddr_on_flash_t * ddr_on_flash = ((struct ddr_on_flash_t *)(&gd->g_ddr_on_flash));
	int blksz = flash_get_block_size();

	memset((unsigned char *)ddr_on_flash, 0, sizeof(struct ddr_on_flash_t));
	ddr_on_flash->magic = DDRONFLASH_MAGIC;
	ddr_on_flash->version = DDRONFLASH_VERSION;

	if (gd->dram_size)
		ddr_on_flash->ddr0_trn_data_len = memctl_dqs_length;
	else
		ddr_on_flash->ddr0_trn_data_len = 0;

	if (gd->memctl1_cs0_size || gd->memctl1_cs1_size)
		ddr_on_flash->ddr1_trn_data_len = memctl_dqs_length;
	else
		ddr_on_flash->ddr1_trn_data_len = 0;

	ddr_on_flash->init_data_len = memctl_init_datas_length;

	ddr_on_flash->total = (ddr_on_flash->init_data_len
		+ ddr_on_flash->ddr0_trn_data_len
		+ ddr_on_flash->ddr1_trn_data_len
		+ sizeof(struct ddr_on_flash_t));

	ddr_on_flash->init_data = ROUND(sizeof(struct ddr_on_flash_t), blksz);

	ddr_on_flash->checksum = checksum;

	if (gd->dram_size)
		ddr_on_flash->ddr0_trn_data = ROUND(ddr_on_flash->init_data + memctl_init_datas_length, blksz);
	else
		ddr_on_flash->ddr0_trn_data = 0;

	if (gd->memctl1_cs0_size || gd->memctl1_cs1_size)
		ddr_on_flash->ddr1_trn_data = ROUND(ddr_on_flash->ddr0_trn_data + memctl_dqs_length, blksz);
	else
		ddr_on_flash->ddr1_trn_data = 0;

	debug("ddr_on_flash[%x-%x],init[%x-%x],trn0[%x-%x],tr1[%x-%x]\n", 
		0,sizeof(struct ddr_on_flash_t),
		ddr_on_flash->init_data, ddr_on_flash->init_data_len,
		ddr_on_flash->ddr0_trn_data,ddr_on_flash->ddr0_trn_data_len,
		ddr_on_flash->ddr1_trn_data,ddr_on_flash->ddr1_trn_data_len);

	ret = flash_block_write(CONFIG_PARTITION_DDR, ddr_on_flash->init_data,
		(unsigned char *)memctl_init_datas, memctl_init_datas_length);
	if (ret) {
		printf("write init data error %d\n", ret);
		return ret;
	}

	if (ddr_on_flash->ddr0_trn_data) {
		ret = flash_block_write(CONFIG_PARTITION_DDR, ddr_on_flash->ddr0_trn_data,
			(unsigned char *)memctl0_dqs, memctl_dqs_length);
		if (ret) {
			printf("write ddr0 training data error %d\n", ret);
			return ret;
		}
	}

	if (ddr_on_flash->ddr1_trn_data) {
		ret = flash_block_write(CONFIG_PARTITION_DDR, ddr_on_flash->ddr1_trn_data,
			(unsigned char *)memctl1_dqs, memctl_dqs_length);
		if (ret) {
			printf("write ddr1 training data error %d\n", ret);
			return ret;
		}
	}

	ret = flash_block_write(CONFIG_PARTITION_DDR, 0, 
		(unsigned char *)ddr_on_flash, sizeof(struct ddr_on_flash_t));
	if (ret) {
			printf("write ddr_on_flash data error %d\n", ret);
			return ret;
	}

	return ret;
}
