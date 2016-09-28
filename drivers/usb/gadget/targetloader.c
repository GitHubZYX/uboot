/*
 * drivers/usb/gadget/targetloader.c
 *
 * Use of source code is subject to the terms of the LeadCore license agreement
 * under which you licensed source code. If you did not accept the terms of the
 * license agreement, you are not authorized to use source code. For the terms
 * of the license, please see the license agreement between you and LeadCore.
 *
 * Copyright (c) 2010-2019  LeadCoreTech Corp.
 *
 * PURPOSE:
 * 	This file is for target loader process.
 *
 * CHANGE HISTORY:
 * 	Version	Date			Author		Description
 * 	1.0.0	2014-01-02		pengyimin	created
 */

#include <common.h>
#include <asm/errno.h>
#include <command.h>
#include <mmc.h>

#include "targetloader.h"
#include "efuse.h"

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_TL_USB
#define TLINFO(fmt, args...) printf(fmt, ##args)
#endif

#ifdef CONFIG_TL_SERIAL
#define TLINFO(fmt, args...)

/* For UART debug;
 * Add global var ts, dead loop to print.
 * Info will show in uart tool such as putty.
 */
#define TLDLP(fmt, args...) do { \
	printf(fmt, ##args); \
}while(1)
#endif

#define TLERR(fmt, args...) do { \
	printf("ERR! " fmt, ##args); \
}while(1)

//#define TARGETLOADER_DEBUG
#ifdef TARGETLOADER_DEBUG
#define TLDEBUG(fmt, args...) printf(fmt, ##args)
#else
#define TLDEBUG(fmt, args...)
#endif

extern int usb_udl_init(void);
extern int usb_udl_recv(void *p, int length);
extern int usb_udl_send(void *p, int length);

extern int uart_sdl_recv(void *p, int length);
extern int uart_sdl_send(void *p, int length);

extern void ap_sleep_wakeup(void);

static int (*tl_recv)(void *, int);
static int (*tl_send)(void *, int);
static u8 tl_force_resp = 0x0;

static unsigned long is_yaffs = NAND_OOB_NONE;
static unsigned long is_upload_yaffs = NAND_OOB_NONE; /*1: yaffs format  2: raw oob*/

/* ------ efuse function ------ */
static unsigned int efuse_data[32];
static unsigned int efuse_data_wtmp[32];
static unsigned int efuse_data_rtmp[32];
/* sample efuse data
{
	//0(4B) ID
	0x07440001, //1860 + 1
	// 1(16B) AES key
	0,0,0,0,  // NULL
	// 5(32B) SHA256
	0x02cd0b91,
	0x031c5ed1,
	0x46774c03,
	0xa61b5fc5,
	0x5fda323b,
	0x2025e2ab,
	0xb2eabfd8,
	0x7f302dd2,
	// 13(60B) custom use
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,
	// 28(16B) function code
	(1<<SE)|(0<<SET),
	0,0,0
};*/

static unsigned int pll1cfg = 1;
static unsigned int busmclk0 = 1;
static unsigned int busmclk1 = 1;

/* save ap bus clk setting and set it to 0.32M. */
void save_apbusclk(void)
{
	pll1cfg = readl(AP_PWR_PLL1CFG_CTL0);
	busmclk0 = readl(AP_PWR_BUSMCLK0_CTL);
	busmclk1 = readl(AP_PWR_BUSMCLK1_CTL);

	writel(0x12206402, AP_PWR_PLL1CFG_CTL0);
	writel(0xF70001, AP_PWR_BUSMCLK0_CTL);
	writel(0x700000, AP_PWR_BUSMCLK1_CTL);

	printf("old:pll1cfg:0x%x, busmclk0:0x%x, busmclk1:0x%x\n",
		pll1cfg, busmclk0, busmclk1);
	printf("dest:pll1cfg:0x%x, busmclk0:0x%x, busmclk1:0x%x\n",
		0x12206402, 0xF70001, 0x700000);
	ap_sleep_wakeup();
	printf("real:pll1cfg:0x%x, busmclk0:0x%x, busmclk1:0x%x\n",
		readl(AP_PWR_PLL1CFG_CTL0),
		readl(AP_PWR_BUSMCLK0_CTL),
		readl(AP_PWR_BUSMCLK1_CTL));
}

/* restore ap bus clk setting from 0.32M. */
void restore_apbusclk(void)
{
	writel(pll1cfg, AP_PWR_PLL1CFG_CTL0);
	writel(busmclk0, AP_PWR_BUSMCLK0_CTL);
	writel(busmclk1, AP_PWR_BUSMCLK1_CTL);

	ap_sleep_wakeup();
	printf("real2:pll1cfg:0x%x, busmclk0:0x%x, busmclk1:0x%x\n",
		readl(AP_PWR_PLL1CFG_CTL0),
		readl(AP_PWR_BUSMCLK0_CTL),
		readl(AP_PWR_BUSMCLK1_CTL));
}

int efuse_start_process(unsigned int r0w1, unsigned int addr, unsigned int len)
{
	u32 val = 0;
	u32 flag = 0;
	s32 timeout = 1000000;

	local_irq_save(flag);

	val = __raw_readl(EFUSE_MODE);
	if (r0w1)
		val |= (1 << EFUSE_MODE_MODE);
	else
		val &= ~(1 << EFUSE_MODE_MODE);
	__raw_writel(val, EFUSE_MODE);

	__raw_writel(addr, EFUSE_ADDR);
	__raw_writel(len, EFUSE_LENGTH);

	val = __raw_readl(SECURITY_INTR_EN);
	val |= (1 << SECURITY_INTR_EFUSE);
	__raw_writel(val, SECURITY_INTR_EN);

	/* Don't save/restore apbusclk for it cause USB no response!
	 * if (r0w1)
	 *	save_apbusclk();*/

	val = __raw_readl(EFUSE_CTRL);
	val |= (1 << EFUSE_CTRL_ENABLE);
	__raw_writel(val, EFUSE_CTRL);

	do {
		val = __raw_readl(SECURITY_INTR_STATUS);
		if (val & (1 << SECURITY_INTR_EFUSE)) {
			printf("%s efuse ok\n", r0w1?"write":"read");
			break;
		}
	} while(timeout-- > 0);

	if(timeout <= 0)
		printf("%s efuse timeout %d\n", r0w1?"write":"read", timeout);

	/* if (r0w1)
	 *	restore_apbusclk();*/

	val = __raw_readl(SECURITY_INTR_EN);
	val &= ~(1 << SECURITY_INTR_EFUSE);
	__raw_writel(val, SECURITY_INTR_EN);

	val = __raw_readl(SECURITY_INTR_STATUS);
	val |= (1 << SECURITY_INTR_EFUSE);
	__raw_writel(val, SECURITY_INTR_STATUS);

	local_irq_restore(flag);

	if(timeout > 0)
		return 0;
	else
		return -EIO;
}

int efuse_write(void)
{
	u32 i,j,m,n;
	int ret = 0;

//Write by Bit to ensure rightness
	for(m=0; m<32; m++) {
		for(n=0; n<32; n++) {
			if(efuse_data[m] & (1<<n)) {
				efuse_data_wtmp[m] |= (1<<n);
				printf("to w uint[%u][b:%u] (0x%08x)\n", m, n, efuse_data_wtmp[m]);
			}else {
				continue;
			}

			j = 0;
by_bit:
			writel(efuse_data_wtmp[m], EFUSE_PGM_DATA_BASE + m*4);

			ret = efuse_start_process(1, (m*32+n), 1);
			if (ret) {
				printf("w error!\n");
				goto by_bit;
			}

			//verify by bit
			ret = efuse_start_process(0, 0, sizeof(efuse_data_rtmp)*8);
			if (ret) {
				printf("r error!\n");
				return -EIO;
			} else {
				for (i=0; i<32; i++)
					efuse_data_rtmp[i] = readl(EFUSE_DATA_BASE + i*4);

				if(efuse_data_rtmp[m] & (1<<n)) {
					printf("bit w ok!\n");
				}else {
					j++;
					if (j>10) {
						printf("bit w over %d times and exit!\n",j);
						return -EIO;
					} else {
						printf("bit w more!\n");
						goto by_bit;
					}
				}
			}
		}
	}

	return ret;
}

int efuse_read(void)
{
	u32 i=0;
	int ret = 0;

	ret = efuse_start_process(0, 0, sizeof(efuse_data)*8);
	if (ret) {
		printf("%s error!\n", __func__);
	} else {
		for (i=0; i<32; i++)
			efuse_data[i] = readl(EFUSE_DATA_BASE + i*4);
		printf("%s OK!\n", __func__);
	}
	return ret;
}


/* ------ protocol helper function ------ */
static void makeup_u32val(u8 *data, u8 is_big_endian, u32 *out)
{
	u32 l = 0;

	if (NULL == out || NULL == data) {
		TLERR("%s: input err param.\n", __func__);
		return;
	}

	if (TRUE == is_big_endian) {
		l |= data[3];
		l |= data[2] << 8;
		l |= data[1] << 16;
		l |= data[0] << 24;
	} else {
		l |= data[0];
		l |= data[1] << 8;
		l |= data[2] << 16;
		l |= data[3] << 24;
	}

	*out = l;
}

static u8 calc_checksum(u8 *data, u16 size)
{
	u16 i = 0;
	u8 checksum = 0;

	for (; i < size; i++) {
		checksum = checksum ^ data[i];
	}

	return checksum;
}

static void set_packet(struct packet *ppk, u32 num,
                       u8 direction, u8 cmd_id, u16 data_size, u8 *data)
{
	if (num & 0x1)
		ppk->direction = direction;
	if (num & 0x2)
		ppk->cmd_id = cmd_id;
	if (num & 0x4)
		ppk->data_size = data_size;
	if (num & 0x8)
		ppk->data = data;
}

static void set_expect_info(struct expect_info *pexp, u32 num,
                       u8 flag, u8 cmd_id, u16 size)
{
	if (num & 0x1)
		pexp->flag = flag;
	if (num & 0x2)
		pexp->cmd_id = cmd_id;
	if (num & 0x4)
		pexp->size = size;
}

static void protocol_handle(struct packet *ppk, struct buffer_desc *pdesc)
{
	u8 *data_start = NULL;
	u16 data_len = 0;

	u8 local_checksum = 0;

	switch (ppk->direction) {
	case FROM_HOST:
		ppk->cmd_id = *(u8 *)(ppk->data);
		if (CMD_DATA == ppk->cmd_id) {
			data_start = ppk->data + CMD_DATA_FILL_SIZE + CMD_CMD_SIZE;
			data_len = ppk->data_size - (CMD_DATA_FILL_SIZE + CMD_CMD_SIZE + CMD_CRC_SIZE);
		} else {
			data_start = ppk->data + CMD_CMD_SIZE;
			data_len = ppk->data_size - (CMD_CMD_SIZE + CMD_CRC_SIZE);
		}

#ifdef TL_PACKET_CHECKSUM
		u8 recv_checksum = 0;

		if (CMD_DATA == ppk->cmd_id) {
			recv_checksum = *(u8 *)(ppk->data + data_len + CMD_DATA_FILL_SIZE + CMD_CMD_SIZE);
		} else {
			recv_checksum = *(u8 *)(ppk->data + data_len + CMD_CMD_SIZE);
		}

		local_checksum = calc_checksum(ppk->data, (ppk->data_size - 1));
		if (recv_checksum != locall_checksum) {
			TLINFO("checksum dismatch! (0x%02x vs 0x%02x)\n", recv_checksum, locall_checksum);
			ppk->cmd_id = CMD_FAIL;
			ppk->data = NULL;
			ppk->data_size = 0;
		} else
#endif
		{
			ppk->data = data_start;
			ppk->data_size = data_len;
		}
		break;

	case TO_HOST:
		if ( NULL == pdesc) {
			TLERR("pdesc is NULL\n");
		}

		memset(pdesc->buf, 0x0, UP_EXTRA_SIZE);
		/* 1byte for cmd_id*/
		(pdesc->buf)[0] = ppk->cmd_id;
		(ppk->data_size)++;

		if (CMD_DATA == ppk->cmd_id) {
			memcpy((pdesc->buf + CMD_CMD_SIZE), (pdesc->buf + UP_EXTRA_SIZE), (ppk->data_size - 1));

			/* 1byte for cmd_crc*/
			(ppk->data_size)++;
		} else {
			memcpy((pdesc->buf + CMD_CMD_SIZE), ppk->data, (ppk->data_size - 1));

			/* at least 8 bytes to response host */
			if (ppk->data_size < UP_EXTRA_SIZE) {
				ppk->data_size = UP_EXTRA_SIZE;
			} else {
			    /* 1byte for cmd_crc*/
				(ppk->data_size)++;
			}

		}

		local_checksum = calc_checksum(pdesc->buf, (ppk->data_size - 1));
		(pdesc->buf)[ppk->data_size - 1] = local_checksum;

		ppk->data = (pdesc->buf);
		ppk->cmd_id = CMD_NONE;
		break;

	default:
		TLINFO("unkown packet direction\n");
		break;
	}
}

void pre_handle(struct buffer_desc *pdesc,
			struct packet *ppk, struct expect_info *pexp)
{
	set_packet(ppk, 0xF, FROM_HOST, CMD_NONE, pdesc->size, pdesc->buf);

	protocol_handle(ppk, NULL);

	if (pexp->flag == SET) {
		if (ppk->cmd_id != pexp->cmd_id) {
			ppk->cmd_id = CMD_FAIL;
			TLINFO("recv cmd_id(0x%02x), but expect cmd_id(0x%02x)\n",
					ppk->cmd_id, pexp->cmd_id);
		}
	}
}

void soft_reset(void)
{
	TLERR("%s\n",__func__);
}

void hard_reset(void)
{
	TLERR("%s\n",__func__);
}

void prepare_resp_link_est(struct packet *ppk, struct expect_info *pexp)
{
	set_packet(ppk, 0x7, TO_HOST, CMD_CONNECT, 0, NULL);

	set_expect_info(pexp, 0x1, RESET, 0, 0);
	switch (*(u8 *)(ppk->data))	{
	case 0x06:
		TLDEBUG("%s: to set partition table.\n", __func__);
		set_expect_info(pexp, 0x7, SET, CMD_PARTITION_DATA, *(u8 *)(ppk->data_size + 1));
		break;

	case 0xf0:
		TLDEBUG("%s: to query TL version.\n", __func__);
		*(u8 *)(ppk->data) = TL_VERSION;
		ppk->data_size = 1;
		break;

	default:
		/* do nothing */
		TLDEBUG("%s: unkown sub command(0x%x).\n", __func__, *(u8 *)(ppk->data));
		break;
	}

}

void prepare_resp_unzip_info(struct packet *ppk,
					struct loader_set *pload)
{
	u8 index = 0;
	u8 type = 0;
	u32 val = 0;

	pload->is_zip = *(u8 *)(ppk->data);

	makeup_u32val((ppk->data+1), TRUE, &val);
	/* type: bits[6-7] index: bits[4-5] others:bits[0-3] should be zero */
	type =  (*((ppk->data) + 5) & 0xC0) >> 6;
	index = (*((ppk->data) + 5) & 0x30) >> 4;

	TLINFO("CMD_UNZIP_INFO: is_zip(%u) type(%u) index(%u) val(0x%x)\n",
			pload->is_zip, type, index, val);

	switch (type) {
	case 0x01:
		if (0 == index) {
			pload->start_addr = val;
		} else if ( 1 == index) {
			pload->allpacks_bytes = val;
		} else {
			TLINFO("unkown index!\n");
		}
		break;
	case 0x02:
		/* BigAddressOffset */
		pload->addr_offset += val;
		break;
	default:
		TLINFO("unkown type\n");
		break;
	}

	set_packet(ppk, 0x7, TO_HOST, CMD_ACK, 0, NULL);
}

void prepare_resp_dev_base_addr(struct packet *ppk)
{
	if (FLASH_TYPE_YAFFS2 == *(ppk->data))
		is_upload_yaffs = NAND_OOB_YAFFS;
	else if (FLASH_TYPE_OOB == *(ppk->data))
		is_upload_yaffs = NAND_OOB_RAW;

	set_packet(ppk, 0xf, TO_HOST, CMD_ACK, 0, NULL);
}

void prepare_resp_efuse_read(struct packet *ppk)
{
	u32 len = 0;

	makeup_u32val(ppk->data,TRUE,&len);
	TLINFO("TL send efuse_data len is %u\n",len);

	/* byte 0: operation status (0-OK, other-FAIL)
	 * byte 1~2: valid length in Big Endian
	 * byte 3: efuse data */
	ppk->data[0] = EFUSE_OK;
	/* fix efuse len now */
	ppk->data[1] = 0x00;
	ppk->data[2] = 0x80;

	if(efuse_read()) {
		ppk->data[0] = EFUSE_TMOUT;
	} else {
		memcpy(ppk->data + 3, (u8 *)efuse_data, sizeof(efuse_data));
	}

	set_packet(ppk, 0x7, TO_HOST, CMD_EFUSE_READ_RSP, (sizeof(efuse_data) + 4), NULL);
}

u32 prepare_resp_efuse_write_len(struct packet *ppk)
{
	u32 len;

	makeup_u32val(ppk->data, TRUE, &len);
	printf("efuse write len: %u bytes\n", len);

	set_packet(ppk, 0x7, TO_HOST, CMD_ACK, 0, NULL);
	return len;
}

void prepare_resp_efuse_write(struct packet *ppk)
{
	int i = 0;

	for(i = 0; i < sizeof(efuse_data)/sizeof(efuse_data[0]); i++) {
		makeup_u32val((ppk->data + i*4), FALSE, &efuse_data[i]);
		printf("0x%08x\n",efuse_data[i]);
	}

	if(efuse_write())
		set_packet(ppk, 0x7, TO_HOST, CMD_FAIL, 0, NULL);
	else
		set_packet(ppk, 0x7, TO_HOST, CMD_ACK, 0, NULL);
}

void prepare_resp_dev_info(struct packet *ppk)
{
	u32 vendor_id, device_id;
	u16 data_size = 12;

	struct mmc *pmmc = flashex_get_device();

	vendor_id = (u32)(((pmmc->cid[0])>>24) & 0xff);
	device_id = (u32)(((pmmc->cid[0])>>8) & 0xff);

	/*sub cmd type*/
	switch (*(u8 *)(ppk->data)) {
	case DEV_FST:
		memset(ppk->data, 0, ppk->data_size);
		*ppk->data = (vendor_id >> 8) & 0xff;
		*(ppk->data + 1) = vendor_id & 0xff;
		*(ppk->data + 2) = (device_id >> 24) & 0xff;
		*(ppk->data + 3) = (device_id >> 16) & 0xff;
		*(ppk->data + 4) = (device_id >> 8) & 0xff;
		*(ppk->data + 5) = device_id & 0xff;
		break;
	case DEV_EMMC_OCR:
		memcpy(ppk->data, &(pmmc->ocr), sizeof(pmmc->ocr));
		break;
	case DEV_EMMC_CID:
		memcpy(ppk->data, &(pmmc->cid[0]), sizeof(pmmc->cid));
		data_size = 16;
		break;
	case DEV_EMMC_CSD:
		memcpy(ppk->data, &(pmmc->csd[0]), sizeof(pmmc->csd));
		data_size = 16;
		break;
	case DEV_EMMC_RCA:
		memcpy(ppk->data, &(pmmc->rca), sizeof(pmmc->rca));
		break;
	case DEV_EMMC_SCR:
		memcpy(ppk->data, &(pmmc->scr[0]), sizeof(pmmc->scr));
		break;
	default:
		TLINFO("unkown DEV_INFO sub cmd!\n");
		break;
	}

	set_packet(ppk, 0x7, TO_HOST, CMD_RESP_DATA, data_size, NULL);
}

void prepare_resp_eraseflash(struct packet *ppk, struct loader_set *pload)
{
	struct mmc *pmmc = flashex_get_device();

	pload->start_addr = 0;
	pload->end_addr = pmmc->capacity;

	set_packet(ppk, 0xf, TO_HOST, CMD_SUCCESS, 0, NULL);
}

/* ------ main function for target loader communication and hanlder ------ */
void loader_scheduler(void)
{
	struct buffer_desc down_desc, up_desc, tmp_desc;
	struct expect_info exp_info;
	struct packet pk;
	struct loader_set ld_set;

	u32 zip_data_type, zip_data_len, zip_fill_val;
	u32 i = 0;

	u8 f_linked = FALSE;
	u8 f_need_resp = FALSE;

	u32 line = 0;

	u8 f_need_sync = FALSE;
	u64 rest_bytes = 0;
	int threshold = 0;
	int max_cnt = 0;

	int status = 0;

	u32 addr = 0;
	u16 size = 0;

	char part_string[256];
	u8  part_name_len = 0;
	u32 part_blk_st = 0;
	u32 part_blk_len = 0;
	u32 part_blk_sz = 0;

	u32 efuse_len = 0;

	/* down buf to receive cmd and data */
	down_desc.size = 0;

	/* up buf to response cmd */
	up_desc.buf= (u8 *)calloc(1, (UP_DATA_SIZE + UP_EXTRA_SIZE));
	if (NULL == up_desc.buf)
		TLERR("alloc up buf mem (0x%lx) failed\n",(UP_DATA_SIZE + UP_EXTRA_SIZE));
	up_desc.size = 0;

	/* tmp buf to sync data to disk */
	tmp_desc.buf = (u8 *)calloc(1, TMP_BUF_SIZE);
	if ( NULL == tmp_desc.buf) {
		TLERR("alloc tmp buf mem space 0x%lx\n", TMP_BUF_SIZE);
	}
	tmp_desc.size = 0;

	exp_info.flag = RESET;
	exp_info.size = DEF_PACK_LEN;

	printf("BUFFER: down 0x%p, up 0x%p, tmp 0x%p \n", down_desc.buf, up_desc.buf, tmp_desc.buf);
	TLINFO("TL protocol version:0x%x\n", TL_VERSION);
	while (1) {
		down_desc.size = exp_info.size + CMD_CMD_SIZE + CMD_CRC_SIZE;
		if (exp_info.cmd_id == CMD_DATA)
			down_desc.size += CMD_DATA_FILL_SIZE; /* 4 bytes aligned */

		tl_recv(&(down_desc.buf), down_desc.size);
		pre_handle(&down_desc, &pk, &exp_info);

		TLDEBUG("cmd_id: 0x%02x\n", pk.cmd_id);

		/* check link with host */
		if (FALSE == f_linked) {
			if (CMD_LINK_EST == pk.cmd_id) {
				f_linked = TRUE;
			} else {
				TLDEBUG("wait for host to link?\n");
				continue;
			}
		}

		f_need_resp = TRUE;

		switch (pk.cmd_id) {
		case CMD_SW_RESET:
			soft_reset();
			break;
		case CMD_HW_RESET:
			hard_reset();
			break;
		case CMD_LINK_EST:
			prepare_resp_link_est(&pk, &exp_info);
			memset(&ld_set, 0, sizeof(struct loader_set));
			break;
		case CMD_UNZIP_INFO:
			prepare_resp_unzip_info(&pk, &ld_set);
			break;
		case CMD_DEV_BASE_ADDR:
			prepare_resp_dev_base_addr(&pk);
			exp_info.flag = RESET;
			break;
		case CMD_EFUSE_WRITE_LEN:
			efuse_len = prepare_resp_efuse_write_len(&pk);
			set_expect_info(&exp_info, 0x7, SET, CMD_EFUSE_WRITE_DATA, efuse_len);
			break;
		case CMD_EFUSE_WRITE_DATA:
			prepare_resp_efuse_write(&pk);
			exp_info.flag = RESET;
			break;
		case CMD_EFUSE_READ:
			prepare_resp_efuse_read(&pk);
			exp_info.flag = RESET;
			break;
		case CMD_DEVICE_INFO:
			prepare_resp_dev_info(&pk);
			exp_info.flag = RESET;
			break;
		case CMD_ERASE_SEG_ST_ADDR:
			makeup_u32val((pk.data + 1), TRUE, &addr);
			ld_set.start_addr = addr;
			ld_set.start_addr += ld_set.addr_offset;

			set_packet(&pk, 0xf, TO_HOST, CMD_ACK, 0, NULL);
			set_expect_info(&exp_info, 0x3, SET, CMD_ERASE_SEG_END_ADDR, 0);
			break;
		case CMD_ERASE_SEG_END_ADDR:
			makeup_u32val((pk.data + 1), TRUE, &addr);
			ld_set.end_addr = addr;
			ld_set.end_addr += ld_set.addr_offset;

			set_packet(&pk, 0xf, TO_HOST, CMD_SUCCESS, 0, NULL);

			rest_bytes = ld_set.end_addr - ld_set.start_addr + 1;
			TLINFO("to erase: start(0x%llx), len(0x%llx)\n",ld_set.start_addr, rest_bytes);

			status = flashex_erase(ld_set.start_addr, rest_bytes);
			if (status)
				pk.cmd_id = CMD_FAIL;

			exp_info.flag = RESET;
			break;
		case CMD_ERASEFLASH:
			prepare_resp_eraseflash(&pk, &ld_set);

			rest_bytes = ld_set.end_addr - ld_set.start_addr + 1;
			TLINFO("to erase all flash: (0x%llx)\n", rest_bytes);

			status = flashex_erase(ld_set.start_addr, rest_bytes);
			if (status) {
				TLERR("flashex_erase fail, status=%d\n", status);
				pk.cmd_id = CMD_FAIL;
			}

			exp_info.flag = RESET;
			break;

		case CMD_PROG_DEV_ST_ADDR:
			if (FLASH_TYPE_YAFFS2 == *(pk.data)) {
				is_yaffs = NAND_OOB_YAFFS;
			} else if (FLASH_TYPE_OOB == *(pk.data)) {
				is_yaffs = NAND_OOB_RAW;
			}

			makeup_u32val((pk.data + 1), TRUE, &addr);
			ld_set.start_addr = addr;
			ld_set.start_addr += ld_set.addr_offset;

			set_packet(&pk, 0x7, TO_HOST, CMD_ACK, 0, NULL);
			set_expect_info(&exp_info, 0x3, SET, CMD_PROG_DEV_END_ADDR, 0);
			break;

		case CMD_PROG_DEV_END_ADDR:
			makeup_u32val((pk.data + 1), TRUE, &addr);
			ld_set.end_addr = addr;
			ld_set.end_addr += ld_set.addr_offset;

			set_packet(&pk, 0x7, TO_HOST, CMD_ACK, 0, NULL);

			if (ld_set.is_zip)
				rest_bytes = ld_set.allpacks_bytes;
			else
				rest_bytes = (ld_set.end_addr - ld_set.start_addr) + 1;
			TLINFO("to down img(%u): start:0x%llx len:0x%llx\n",
                      ld_set.is_zip, ld_set.start_addr, rest_bytes);
			ld_set.cur_addr = ld_set.start_addr;
			set_expect_info(&exp_info, 0x3, SET, CMD_DATA_PACKET_SIZE, 0);
			break;

		case CMD_PART_NAME_LEN:
			part_name_len = *(pk.data);
			set_expect_info(&exp_info, 0x7, SET, CMD_PART_NAME_STRING, (part_name_len + DEF_PACK_LEN));
			set_packet(&pk, 0xf, TO_HOST, CMD_ACK, 0, NULL);
			break;

		case CMD_PART_NAME_STRING:
			for(i = 0; i < part_name_len; i++)
				part_string[i] = *(pk.data + i);
			part_string[i]='\0';
			set_expect_info(&exp_info, 0x5, RESET, CMD_NONE, DEF_PACK_LEN);
			set_packet(&pk, 0xf, TO_HOST, CMD_ACK, 0, NULL);

			if(flashex_partiton_find_by_name(part_string, &part_blk_st, &part_blk_len,
											&part_blk_sz)) {
				TLINFO("found flash partition, failed!\n");
				set_packet(&pk, 0xf, TO_HOST, CMD_FAIL, 0, NULL);
				break;
			}
			TLINFO("%s 0x%x 0x%08x 0x%08x\n", part_string, part_blk_sz, part_blk_st, part_blk_len);
			ld_set.cur_addr = ld_set.start_addr = (part_blk_st * part_blk_sz);
			ld_set.end_addr = (part_blk_st + part_blk_len) * part_blk_sz;
			break;

		case CMD_PART_REQ_ST:
			*(pk.data)     = (part_blk_st & 0xFF);
			*(pk.data + 1) = (part_blk_st & 0xFF00) >> 8;
			*(pk.data + 2) = (part_blk_st & 0xFF0000) >> 16;
			*(pk.data + 3) = (part_blk_st & 0xFF000000) >> 24;
			set_packet(&pk, 0x7, TO_HOST, CMD_RESP_DATA, 4, NULL);

			set_expect_info(&exp_info, 0x7, SET, CMD_PART_REQ_SZ, DEF_PACK_LEN);
			break;

		case CMD_PART_REQ_SZ:
			*(pk.data)     = (part_blk_len & 0xFF);
			*(pk.data + 1) = (part_blk_len & 0xFF00) >> 8;
			*(pk.data + 2) = (part_blk_len & 0xFF0000) >> 16;
			*(pk.data + 3) = (part_blk_len & 0xFF000000) >> 24;
			set_packet(&pk, 0x7, TO_HOST, CMD_RESP_DATA, 4, NULL);

			set_expect_info(&exp_info, 0x5, RESET, CMD_NONE, DEF_PACK_LEN);
			break;

		case CMD_DATA_PACKET_SIZE:
			size =  *(pk.data + 1);
			size |= *(pk.data) << 8;
			set_expect_info(&exp_info, 0x7, SET, CMD_DATA, size);
			set_packet(&pk, 0xf, TO_HOST, CMD_ACK, 0, NULL);
			ld_set.once_data_bytes = size;

			/* ensure gap at the end of tmp buf */
			threshold = 0;
			max_cnt = TMP_BUF_SIZE / size;
			TLINFO("once load (0x%x) data; max_cnt(%d) to sync\n", size, max_cnt);
			break;

		case CMD_DATA:
			if ( 0x0 == ld_set.is_zip) {
				memcpy((tmp_desc.buf + tmp_desc.size), pk.data, pk.data_size);
				threshold++;
				tmp_desc.size += pk.data_size;

				//??? copy_data func.  no response why to set packet
				//set_packet(&pk, 0x7, TO_HOST, CMD_SUCCESS, 0, NULL);
			} else if( 0x2 == ld_set.is_zip) {
				zip_data_type = 0;
				zip_data_len = 0;
				zip_fill_val =0;

				makeup_u32val(pk.data, FALSE, &zip_data_type);
				makeup_u32val((pk.data + 4), FALSE, &zip_data_len);
				switch (zip_data_type) {
				case 1: // raw data
					memcpy(((u8 *)tmp_desc.buf + tmp_desc.size), (pk.data + 8), zip_data_len);
					tmp_desc.size += zip_data_len;
					threshold++;
					if (zip_data_len % DEF_BLK_SIZE) {
						TLERR("raw data len(%x) can't div DEF_BLK_SIZE\n",zip_data_len);
						f_need_sync = TRUE;
					}
					break;
				case 2: // fill a region with a value
					makeup_u32val((pk.data + 8), FALSE, &zip_fill_val);
					TLINFO("zip_fill_val(0x%x)", zip_fill_val);
				/*special zip data type, fall through*/
				case 3: // skip a region and don't care its data
					f_need_sync = TRUE;
					break;
				default:
					TLINFO("unkown zip_data_type(%d)!\n",zip_data_type);
				}
			} else {
				TLINFO("zip_type(0x%x) don't support now!\n", ld_set.is_zip);
			}

			rest_bytes = rest_bytes - exp_info.size;
			if (rest_bytes <= exp_info.size) {
				exp_info.size = rest_bytes;

				if (0 == rest_bytes) {
					set_expect_info(&exp_info, 0x7, SET, CMD_DATA_SYNC, DEF_PACK_LEN);
				}
			}

			/*serial TL need force resp for CMD_DATA packet*/
			f_need_resp = tl_force_resp;
			if (f_need_resp)
				set_packet(&pk, 0xf, TO_HOST, CMD_ACK, 0, NULL);
			break;

		case CMD_DATA_SYNC:
			status = flashex_write(ld_set.cur_addr, tmp_desc.buf, tmp_desc.size);
			if (status) {
				TLERR("write data err!\n");
			}

			TLINFO("CMD_DATA_SYNC: size:0x%x rest_bytes 0x%llx\n",tmp_desc.size, rest_bytes);
			tmp_desc.size = 0;
			threshold = 0;

			if (status) {
				*(u32 *)(pk.data) = status;
				set_packet(&pk, 0x7, TO_HOST, CMD_FAIL, 4, NULL);
			} else {
				set_packet(&pk, 0xf, TO_HOST, CMD_SUCCESS, 0, NULL);
			}
			set_expect_info(&exp_info, 0x7, RESET, CMD_LINK_EST, DEF_PACK_LEN);
			break;

		case CMD_RAM_CHECK:
		case CMD_FLASH_CHECK:
			set_packet(&pk, 0xf, TO_HOST, CMD_SUCCESS, 0, NULL);
			set_expect_info(&exp_info, 0x5, RESET, CMD_NONE, DEF_PACK_LEN);
			break;

		case CMD_RVRS_DWNLD:
			makeup_u32val(pk.data, TRUE, &addr);
			ld_set.start_addr = addr;
			ld_set.cur_addr = addr;
			set_packet(&pk, 0xf, TO_HOST, CMD_ACK, 0, NULL );

			ld_set.mode = 1;
			ld_set.once_data_bytes = UP_DATA_SIZE;
			up_desc.size = UP_DATA_SIZE;
			set_expect_info(&exp_info, 0x3, SET, CMD_UPLD_DATA_SIZE, 0);
			break;

		case CMD_UPLD_DATA_SIZE:
			makeup_u32val(pk.data, TRUE, (u32 *)(&rest_bytes));
			ld_set.allpacks_bytes = rest_bytes;

			TLINFO("upload start:0x%llx, size:0x%llx, is_yaffs:0x%lx\n",
					ld_set.start_addr, rest_bytes, is_upload_yaffs);
			if (rest_bytes < UP_DATA_SIZE)
				up_desc.size = rest_bytes;

			status = flashex_read(ld_set.cur_addr, (up_desc.buf + UP_EXTRA_SIZE), up_desc.size);
			if (status) {
				TLERR("read data err!\n");
				pk.cmd_id = CMD_FAIL;
			}

			ld_set.cur_addr += up_desc.size;
			rest_bytes -= up_desc.size;
			set_packet(&pk, 0x7, TO_HOST, CMD_DATA, up_desc.size, NULL);
			exp_info.flag = RESET;
			break;

		case CMD_SUCCESS:
			if (rest_bytes) {
				if (rest_bytes < UP_DATA_SIZE)
					up_desc.size = rest_bytes;

				status = flashex_read(ld_set.cur_addr, (up_desc.buf + UP_EXTRA_SIZE), up_desc.size);
				if (status) {
					TLERR("read data err!\n");
					pk.cmd_id = CMD_FAIL;
				}

				ld_set.cur_addr += up_desc.size;
				rest_bytes -= up_desc.size;

				set_packet(&pk, 0x7, TO_HOST, CMD_DATA, up_desc.size, NULL);
			} else {
				if (is_upload_yaffs)
					is_upload_yaffs = NAND_OOB_NONE;
				ld_set.mode = 0;
				f_need_resp = FALSE;
			}
			break;

		case CMD_DEV_PROTECT:
		case CMD_DEV_UNPROTECT:
		case CMD_PARTITION_DATA:
			set_packet(&pk, 0xf, TO_HOST, CMD_SUCCESS, 0, NULL);
			set_expect_info(&exp_info, 0x5, RESET, CMD_NONE, DEF_PACK_LEN);
			break;

		case CMD_CHANGE_CONFIG:
		case CMD_START_EXECUTION:
		case CMD_LOCK_SEG_ST_ADDR:
		case CMD_LOCK_SEG_END_ADDR:
		case CMD_UNLOCK_SEG_ST_ADDR:
		case CMD_UNLOCK_SEG_END_ADDR:
		case CMD_TL_DOWNLOAD:
			break;

		default:
			(pk.data)[0] = pk.cmd_id;
			(pk.data)[1] = 12;
			set_packet(&pk, 0x7, TO_HOST, CMD_FAIL, 2, NULL);
			break;
		}//end for switch (pk.cmd_id)

		if (f_need_sync) {
			flashex_write(ld_set.cur_addr, tmp_desc.buf, tmp_desc.size);
			ld_set.cur_addr += tmp_desc.size;

			if (2 == zip_data_type) {
				// First, use tmp buf to fill with the value.
				for (i = 0; i < TMP_BUF_SIZE; i = i + 4) {
					*(u32 *)(tmp_desc.buf + i) = zip_fill_val;
				}

				// Second, use tmp buf to sync disk.
				while(zip_data_len){
					i = (zip_data_len * 4) > TMP_BUF_SIZE ? TMP_BUF_SIZE : (zip_data_len * 4);
					flashex_write(ld_set.cur_addr, tmp_desc.buf, i);
					ld_set.cur_addr += i;
					zip_data_len = zip_data_len - (i / 4);
				}
			} else if (3 == zip_data_type) {
				ld_set.cur_addr += zip_data_len;// skip and just change cur_addr
			}

			if (zip_data_len % DEF_BLK_SIZE)
				TLERR("fill/skip(0x%x) data len(%x) can't div DEF_BLK_SIZE\n",
                                   zip_data_type,zip_data_len);

			TLINFO("force to sync: zip_data_type (0x%x): zip_data_len 0x%x\n",
                                   zip_data_type, zip_data_len);

			tmp_desc.size = 0;
			threshold = 0;
			f_need_sync = FALSE;
		}

		if (max_cnt && (max_cnt == threshold)) {
			flashex_write(ld_set.cur_addr, tmp_desc.buf, tmp_desc.size);
			ld_set.cur_addr += tmp_desc.size;
			TLINFO("tmp buf full to sync: ld_set.cur_addr 0x%llx\n",ld_set.cur_addr);
			tmp_desc.size = 0;
			threshold = 0;
			f_need_sync = FALSE;
		}

		if(f_need_resp) {
			protocol_handle(&pk, &up_desc);
			tl_send(pk.data, pk.data_size);
		}
	} // end for while(1)
}

int targetloader_init(void)
{
#ifdef CONFIG_TL_USB
	usb_udl_init();
	tl_recv = usb_udl_recv;
	tl_send = usb_udl_send;
	tl_force_resp = FALSE;
#endif

#ifdef CONFIG_TL_SERIAL
	tl_recv = uart_sdl_recv;
	tl_send = uart_sdl_send;
	tl_force_resp = TRUE;
#endif

	printf("%s\n",__func__);
	loader_scheduler();

	return 0;
}

void targetloader_config(void)
{
	bd_t *bd;
	gd_t *id;
	ulong mem_addr, addr;

	addr = CONFIG_TL_RAM_START + CONFIG_TL_RAM_SIZE;

	/*
	 * (permanently) allocate a Board Info struct
	 * and a permanent copy of the "global" data
	 */
	addr -= sizeof (bd_t);
	bd = (bd_t *) addr;
	gd->bd = bd;
	debug("Reserving %zu Bytes for Board Info at: %08lx\n",
			sizeof (bd_t), addr);
	addr &= ~(0x03);

#ifdef CONFIG_MACH_TYPE
	gd->bd->bi_arch_number = CONFIG_MACH_TYPE; /* board id for Linux */
#endif

	addr -= sizeof (gd_t);
	addr -= 12;
	id = (gd_t *) addr;
	debug("Reserving %zu Bytes for Global Data at: %08lx\n",
			sizeof (gd_t), addr);

	addr &= ~(1024 - 1);
	/* setup stackpointer for exeptions in internel RAM */
	gd->irq_sp = addr;
	addr -= (CONFIG_STACKSIZE_IRQ + CONFIG_STACKSIZE_FIQ);
	addr -= 12;
	addr &= ~0x07;
	debug("IRQ Stack Pointer is: %08lx\n", gd->irq_sp);

	gd->bd->bi_baudrate = gd->baudrate;

#ifdef CONFIG_TL_NODDR
	mem_addr = addr;
#else
	mem_addr = CONFIG_SYS_SDRAM_BASE + gd->ram_size;
	dram_init_banksize();
	//display_dram_config();	/* and display it */
#endif

	gd->relocaddr = CONFIG_TL_RAM_START;
	gd->start_addr_sp = mem_addr;
	gd->reloc_off = gd->relocaddr  - _TEXT_BASE;
	debug("relocation Offset is: %08lx\n", gd->reloc_off);
	memcpy(id, (void *)gd, sizeof(gd_t));

	debug("gd->start_addr_sp: %08lx\n",gd->start_addr_sp);
	relocate_code(addr, id, CONFIG_TL_RAM_START);
}
