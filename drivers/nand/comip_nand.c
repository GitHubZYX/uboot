/* 
 **
 ** Use of source code is subject to the terms of the LeadCore license agreement under
 ** which you licensed source code. If you did not accept the terms of the license agreement,
 ** you are not authorized to use source code. For the terms of the license, please see the
 ** license agreement between you and LeadCore.
 **
 ** Copyright (c) 2013-2020  LeadCoreTech Corp.
 **
 **  PURPOSE:		nand main function, based on low level nand driver, implemented  bad block management
 **
 **  CHANGE HISTORY:
 **
 **  Version		Date		Author		Description
 **  1.0.0		2013-06-05	zouqiao		created
 **
 */
#include <common.h>
#include "nand_dev.h"

//#define NAND_DEBUG
#ifdef NAND_DEBUG
#define NAND_PRINT(fmt, args...) 	printf(fmt, ##args)
#else
#define NAND_PRINT(fmt, args...)
#endif

#define NAND_CHIP_NUM			(1)

#define NAND_ECC_ERR			(-1000)

#define ECC_POSITION_MASK		(0x0700)

#define NAND_DMAS_TX_CHANNEL		(0)
#define NAND_DMAS_RX_CHANNEL		(8)

/* Standard NAND flash commands. */
#define NAND_CMD_READID			0x90

/* COMIP NAND flash control codes. */
#define NOP_CODE			0x8000
#define CMD_CODE			0x1000
#define ADD_CODE			0x2000
#define READ_CODE			0x5000
#define WRITE_CODE			0x6000
#define STAT_CODE			0x7000
#define RST_CODE			0xF000

#define VALID_HEADER			0x524E

#define NAND_MAX_BBT_SLOTS			(24)
#define NAND_MAX_RELOC_ENTRY			(127)
#define NAND_RELOC_ENTRY(maxblocks)		(((maxblocks) * 2) / 100)

#define NAND_WRITE_LEVEL_PAGES			(10)

#define NAND_BUFFER_SIZE_MAX			(256 * 1024)

enum {
	BBT_UNCHANGED = 0,
	BBT_CHANGED = 1
};

struct reloc_item {
	unsigned short from;
	unsigned short to;
};

struct reloc_table {
	unsigned short header;
	unsigned short total;
	struct reloc_item item[];
};

struct nand_bbt {
	struct reloc_table *reloc;	// Pointer to local relo table
	uchar bbt_state;	// Whether BBT changed
	ulong bbt_slot;		// Indicates which slot/page in Block next BBT will be written to
	ulong bbt_full;		// Indicates whether 24 pages written and we need to erease block0
};

enum  {
	NAND_PRE_READ,
	NAND_PRE_WRITE,
	NAND_PRE_ERASE,
	NAND_PRE_RESET
};

#define YAFFS2_SPARE_SIZE		(28)

#define NAND_BOOTLOADER_SIZE_MIN	(8 * 1024)
#define NAND_BOOTLOADER_SIZE_MAX	(110 * 1024)

#define NAND_4BIT_ECC_SPARE_START	(2)
#define NAND_4BIT_ECC_SPARE_SIZE	(7)

#define NAND_8BIT_ECC_SPARE_START	(2)
#define NAND_8BIT_ECC_SPARE_SIZE	(13)

static struct nand_ecclayout comip_1bit_ecc_lb_nand_oob = {
	/* eccbytes */
	12,

	/* eccpos */
	{8, 9, 10, 24, 25, 26, 40, 41, 42, 56, 57, 58},

	/* oobfree */
	{{11, 13}, {27, 13}, {43, 13}, {59, 5}, {2, 6}}
};

static struct nand_ecclayout comip_1bit_ecc_sb_nand_oob = {
	/* eccbytes */
	6,

	/* eccpos */
	{8, 9, 10, 11, 12, 13},

	/* oobfree */
	{{2, 6}}
};

static struct nand_ecclayout* ecc_layout = NULL;
static struct nand_chip *chip_table[NAND_CHIP_NUM];
static struct nand_bbt bbt_table[NAND_CHIP_NUM];
static ulong gchip;
static ulong gmax_reloc_entry = NAND_MAX_RELOC_ENTRY;
static ulong gsectorsize;
static ulong gsectornum;
static ulong gsectorspare;
static ulong geccbitstart;
static ulong geccbitsize;
static ulong *greloc;
static ulong *greloc2;
static ulong *grelocbuf;
static ulong *gdbuf;
static ulong *gsbuf;
static ulong *gsbuf2;

#define WRITE_CMD(code, flag, cmd) \
		(REG16(NFC_TX_BUF) = ((code) | ((flag) << 8) | (cmd)))
#define WRITE_DATA(data) \
		(REG16(NFC_TX_BUF) = data)

static __inline int nand_rx(ushort* data)
{
	while(!(REG32(NFC_INTR_RAW_STAT) & NFC_INT_RNE));
	*data = REG16(NFC_RX_BUF);
	return 0;
}

static __inline int nand_tx(ushort data)
{
	while(!(REG32(NFC_INTR_RAW_STAT) & NFC_INT_TNF));
	REG16(NFC_TX_BUF) = data;
	return 0;
}

static __inline int comip_nand_receive(uchar width, uchar *buf, ulong len)
{
#if defined(CONFIG_COMIP_NAND_NODMA)
	ulong cnt;
	ushort data;

	if (width == NAND_IO_8BIT) {
		for(cnt = 0; cnt < len; cnt++) {
			nand_rx(&data);
			*buf++ = (uchar)data;
		}
	} else {
		for (cnt = 0; cnt < len / 2; cnt++) {
			nand_rx((ushort*)buf);
			buf += 2;
		}
	}
#else /* DMA. */
	REG32(DMAS_CH_SAR(NAND_DMAS_RX_CHANNEL)) = NFC_RX_BUF;
	REG32(DMAS_CH_DAR(NAND_DMAS_RX_CHANNEL)) = (ulong)buf;
	REG32(DMAS_CH_CTL0(NAND_DMAS_RX_CHANNEL)) = len;
	REG32(DMAS_CH_CTL1(NAND_DMAS_RX_CHANNEL)) = 0x201;

	REG32(AP_DMAS_BASE + DMAS_EN) = NAND_DMAS_RX_CHANNEL;
	while(!(REG32(AP_DMAS_BASE + DMAS_INT_RAW) & (1 << NAND_DMAS_RX_CHANNEL)));
	REG32(AP_DMAS_BASE + DMAS_INT_CLR) = NAND_DMAS_RX_CHANNEL;
#endif

	return 0;
}

static __inline int comip_nand_send(uchar width, uchar *buf, ulong len)
{
#if defined(CONFIG_COMIP_NAND_NODMA)
	ulong cnt = 0;

	for (cnt = 0; cnt < len / 2; cnt++) {
		nand_tx(*(ushort*)buf);
		buf += 2;
	}
#else /* DMA. */
	REG32(DMAS_CH_SAR(NAND_DMAS_TX_CHANNEL)) = (ulong)buf;
	REG32(DMAS_CH_DAR(NAND_DMAS_TX_CHANNEL)) = NFC_TX_BUF;
	REG32(DMAS_CH_CTL0(NAND_DMAS_TX_CHANNEL)) = len;
	REG32(DMAS_CH_CTL1(NAND_DMAS_TX_CHANNEL)) = 0x201;

	REG32(AP_DMAS_BASE + DMAS_EN) = NAND_DMAS_TX_CHANNEL;
	while(!(REG32(AP_DMAS_BASE + DMAS_INT_RAW) & (1 << NAND_DMAS_TX_CHANNEL)));
	REG32(AP_DMAS_BASE + DMAS_INT_CLR) = NAND_DMAS_TX_CHANNEL;

#endif

	return 0;
}

static void comip_nand_flush_fifo(int is_rx)
{
	unsigned int reg;

	reg = REG32(NFC_CTRLR0);
	if (is_rx)
		REG32(NFC_CTRLR0) = (reg | 0x0000004);
	else
		REG32(NFC_CTRLR0) = (reg | 0x0000008);
	REG32(NFC_CTRLR0) = reg;
}

static void comip_nfc_set_ecc_type(int type)
{
	unsigned int reg;
	unsigned int val;

	if (type == NAND_ECC_8BIT)
		val = 1;
	else if (type == NAND_ECC_4BIT)
		val = 2;
	else
		val = 0;

	reg = REG32(NFC_CTRLR0);
	reg &= ~(3 << 8);
	reg |= val << 8;
	REG32(NFC_CTRLR0) = reg;
}

static void comip_nfc_enable_auto_ecc(int enable, unsigned int address)
{
	unsigned int reg;

	reg = REG32(NFC_CTRLR0);
	if (!enable)
		reg &= ~NFC_CTRLR0_ECC_FIX_ERROR_EN;
	else
		reg |= NFC_CTRLR0_ECC_FIX_ERROR_EN;
	REG32(NFC_CTRLR0) = reg;

	REG32(NFC_AUTO_EC_BASE_ADDR) = address;
}

static unsigned int comip_nand_ns2clkcnt(int ns)
{
	unsigned int clkcnt = (CONFIG_DDR_BUS_CLK / 100000) * ns;

	clkcnt /= 10000;

	return (clkcnt + 1);
}

static unsigned int comip_nand_check_timing(unsigned int *range, int ns)
{
	unsigned int clkcnt = comip_nand_ns2clkcnt(ns);
	unsigned int index;

	for (index = 0; index < 4; index++)
		if (clkcnt <= range[index])
			break;

	if (index >= 4)
		index = 3;

	return index;
}

void comip_nand_set_timing(struct nand_chip *chip)
{
	unsigned int range[4][4] = {{0, 1, 2, 3}, {1, 3, 4, 5},
				{1, 2, 3, 4}, {3, 4, 5, 6}};
	unsigned int value;
	unsigned int temp;

	/* configure the timming */
	value = 0;
	temp = comip_nand_check_timing(&range[3][0], chip->timing.tWP);
	value |= (temp << 0);
	temp = comip_nand_check_timing(&range[2][0], chip->timing.tWC - chip->timing.tWP);
	value |= (temp << 2);
	temp = comip_nand_check_timing(&range[1][0], chip->timing.tCLS - chip->timing.tWP);
	value |= (temp << 4);
	temp = comip_nand_check_timing(&range[0][0], chip->timing.tCLH);
	value |= (temp << 6);
	REG32(NFC_TIMR0) = value;

	value = 0;
	temp = comip_nand_check_timing(&range[3][0], chip->timing.tRP);
	value |= (temp << 0);
	temp = comip_nand_check_timing(&range[2][0], chip->timing.tRC - chip->timing.tRP);
	value |= (temp << 2);
	temp = comip_nand_check_timing(&range[1][0], chip->timing.tCS - chip->timing.tRP);
	value |= (temp << 4);
	temp = 0x3;
	value |= (temp << 6);
	REG32(NFC_TIMR1) = value;
}

static int comip_ecc_gen(uchar *dbuf, uchar *sbuf, uchar width, uchar eccbit)
{
	ulong cnt;
	ulong pos;
	ulong sector;
	ulong hwecc;

	if (eccbit != NAND_ECC_1BIT) {
		while(!(REG32(NFC_INTR_RAW_STAT) & NFC_INT_BCH_ENC_DONE));
		REG32(NFC_INTR_STAT) = NFC_INT_BCH_ENC_DONE;
		for (pos = 0; pos < geccbitsize; pos++)
			sbuf[geccbitstart + pos] = REG8(NFC_BCH_RAM + pos);
	} else {
		for (sector = 0; sector < 4; sector++) {
			/* Wait for hardware ecc ready. */
			while (!(REG32(NFC_INTR_RAW_STAT) & (1 << (13 + sector))));

			hwecc = REG32(NFC_ECC_CODE0 + 4 * sector);

			/* Clear the ready flag. */
			REG32(NFC_INTR_STAT) = 0x1 << (13 + sector);

			for (pos = 0, cnt = 0; (pos < gsectorspare) && (cnt < 3); pos++) {
				if (ECC_POSITION_MASK & (0x1 << pos)) {
					sbuf[pos] = (uchar)hwecc;
					hwecc = hwecc >> 8;
					cnt++;
				}
			}

			dbuf += gsectorsize;
			sbuf += gsectorspare;
		}
	}
	return 0;
}

static int comip_ecc_arithmetic(uchar *ecc1, uchar *ecc2, uchar *buf,
				   uchar bw)
{
	ulong result;
	ulong err_position;
	ulong err_bit;
	ulong err_byte;
	uchar *errAddr;
	uchar cnt;
	uchar tmp;

	if (!memcmp(ecc1, ecc2, 3))
		return 0;

	cnt = 0;
	tmp = 0;
	result = 0;

	result |= (ecc1[0] ^ ecc2[0]);
	result |= ((ecc1[1] ^ ecc2[1]) << 8);
	result |= ((ecc1[2] ^ ecc2[2]) << 16);

	for (cnt = 0; cnt < 24; cnt++)
		if (result & (0x1 << cnt))
			tmp++;

	switch (tmp) {
	case 0:
		return 0;

	case 1:
		return 0;

	case 12:
		err_position = ((result & 0x02) >> 1) | ((result & 0x08) >> 2)
					| ((result & 0x20) >> 3) | ((result & 0x80) >> 4);
		err_position = err_position | ((result & 0x200) >> 5) | ((result & 0x800) >> 6)
					| ((result & 0x2000) >> 7) | ((result & 0x8000) >> 8);
		err_position = err_position | ((result & 0x20000) >> 9) | ((result & 0x80000) >> 10)
					| ((result & 0x200000) >> 11) | ((result & 0x800000) >> 12);
		err_bit = (err_position >> 9) & 0x7;
		err_byte = err_position & 0x1ff;
		errAddr = (uchar *)(buf + err_byte);

		/* If err_bit is 1, then clr this bit; If err_bit is 0, then set this bit. */
		tmp = *errAddr;
		if((tmp >> err_bit) & 0x01)
			tmp = tmp & (0xff & (~(1 << err_bit)));
		else
			tmp = tmp | (1 << err_bit);
		*errAddr = tmp;
		return 0;

	default:
		return -1;
	}
}

static int comip_ecc_verify(uchar *dbuf, uchar *sbuf, uchar width, uchar eccbit)
{
	ulong cnt;
	ulong pos;
	ulong sector;
	ulong hwecc;
	uchar swecc[3];	/* Calc for caculate, store is in sbuf. */
	int status = 0;

	if (eccbit != NAND_ECC_1BIT) {
		while(!(REG32(NFC_INTR_RAW_STAT) & NFC_INT_BCH_DEC_DONE));
		REG32(NFC_INTR_STAT) = NFC_INT_BCH_DEC_DONE;

		if (REG32(NFC_BCH_ERR_MODE) & 0xff00)
			return NAND_ECC_ERR;
	} else {
		/* Get the store-code from sbuf. */
		for (sector = 0; sector < 4; sector++) {
			for (pos = 0, cnt = 0; (pos < gsectorspare) && (cnt < 3); pos++) {
				if (ECC_POSITION_MASK & (0x1 << pos))
					swecc[cnt++] = sbuf[pos];
			}

			/* Wait for hardware ecc ready. */
			while (!(REG32(NFC_INTR_RAW_STAT) & (1 << (13 + sector))));

			hwecc = REG32(NFC_ECC_CODE0 + 4 * sector);

			/* Clear the ready flag. */
			REG32(NFC_INTR_STAT) = 0x1 << (13 + sector);

			status = comip_ecc_arithmetic((uchar *)&hwecc, swecc, dbuf, width);
			if (status != 0)
				return NAND_ECC_ERR;

			dbuf += gsectorsize;
			sbuf += gsectorspare;
		}
	}

	return 0;
}

static int comip_nand_status(ulong chip_num, uchar* status)
{
	static uchar cnt = 1;
	ushort seq_status;
	uchar seq_num;

	chip_num += 1;

	WRITE_CMD(STAT_CODE, chip_num, cnt);

	WRITE_CMD(NOP_CODE, 0x0, 0x8);

	do {
		seq_status = REG16(NFC_STA_SEQ);
		seq_num = (uchar)((seq_status >> 8) & 0x7f);
		if (seq_num != cnt)
			printf("Invalid seq number. Wanted %d, actual %d. Try again!\n",
						cnt, seq_num);
	} while (seq_num != cnt);

	*status = (uchar)seq_status;

	cnt++;
	if (cnt > 16)
		cnt = 1;

	comip_nand_flush_fifo(1);

	return 0;
}

static int comip_nand_wait_ready(ulong chip_num, ulong timeout)
{
	/* Check ready status. */
	do {
		udelay(5);
	} while (timeout-- && !(REG32(NFC_RB_STATUS) & NFC_RB_STATUS_RDY));

	if (!(REG32(NFC_RB_STATUS) & NFC_RB_STATUS_RDY))
		return -1;

	return 0;
}

static int comip_nand_reset(ulong chip_num)
{
	chip_num += 1;
	WRITE_CMD(RST_CODE, chip_num, 0x00);
	WRITE_CMD(NOP_CODE, 0x0, 0x3);

	return 0;
}

static int comip_nand_init(ulong flags)
{
	struct nand_chip *chip;
	uchar page_size;
	uchar parity;
	uchar tmp;
	ulong i;

	if (flags == 0) {
		REG32(NFC_SEC_EN) = 0x00;
		/* nand_ale, nand_cle and nand_rb muxpin. */
		REG32(MUX_PIN_MUX4) = ((1 << 22) | (1 << 24) | (1 << 30));
		/* nand_cs0 and nand_cs1 muxpin. */
		REG32(MUX_PIN_MUX5) = ((1 << 0) | (1 << 2));

		/* Initial the environment using default timming. */
		REG32(NFC_CTRLR0) = 0x00;
		udelay(10);
		REG32(NFC_CTRLR0) = 0xfc;
		udelay(10);
		REG32(NFC_CTRLR0) = 0xf0;
		udelay(10);

		for (i = 0; i < NAND_CHIP_NUM; i++)
			comip_nand_reset(i);

		udelay(1000);
	} else {
		/* Configure the nand controller according to the nand chip information. */
		for (i = 0; i < NAND_CHIP_NUM; i++)
			if (chip_table[i] != NULL)
				break;

		if (i == NAND_CHIP_NUM)
			return -1;

		REG32(NFC_CS) = i + 1;

		chip = chip_table[i];
		if (chip->data_width == NAND_IO_8BIT)
			REG32(NFC_CTRLR0) &= ~NFC_CTRLR0_NFC_WIDTH;

		if (chip->ecc_bit == NAND_ECC_1BIT) {
			/* Disable auto ECC. */
			REG32(NFC_CTRLR0) &= ~NFC_CTRLR0_ECC_FIX_ERROR_EN;
		} else {
			/* Enable auto ECC. */
			if (chip->ecc_bit == NAND_ECC_8BIT)
				REG32(NFC_CTRLR0) |= (1 << 8) | NFC_CTRLR0_ECC_FIX_ERROR_EN;
			else
				REG32(NFC_CTRLR0) |= (2 << 8) | NFC_CTRLR0_ECC_FIX_ERROR_EN;

			tmp = (chip_table[i]->page_size / gsectorsize);
			parity = ((1 << tmp) - 1);
			page_size = 0;
			while (tmp /= 2)
				page_size++;

			REG32(NFC_BCH_RST) = 1;
			REG32(NFC_BCH_CFG) = ((page_size << 8) | parity);
		}

		comip_nand_set_timing(chip);
	}

	return 0;
}

static int comip_nand_read_id(ulong chip_num, uchar *info)
{
	ulong i;

	chip_num += 1;

	WRITE_CMD(CMD_CODE, chip_num, NAND_CMD_READID);

	WRITE_CMD(ADD_CODE, chip_num, 0x00);
	WRITE_DATA(0x00);

	WRITE_CMD(READ_CODE, chip_num, 0x0);
	WRITE_DATA(3);

	for (i = 0; i < 4; i++) {
		while (!(REG32(NFC_INTR_RAW_STAT) & NFC_INT_RNE));
		info[i] = REG16(NFC_RX_BUF);
	}

	return 0;
}

static int comip_nand_get_status(ulong chip_num, uchar *status2, ulong timeout)
{
	int status;

	status = comip_nand_wait_ready(chip_num, timeout);
	if (status)
		return status;

	return comip_nand_status(chip_num, status2);
}

static int comip_nand_erase(ulong chip_num, ulong block)
{
	struct nand_chip *chip = chip_table[chip_num];
	ulong temp;

	chip_num += 1;

	WRITE_CMD(CMD_CODE, chip_num, chip->command.CMD_ERASE1);

	WRITE_CMD(ADD_CODE, chip_num, chip->row_addr - 1);
	if (chip->row_addr > 2) {
		temp = chip->max_pages * block;
		WRITE_DATA(temp);
		WRITE_DATA(temp >> 16);
	} else
		WRITE_DATA(chip->max_pages * block);

	WRITE_CMD(CMD_CODE, chip_num, chip->command.CMD_ERASE2);

	WRITE_CMD(NOP_CODE, 0x0, 0x3);

	return 0;
}

static int comip_nand_read_page(ulong chip_num, ulong page,
				uchar *dbuf, uchar *sbuf, ulong flags)
{
	struct nand_chip *chip = chip_table[chip_num];
	uchar width = chip->data_width;
	uchar *sptr;
	int status;

	comip_nand_flush_fifo(1);

	chip_num += 1;

	/* Read the main data. */
	if (dbuf != NULL) {
		if (chip->ecc_bit != NAND_ECC_1BIT) {
			comip_nfc_enable_auto_ecc(1, (ulong)dbuf);
			comip_nfc_set_ecc_type(chip->ecc_bit);
			REG32(NFC_PARITY_EN) = 0x01;

			WRITE_CMD(CMD_CODE, chip_num, chip->command.CMD_READ1);

			WRITE_CMD(ADD_CODE, chip_num, chip->col_addr + chip->row_addr - 1);
			WRITE_DATA((chip->page_size + geccbitstart) >> width);
			if (chip->row_addr > 2) {
				WRITE_DATA(page);
				WRITE_DATA(page >> 16);
			} else
				WRITE_DATA(page);

			WRITE_CMD(CMD_CODE, chip_num, chip->command.CMD_READ2);

			WRITE_CMD(NOP_CODE, 0x0, 0x7);
			WRITE_CMD(READ_CODE, chip_num, 0x2);
			WRITE_DATA(geccbitsize - 1);

			WRITE_CMD(NOP_CODE, 0x1, 0x00);
			comip_nand_receive(width, ((uchar*)gsbuf2) + geccbitstart, geccbitsize);

			REG32(NFC_PARITY_EN) = 0x00;
		}

		WRITE_CMD(CMD_CODE, chip_num, chip->command.CMD_READ1);

		WRITE_CMD(ADD_CODE, chip_num, chip->col_addr + chip->row_addr - 1);
		WRITE_DATA(0);
		if (chip->row_addr > 2) {
			WRITE_DATA(page);
			WRITE_DATA(page >> 16);
		} else
			WRITE_DATA(page);

		WRITE_CMD(CMD_CODE, chip_num, chip->command.CMD_READ2);

		WRITE_CMD(NOP_CODE, 0x0, 0x7);

		if (chip->ecc_bit == NAND_ECC_1BIT)
			WRITE_CMD(READ_CODE, chip_num, 1);
		else
			WRITE_CMD(READ_CODE, chip_num, 0);
		WRITE_DATA(gsectornum * gsectorsize - 1);

		WRITE_CMD(NOP_CODE, 0x1, 0x0);
		comip_nand_receive(width, dbuf, gsectornum * gsectorsize);

		/* Do ecc-verify. */
		if (chip->ecc_bit != NAND_ECC_1BIT) {
			status = comip_ecc_verify(dbuf, NULL, width, chip->ecc_bit);
			if (status != 0) {
				NAND_PRINT("nand ecc verify failed\n");
				return status;
			}
			comip_nfc_enable_auto_ecc(0, 0xffffffff);
			comip_nfc_set_ecc_type(NAND_ECC_1BIT);
		}

		/* Demand for ECC-verify. */
		if (sbuf == NULL)
			sptr = (uchar*)gsbuf2;
		else
			sptr = sbuf;

		/* Using random-output way to read spare data. */
		if (chip->command.CMD_RANDOM_OUT1 != 0) {
			WRITE_CMD(CMD_CODE, chip_num, chip->command.CMD_RANDOM_OUT1);

			WRITE_CMD(ADD_CODE, chip_num, chip->col_addr - 1);
			WRITE_DATA(chip->page_size >> width);

			WRITE_CMD(CMD_CODE, chip_num, chip->command.CMD_RANDOM_OUT2);

			WRITE_CMD(READ_CODE, chip_num, 0x0);
			WRITE_DATA(gsectorspare * gsectornum - 1);

			WRITE_CMD(NOP_CODE, 0x1, 0x00);
			comip_nand_receive(width, sptr, gsectornum * gsectorspare);
		}

		/* Do ecc-verify. */
		if (chip->ecc_bit == NAND_ECC_1BIT) {
			status = comip_ecc_verify(dbuf, sptr, width, chip->ecc_bit);
			if (status != 0) {
				NAND_PRINT("nand ecc verify failed\n");
				return status;
			}
		}
	} else if (sbuf != NULL) {
		WRITE_CMD(CMD_CODE, chip_num, chip->command.CMD_READ1);

		WRITE_CMD(ADD_CODE, chip_num, chip->col_addr + chip->row_addr - 1);
		WRITE_DATA(chip->page_size >> width);
		if (chip->row_addr > 2) {
			WRITE_DATA(page);
			WRITE_DATA(page >> 16);
		} else
			WRITE_DATA(page);

		WRITE_CMD(CMD_CODE, chip_num, chip->command.CMD_READ2);

		WRITE_CMD(NOP_CODE, 0x0, 0x7);

		WRITE_CMD(READ_CODE, chip_num, 0);
		WRITE_DATA(gsectornum * gsectorspare - 1);

		WRITE_CMD(NOP_CODE, 0x1, 0x0);
		comip_nand_receive(width, sbuf, gsectornum * gsectorspare);
	}

	return 0;
}

static int comip_nand_read_random(ulong chip_num, ulong page,
				ulong offset, ulong size, uchar *buf)
{
	struct nand_chip *chip = chip_table[chip_num];
	uchar width = chip->data_width;

	comip_nand_flush_fifo(1);

	chip_num += 1;

	WRITE_CMD(CMD_CODE, chip_num, chip->command.CMD_READ1);

	WRITE_CMD(ADD_CODE, chip_num, chip->col_addr + chip->row_addr - 1);
	WRITE_DATA(offset >> width);
	if (chip->row_addr > 2) {
		WRITE_DATA(page);
		WRITE_DATA(page >> 16);
	} else
		WRITE_DATA(page);

	WRITE_CMD(CMD_CODE, chip_num, chip->command.CMD_READ2);

	WRITE_CMD(NOP_CODE, 0x0, 0x7);

	WRITE_CMD(READ_CODE, chip_num, 0x0);
	WRITE_DATA(size - 1);

	comip_nand_receive(width, buf, size);

	return 0;
}

static int comip_nand_write_page(ulong chip_num, ulong page,
				uchar *dbuf, uchar *sbuf, ulong flags)
{
	struct nand_chip *chip = chip_table[chip_num];
	uchar width = chip->data_width;
	uchar *sptr;

	chip_num += 1;

	if (dbuf != NULL) {
		if (chip->ecc_bit != NAND_ECC_1BIT)
			comip_nfc_set_ecc_type(chip->ecc_bit);
		WRITE_CMD(CMD_CODE, chip_num, chip->command.CMD_WRITE1);

		WRITE_CMD(ADD_CODE, chip_num, chip->col_addr + chip->row_addr - 1);
		WRITE_DATA(0);
		if (chip->row_addr > 2) {
			WRITE_DATA(page);
			WRITE_DATA(page >> 16);
		} else
			WRITE_DATA(page);

		if (chip->ecc_bit == NAND_ECC_1BIT)
			WRITE_CMD(WRITE_CODE, chip_num, 0x1);
		else
			WRITE_CMD(WRITE_CODE, chip_num, 0x0);
		WRITE_DATA(gsectornum * gsectorsize - 1);

		comip_nand_send(width, dbuf, gsectornum * gsectorsize);

		if (sbuf == NULL) {
			sptr = (uchar*)gsbuf2;
			memset(sptr, 0xFF, chip->spare_size);
		} else
			sptr = sbuf;

		comip_ecc_gen(dbuf, sptr, width, chip->ecc_bit);
		if (chip->ecc_bit != NAND_ECC_1BIT)
			comip_nfc_set_ecc_type(NAND_ECC_1BIT);

		WRITE_CMD(CMD_CODE, chip_num, chip->command.CMD_RANDOM_IN);

		WRITE_CMD(ADD_CODE, chip_num, chip->col_addr - 1);
		WRITE_DATA(chip->page_size >> width);

		WRITE_CMD(WRITE_CODE, chip_num, 0x0);
		WRITE_DATA(gsectorspare * gsectornum - 1);

		comip_nand_send(width, sptr, gsectorspare * gsectornum);

		WRITE_CMD(CMD_CODE, chip_num, chip->command.CMD_WRITE2);

		WRITE_CMD(NOP_CODE, 0x0, 0x03);
	} else if (sbuf != NULL) {
		WRITE_CMD(CMD_CODE, chip_num, chip->command.CMD_WRITE1);

		WRITE_CMD(ADD_CODE, chip_num, chip->col_addr + chip->row_addr - 1);
		WRITE_DATA(chip->page_size >> width);
		if (chip->row_addr > 2) {
			WRITE_DATA(page);
			WRITE_DATA(page >> 16);
		} else
			WRITE_DATA(page);

		WRITE_CMD(WRITE_CODE, chip_num, 0x0);
		WRITE_DATA(gsectornum * gsectorspare - 1);

		comip_nand_send(width, sbuf, gsectornum * gsectorspare);

		WRITE_CMD(CMD_CODE, chip_num, chip->command.CMD_WRITE2);

		WRITE_CMD(NOP_CODE, 0x0, 0x03);
	}

	return 0;
}

static int __nand_core_compare_to_pattern
(
	uchar* dbuf,
	ulong dsize,
	uchar* sbuf,
	ulong ssize,
	ulong pattern
)
{
	ulong i;
	ulong* ldbuf = (ulong*)dbuf;
	ulong* lsbuf = (ulong*)sbuf;

	if (ldbuf) {
		for (i = 0; i < dsize / 4; i++)
			if (ldbuf[i] != pattern)
				return 0;
	}

	if (lsbuf) {
		for (i = 0; i < ssize / 4; i++)
			if (lsbuf[i] != pattern)
				return 0;
	}

	return 1;
}

static void __nand_core_fill_oob
(
	uchar* oob_buf,
	uchar ch
)
{
	ulong i;

	for (i = 0; i < ecc_layout->eccbytes; i++)
		oob_buf[ecc_layout->eccpos[i]] = ch;
}

static void __nand_core_yaffs2oob
(
	uchar* yaffs_buf,
	uchar* oob_buf,
	ulong len
)
{
	struct nand_oobfree *oobfree;
	ulong oob_index = 0;
	ulong total_len = 0;
	ulong copy_len;

	while (total_len < len) {
		oobfree = &ecc_layout->oobfree[oob_index];
		if ((len - total_len) > oobfree->length)
			copy_len = oobfree->length;
		else
			copy_len = (len - total_len);
		memcpy(&oob_buf[oobfree->offset], yaffs_buf + total_len, copy_len);
		total_len += copy_len;
		oob_index++;
	}
}

static void __nand_core_oob2yaffs
(
	uchar* yaffs_buf,
	uchar* oob_buf,
	ulong len
)
{
	struct nand_oobfree *oobfree;
	ulong oob_index = 0;
	ulong total_len = 0;
	ulong copy_len;

	while (total_len < len) {
		oobfree = &ecc_layout->oobfree[oob_index];
		if ((len - total_len) > oobfree->length)
			copy_len = oobfree->length;
		else
			copy_len = (len - total_len);
		memcpy(yaffs_buf + total_len, &oob_buf[oobfree->offset], copy_len);
		total_len += copy_len;
		oob_index++;
	}
}

static int __nand_core_sync(ulong chip, int operation)
{
	int status;
	uchar tmp;

	switch (operation) {
		case NAND_PRE_RESET:
			status = comip_nand_get_status(chip,
					&tmp, chip_table[chip]->timing.nRESET);
			if ((status != 0)
					|| (!(tmp & chip_table[chip]->command.MASK_READY))) {
				NAND_PRINT("nand reset failed %d. status 0x%08x\n", status, tmp);
				return -1;
			}
			break;
		case NAND_PRE_ERASE:
			status = comip_nand_get_status(chip,
					&tmp, chip_table[chip]->timing.nERASE);
			if ((status != 0)
					|| (tmp & chip_table[chip]->command.MASK_WRITE)) {
				NAND_PRINT("nand erase failed %d. status 0x%08x\n", status, tmp);
				return -1;
			}
			break;
		case NAND_PRE_WRITE:
			status = comip_nand_get_status(chip,
					&tmp, chip_table[chip]->timing.nPROG);
			if ((status != 0)
					|| (tmp & chip_table[chip]->command.MASK_WRITE)) {
				NAND_PRINT("nand write failed %d. status 0x%08x\n", status, tmp);
				return -1;
			}
			break;
		default:
			break;
	}

	return 0;
}

static int __nand_core_erase_nobbt(ulong chip, ulong block)
{
	int status = 0;

	status = comip_nand_erase(chip, block);
	if (status != 0)
		return status;

	return __nand_core_sync(chip, NAND_PRE_ERASE);
}

static int __nand_core_read_page
(
	ulong chip,
	ulong page,
	uchar* dbuf,
	uchar* sbuf,
	ulong flags
)
{
	int status;

	status = comip_nand_read_page(chip, page, dbuf, sbuf, flags);
	if (status != 0)
		return status;

	return __nand_core_sync(chip, NAND_PRE_READ);
}

static int __nand_core_write_page
(
	ulong chip,
	ulong page,
	uchar* dbuf,
	uchar* sbuf,
	ulong flags
)
{
	int status;

	status = comip_nand_write_page(chip, page, dbuf, sbuf, flags);
	if (status != 0)
		return status;

	return __nand_core_sync(chip, NAND_PRE_WRITE);
}

static int __nand_core_check(ulong chip, ulong block)
{
	int status;
	ulong cnt;
	ulong tmp = chip_table[chip]->bad_block_pos - chip_table[chip]->page_size;
	uchar* sbuf = (uchar*)gsbuf;

	for (cnt = 0; cnt < 2; cnt++) {
		status = comip_nand_read_random(chip,
					block * chip_table[chip]->max_pages + cnt,
					chip_table[chip]->page_size,
					chip_table[chip]->spare_size,
					sbuf);
		if (status != 0)
			return status;

		if (sbuf[tmp] != 0xff)
			return -1;
	}

	return 0;
}

static int __nand_core_block_check(ulong chip, ulong block)
{
	int status = 0;
	ulong i;
	ulong page;
	ulong maxpages = chip_table[chip]->max_pages;
	ulong pagesize = chip_table[chip]->page_size;
	ulong sparesize = chip_table[chip]->spare_size;
	uchar* dbuf = (uchar*)gdbuf;
	uchar* sbuf = (uchar*)gsbuf;

	/* Erase. */
	status = __nand_core_erase_nobbt(chip, block);
	if (status != 0)
		goto bad_block;

	for (i = 0; i < maxpages; i++) {
		page = block * maxpages + i;

		/* Fill buffers with special pattern. */
		memset(dbuf, 0xA5, pagesize);
		memset(sbuf, 0xA5, sparesize);

		/* Write buffer to the current flash page + spare area. */
		status = __nand_core_write_page(chip, page, dbuf, sbuf, 1);
		if (status != 0)
			goto bad_block;

		/* Fill buffers again. */
		memset(dbuf, 0x00, pagesize);
		memset(sbuf, 0x00, sparesize);

		/* Read flash page + spare area. */
		status = __nand_core_read_page(chip, page, dbuf, sbuf, 1);
		if ((status != 0)
			&& (status != NAND_ECC_ERR))
			goto bad_block;

		/* Compare flash page that was read from flash to buffer.*/
		__nand_core_fill_oob(sbuf, 0xA5);
		if (!__nand_core_compare_to_pattern(dbuf,
				pagesize,
				sbuf,
				sparesize,
				0xA5A5A5A5))
			goto bad_block;
	}

	/* Erase. */
	status = __nand_core_erase_nobbt(chip, block);
	if (status != 0)
		goto bad_block;

	for (i = 0; i < maxpages; i++) {
		page = block * maxpages + i;

		/* Fill buffers with special pattern. */
		memset(dbuf, 0x5A, pagesize);
		memset(sbuf, 0x5A, sparesize);

		/* Write buffer to the current flash page + spare area. */
		status = __nand_core_write_page(chip, page, dbuf, sbuf, 1);
		if (status != 0)
			goto bad_block;

		/* Fill buffers again. */
		memset(dbuf, 0x00, pagesize);
		memset(sbuf, 0x00, sparesize);

		/* Read flash page + spare area. */
		status = __nand_core_read_page(chip, page, dbuf, sbuf, 1);
		if ((status != 0)
			&& (status != NAND_ECC_ERR))
			goto bad_block;

		/* Compare flash page that was read from flash to buffer.*/
		__nand_core_fill_oob(sbuf, 0x5A);
		if (!__nand_core_compare_to_pattern(dbuf,
				pagesize,
				sbuf,
				sparesize,
				0x5A5A5A5A))
			goto bad_block;
	}

	/* Erase. */
	status = __nand_core_erase_nobbt(chip, block);
	if (status != 0)
		goto bad_block;

	return 0;

bad_block:
	return -1;
}

static int __nand_core_relocate_block(ulong chip, ulong* block)
{
	struct reloc_table *pRel = bbt_table[chip].reloc;
	ulong maxblocks = chip_table[chip]->max_blocks;
	ulong reloc_block;
	ushort reloc_num;
	int status = 0;

	if (*block >= maxblocks)
		return -1;

	reloc_num = pRel->total;
	if (reloc_num == 0) {
		reloc_block = maxblocks;
	} else {
		do {
			/* Last Entry left off. */
			reloc_block = pRel->item[reloc_num - 1].to;
			reloc_num--;
		} while((reloc_block > maxblocks) && (reloc_num > 0));

		if ((reloc_block > maxblocks) && (reloc_num == 0))
			reloc_block = maxblocks;
	}

	do {
		reloc_block--;
		if (reloc_block < (maxblocks - gmax_reloc_entry))
			return -1;

		status = __nand_core_check(chip, reloc_block);
		if (status != 0)
			continue;

		status = __nand_core_erase_nobbt(chip, reloc_block);
	} while (status != 0);

	/* Relocate it. */
	bbt_table[chip].bbt_state = BBT_CHANGED;
	pRel->item[pRel->total].to = (ushort)reloc_block;
	pRel->item[pRel->total].from = (ushort)(*block);
	pRel->total++;
	*block = reloc_block;

	return 0;
}

static ulong __nand_core_find_real_block(ulong chip, ulong block)
{
	ulong i;
	struct reloc_table *pRel = bbt_table[chip].reloc;

	for (i = 0; i < pRel->total; i++) {
		if (pRel->item[i].from == block) {
			block = pRel->item[i].to;
			/* Start over. */
			i = 0;
			continue;
		}
	}

	return block;
}

static int __nand_core_calc_reloc_entry(ulong chip)
{
	ulong maxblock = chip_table[chip]->max_blocks;

	gmax_reloc_entry = NAND_RELOC_ENTRY(maxblock);
	if (gmax_reloc_entry > NAND_MAX_RELOC_ENTRY)
		gmax_reloc_entry = NAND_MAX_RELOC_ENTRY;

	return 0;
}

static int __nand_core_check_bbt(ulong chip)
{
	if (!chip_table[chip])
		return 0;

	if (!bbt_table[chip].reloc)
		return 0;

	if (bbt_table[chip].reloc->header != VALID_HEADER)
		return 0;

	return 1;
}

static int __nand_core_create_bbt(ulong chip)
{
	ulong i;
	ulong block;
	struct reloc_table *pRel = bbt_table[chip].reloc;
	int status = 0;

	// Create a new header
	memset(pRel, 0xFF, chip_table[chip]->page_size);
	pRel->header = VALID_HEADER;
	pRel->total = 0;
	bbt_table[chip].bbt_state = BBT_CHANGED;
	bbt_table[chip].bbt_slot = chip_table[chip]->max_pages - 1;
	bbt_table[chip].bbt_full = 0;

	/** Empty BBT has been created **/
	for (i = 1; i < chip_table[chip]->max_blocks - gmax_reloc_entry; i++) {
		block = i;
		status = __nand_core_check(chip, block);
		if (status == 0)
			continue;

		/* Relocating the i'th block. */
		status = __nand_core_relocate_block(chip, &block);
		if (status != 0)
			break;
	}

	return status;
}

static int __nand_core_load_bbt(ulong chip)
{
	ulong slot;
	ulong slot_num;
	ulong slot_min;
	ulong slot_max;
	struct reloc_table *bbt = (struct reloc_table *)greloc2;
	struct reloc_table *pRel = bbt_table[chip].reloc;
	int status = 0;

	slot_num = NAND_MAX_BBT_SLOTS;
	slot_max = chip_table[chip]->max_pages - 1;
	slot_min = slot_max - slot_num + 1;
	slot = slot_max;

	status = __nand_core_read_page(chip, slot, (uchar *)pRel, 0, 1);
	if ((status != 0)
		&& (status != NAND_ECC_ERR))
		return status;

	if (pRel->header == VALID_HEADER) {
		while (--slot >= slot_min) {
			status = __nand_core_read_page(chip, slot, (uchar *)bbt, 0, 1);
			if ((status != 0)
				&& (status != NAND_ECC_ERR))
				return status;

			if (bbt->header != VALID_HEADER)
				break;
			memcpy(pRel, bbt, sizeof(struct reloc_table));
		}

		if (slot >= slot_min) {
			bbt_table[chip].bbt_full = 0;
			bbt_table[chip].bbt_slot = slot;
			bbt_table[chip].bbt_state = BBT_UNCHANGED;
		} else {
			bbt_table[chip].bbt_full = 1;
			bbt_table[chip].bbt_slot = slot_max;
			bbt_table[chip].bbt_state = BBT_CHANGED;
		}
	} else {
		__nand_core_create_bbt(chip);
	}

	__nand_core_calc_reloc_entry(chip);

	return 0;
}

static int __nand_core_mark_bad(ulong chip, ulong block)
{
	uchar* sbuf = (uchar*)gsbuf;

	memset(sbuf, 0xFF, chip_table[chip]->spare_size);
	sbuf[0] = 0x11;

	__nand_core_write_page(chip, block * chip_table[chip]->max_pages, 0, sbuf, 0);
	__nand_core_write_page(chip, block * chip_table[chip]->max_pages + 1, 0, sbuf, 0);

	return 0;
}

int __nand_core_write_bbt(ulong chip)
{
	int status = 0;
	uchar *buf;
	ulong i;

	/* update BBT information, since the previous operation erase the BBT. */
	if (bbt_table[chip].bbt_state != BBT_CHANGED)
		return 0;

	if (bbt_table[chip].reloc->header != VALID_HEADER)
		return -1;

	if (bbt_table[chip].bbt_full) {
		buf = (uchar*)grelocbuf;
		for (i = 0; i < chip_table[chip]->max_pages - NAND_MAX_BBT_SLOTS; i++) {
			status = __nand_core_read_page(chip, i, buf + i * chip_table[chip]->page_size, NULL, 1);
			if (status != 0)
				return -1;
		}

		status = __nand_core_erase_nobbt(chip, 0);
		if (status != 0)
			return -1;

		for (i = 0; i < chip_table[chip]->max_pages - NAND_MAX_BBT_SLOTS; i++) {
			status = __nand_core_write_page(chip, i, buf + i * chip_table[chip]->page_size, NULL, 1);
			if (status != 0)
				return -1;
		}

		/* new BBT goes into last page in BLK 0. */
		bbt_table[chip].bbt_slot = chip_table[chip]->max_pages - 1;
	}


	status = __nand_core_write_page(chip,
				bbt_table[chip].bbt_slot,
				(uchar *)(bbt_table[chip].reloc),
				0,
				1);
	if (status != 0)
		return status ;

	bbt_table[chip].bbt_state = BBT_UNCHANGED;
	bbt_table[chip].bbt_full = 0;
	bbt_table[chip].bbt_slot--;
	if (bbt_table[chip].bbt_slot < (chip_table[chip]->max_pages - NAND_MAX_BBT_SLOTS))
		bbt_table[chip].bbt_full = 1;

	return 0;
}


static int __nand_core_read
(
	ulong chip,
	ulong page,
	ulong page_num,
	uchar *dbuf,
	ulong flags
)
{
	ulong i;
	ulong block;
	ulong real_block;
	ulong first_page_offset;
	ulong rd_page_offset;
	ulong rd_page_num;
	ulong pagesize = chip_table[chip]->page_size;
	ulong sparesize = chip_table[chip]->spare_size;
	ulong maxpages = chip_table[chip]->max_pages;
	ulong oob_flag = flags & (NANDF_OOB_YAFFS | NANDF_OOB_RAW);
	uchar *rd_dbuf = dbuf;
	uchar *rd_sbuf = oob_flag ? (rd_dbuf + pagesize) : NULL;
	uchar *rd_sbuf2 = oob_flag ? (uchar *)gsbuf : NULL;
	int status = 0;

	/* Find starting block number and offset. */
	block = page / maxpages;
	first_page_offset = page - block * maxpages;

	while (page_num) {
		real_block = __nand_core_find_real_block(chip, block);
		rd_page_offset = (real_block * maxpages) + first_page_offset;
		rd_page_num = maxpages - first_page_offset;
		first_page_offset = 0;

		if (rd_page_num > page_num)
			rd_page_num = page_num;

		for (i = 0; i < rd_page_num; i++) {
			status = __nand_core_read_page(chip,
						rd_page_offset + i,
						rd_dbuf,
						rd_sbuf,
						flags);
			if (status != 0) {
				if ((flags & NANDF_OOB_RAW) || (flags & NANDF_IGNORE_ERR))
					status = 0;
				else if ((status == NAND_ECC_ERR) && (flags & NANDF_CHECK_BLANK)
						&& __nand_core_compare_to_pattern(rd_dbuf,
									pagesize,
									rd_sbuf,
									sparesize,
									0xFFFFFFFF))
                                        status = 0;
			}

			if (status != 0)
				return status;

			if (oob_flag) {
				if (flags & NANDF_OOB_YAFFS) {
					memset(rd_sbuf2, 0xFF, sparesize);
					__nand_core_oob2yaffs(rd_sbuf2, rd_sbuf, YAFFS2_SPARE_SIZE);
					memcpy(rd_sbuf, rd_sbuf2, sparesize);
				}
				rd_dbuf += pagesize + sparesize;
				rd_sbuf = rd_dbuf + pagesize;
			} else {
				rd_dbuf += pagesize;
			}
		}

		page_num -= rd_page_num;
		block++;
	}

	return status;
}

static int __nand_core_write
(
	ulong chip,
	ulong page,
	ulong page_num,
	uchar* dbuf,
	ulong flags
)
{
	ulong i;
	ulong block;
	ulong real_block;
	ulong first_page_offset;
	ulong wr_page_offset;
	ulong wr_page_num;
	ulong pagesize = chip_table[chip]->page_size;
	ulong sparesize = chip_table[chip]->spare_size;
	ulong maxpages = chip_table[chip]->max_pages;
	ulong oob_flag = flags & (NANDF_OOB_YAFFS | NANDF_OOB_RAW);
	uchar *wr_dbuf = dbuf;
	uchar *wr_sbuf = oob_flag ? (uchar *)gsbuf : NULL;
	uchar *wr_sbuf2 = NULL;
	int status = 0;

	/* Find starting block number and offset. */
	block = page / maxpages;
	first_page_offset = page - block * maxpages;

	while (page_num) {
		real_block = __nand_core_find_real_block(chip, block);

		wr_page_num = maxpages - first_page_offset;
		if (wr_page_num > page_num)
			wr_page_num = page_num;

		while (1) {
			wr_page_offset = (real_block * maxpages) + first_page_offset;

			if (real_block == 0) {
				for (i = 0; i < wr_page_num; i++) {
					wr_dbuf = dbuf + i * pagesize;
					if ((wr_page_offset + i) < (maxpages - NAND_MAX_BBT_SLOTS)) {
						if (!__nand_core_compare_to_pattern(wr_dbuf,
											pagesize,
											wr_sbuf,
											sparesize,
											0xFFFFFFFF)) {
							status = __nand_core_write_page(chip,
											wr_page_offset + i,
											wr_dbuf,
											0,
											flags);
							if (status != 0)
								break;
						}
					}
				}
			} else {
				for (i = 0; i < wr_page_num; i++) {
					if (oob_flag) {
						wr_dbuf = dbuf + i * (pagesize + sparesize);
						wr_sbuf2 = wr_dbuf + pagesize;
						if (__nand_core_compare_to_pattern(wr_dbuf,
											pagesize,
											wr_sbuf2,
											sparesize,
											0xFFFFFFFF))
								continue;

						/* Remap oob data of yaffs image to our layout. */
						memset(wr_sbuf, 0xFF, sparesize);
						__nand_core_yaffs2oob(wr_sbuf2, wr_sbuf, YAFFS2_SPARE_SIZE);
					} else {
						wr_dbuf = dbuf + i * pagesize;
					}

					status = __nand_core_write_page(chip,
										wr_page_offset + i,
										wr_dbuf,
										wr_sbuf,
										flags);
					if (status != 0)
						break;
				}
			}

			/* Write was successful. */
			if (status == 0)
				break;

			__nand_core_mark_bad(chip, real_block);
			status = __nand_core_relocate_block(chip, &real_block);
			if (status != 0)
				return status;
		}

		if (oob_flag)
			dbuf += wr_page_num * (pagesize + sparesize);
		else
			dbuf += wr_page_num * pagesize;
		first_page_offset = 0;
		page_num -= wr_page_num;
		block++;
	}

	return status;
}

static int __nand_core_erase(ulong chip, ulong block)
{
	int status;

	block = __nand_core_find_real_block(chip, block);

	while (1) {
		status = __nand_core_erase_nobbt(chip, block);
		if (status == 0) /* Write was successful. */
			break;

		__nand_core_mark_bad(chip, block);
		status = __nand_core_relocate_block(chip, &block);
		if (status != 0)
			return status;
	}

	return status;
}

static int __nand_fill_boot_header
(
	ulong chip,
	uchar* buf,
	ulong len
)
{
	uchar i;
	uchar page_num;
	uchar nand_info;
	uchar eccbit = chip_table[chip]->ecc_bit;
	ulong maxpages = chip_table[chip]->max_pages;
	ulong pagesize = chip_table[chip]->page_size;
	ulong datawidth = chip_table[chip]->data_width;

	/* Page number. */
	len = *((ulong *)buf);
	if ((len < NAND_BOOTLOADER_SIZE_MIN) || (len > NAND_BOOTLOADER_SIZE_MAX))
		len = (NAND_BOOTLOADER_SIZE_MAX / pagesize) * pagesize;

	page_num = len / pagesize;
	if (len % pagesize)
		page_num++;

	if (page_num > (maxpages - NAND_MAX_BBT_SLOTS))
		return -1;

	/* Reserved. */
	nand_info = 0x80;

	/* Ecc bits. */
	if (eccbit == NAND_ECC_1BIT)
		nand_info |= 0x00;
	else if (eccbit == NAND_ECC_4BIT)
		nand_info |= 0x10;
	else if (eccbit == NAND_ECC_8BIT)
		nand_info |= 0x08;
	else
		return -1;

	/* Data width. */
	if (datawidth == NAND_IO_16BIT) {
		nand_info |= 0x20;
		pagesize /= 2;
	}

	/* Page size. */
	i = 0;
	while (pagesize /= 2)
		i++;
	nand_info |= i;

	/* Byte0 ~ Bytes33. */
	for (i = 0; i < 34; i++)
		*buf++ = nand_info;

	/* Byte34 ~ Bytes67. */
	for (i = 0; i < 34; i++)
		*buf++ = page_num;

	/* SHA. Byte68 ~ Byte99. */
	buf += 32;

	/* Byte100 ~ Byte103. */
	/* Aligned to 64Bytes for SHA. */
	if (len % 64)
		len = ((len / 64) + 1) * 64;
	memcpy(buf, &len, 4);

	return 0;
}

static int __nand_info_init(ulong chip)
{
	ulong maxpages = chip_table[chip]->max_pages;
	ulong pagesize = chip_table[chip]->page_size;
	ulong sparesize = chip_table[chip]->spare_size;
	uchar eccbit = chip_table[chip]->ecc_bit;
	ulong *buf = (ulong *)calloc(1, chip_table[chip]->max_pages * chip_table[chip]->page_size * 2);
	ulong i;

	grelocbuf = buf;
	greloc = grelocbuf + (((maxpages - NAND_MAX_BBT_SLOTS) * pagesize) / sizeof(ulong));
	greloc2 = greloc + (pagesize / sizeof(ulong));
	gdbuf = greloc2 + (pagesize / sizeof(ulong));
	gsbuf = gdbuf + (pagesize / sizeof(ulong));
	gsbuf2 = gsbuf + (sparesize / sizeof(ulong));

	bbt_table[chip].reloc = (struct reloc_table *)greloc;
	bbt_table[chip].bbt_state = BBT_UNCHANGED;
	gchip = chip;

	gsectorsize = 512;
	gsectornum = pagesize / gsectorsize;
	gsectorspare = sparesize / gsectornum;

	if (eccbit == NAND_ECC_1BIT) {
		if (sparesize > 16)
			ecc_layout = &comip_1bit_ecc_lb_nand_oob;
		else
			ecc_layout = &comip_1bit_ecc_sb_nand_oob;
	} else {
		ecc_layout = (struct nand_ecclayout*)(gsbuf2 + (sparesize / sizeof(ulong)));
		memset(ecc_layout, 0, sizeof(struct nand_ecclayout));
		if (eccbit == NAND_ECC_4BIT) {
			geccbitstart = NAND_4BIT_ECC_SPARE_START;
			geccbitsize = NAND_4BIT_ECC_SPARE_SIZE * gsectornum;
		} else if (eccbit == NAND_ECC_8BIT) {
			geccbitstart = NAND_8BIT_ECC_SPARE_START;
			geccbitsize = NAND_8BIT_ECC_SPARE_SIZE * gsectornum;
		} else
			return -1;

		ecc_layout->eccbytes = geccbitsize;
		ecc_layout->oobfree[0].offset = geccbitstart + geccbitsize;
		ecc_layout->oobfree[0].length = sparesize - (geccbitstart + geccbitsize);
		for (i = 0; i < geccbitsize; i++)
			ecc_layout->eccpos[i] = geccbitstart + i;
	}

	return 0;
}

int nand_init(void)
{
	uchar id_buf[4];
	ulong chip;
	ulong i;
	ulong num_retry = 0;
	ulong found = 0;

	memset(chip_table, 0x00, sizeof(chip_table));
	memset(bbt_table, 0x00, sizeof(bbt_table));

retry:
	comip_nand_init(0);

	/* probe the nand chip and register it. */
	for (chip = 0; chip < NAND_CHIP_NUM; chip++) {
		comip_nand_read_id(chip, id_buf);
		/* this cs has no chip, go to next. */
		if (id_buf[0] == 0x00)
			continue;

		memcpy(&i, id_buf, sizeof(ulong));
		printf("nand id: 0x%lx\n", i);

		i = 0;
		while (NAND_SUPPORT_TABLE[i] != NULL) {
			if ((id_buf[0] == NAND_SUPPORT_TABLE[i]->maker_code)
				&& (id_buf[1] == NAND_SUPPORT_TABLE[i]->device_code)) {
				memcpy(&(chip_table[chip]), &(NAND_SUPPORT_TABLE[i]), sizeof(struct nand_chip *));
				found++;
				printf("found nand: %s\n", chip_table[chip]->name);
				/* we have found the right chip. */
				goto out;
			}
			i++;
		}
	}

out:
	if (!found) {
		num_retry++;
		if (num_retry <= 20) {
			udelay(10000);
			goto retry;
		} else {
			NAND_PRINT("nand init failed\n");
			return -1;
		}
	}

	__nand_info_init(chip);

	/* initialize the nand controller. */
	comip_nand_init(1);

	return __nand_core_load_bbt(chip);
}

int nand_finalize(void)
{
	ulong chip = gchip;

	return __nand_core_write_bbt(chip);
}

/*
 * buf : write buffer addr
 * start: nand write start addr, raw data page aligned, not include oob data
 * size: nand write size, if yaffs, raw and oob page aligned,include oob data
 * flags: nand operation flags
 *
 */
int nand_write(uchar* buf, ulong start, ulong size, ulong flags)
{
	ulong chip = gchip;
	ulong page_start;
	ulong page_end;
	ulong page_size;
	ulong spare_size;
	ulong chip_size;
	ulong oob_flag = (flags & NANDF_OOB_YAFFS) | (flags & NANDF_OOB_RAW);
	int status = 0;

	if (!__nand_core_check_bbt(chip))
		return -1;

	if (!start)
		__nand_fill_boot_header(chip, buf, size);

	page_size = chip_table[chip]->page_size;
	spare_size = chip_table[chip]->spare_size;
	if (!buf || !size || (start % page_size))
		return -1;

	chip_size = chip_table[chip]->max_blocks * chip_table[chip]->max_pages;
	chip_size = oob_flag ? (chip_size * (page_size + spare_size)) : (chip_size * page_size);
	if ((start > chip_size) || ((start + size) > chip_size))
		return -1;

	if (oob_flag) {
		page_start = start / page_size;
		page_end = page_start + (size / (page_size + spare_size));
		if(size % (page_size + spare_size))
			page_end++;
	} else {
		page_start = start / page_size;
		page_end = (start + size) / page_size;
		if ((start + size) % page_size) {
			page_end++;
			memset(buf + start + size, 0xFF, page_size - ((start + size) % page_size));
		}
	}

	status = __nand_core_write(chip,
					page_start,
					page_end - page_start,
					buf,
					flags);
	if (status != 0)
		return status;

	/* Write the BBT. */
	return nand_finalize();
}

/*
 * buf : read buffer addr
 * start: nand read start addr, raw data page aligned, not include oob data
 * size: nand read size, if yaffs, raw and oob page aligned,include oob data
 * flags: nand operation flags
 *
 */
int nand_read(uchar *buf, ulong start, ulong size, ulong flags)
{
	ulong chip = gchip;
	ulong page_start;
	ulong page_end;
	ulong chip_size;
	ulong page_size = chip_table[chip]->page_size;
	ulong spare_size = chip_table[chip]->spare_size;
	ulong oob_flag = flags & (NANDF_OOB_YAFFS | NANDF_OOB_RAW);
	int status = 0;

	if (!__nand_core_check_bbt(chip))
		return -1;

	if (!buf || !size || (start % page_size))
		return -1;

	chip_size = chip_table[chip]->max_blocks * chip_table[chip]->max_pages;
	chip_size = oob_flag ? (chip_size * (page_size + spare_size)) : (chip_size * page_size);
	if ((start > chip_size) || ((start + size) > chip_size))
		return -1;

	if (oob_flag) {
		page_start = start / page_size;
		page_end = page_start + (size / (page_size + spare_size));
		if (size % (page_size + spare_size))
			page_end++;
	} else {
		page_start = start / page_size;
		page_end = (start + size) / page_size;
		if ((start + size) % page_size)
			page_end++;
	}

	status = __nand_core_read(chip,
					page_start,
					page_end - page_start,
					buf,
					flags);

	return status;
}

int nand_erase_force(void)
{
	int status = 0;
	ulong chip = gchip;
	ulong i;

	/* Check all blocks. */
	for (i = 1; i < chip_table[chip]->max_blocks; i++) {
		status = __nand_core_block_check(chip, i);

		if (status != 0)
			/* Mark bad block. */
			__nand_core_mark_bad(chip, i);
	}

	/* Erase the block 0. */
	status = __nand_core_erase_nobbt(chip, 0);
	if (status != 0)
		return status;

	/* Create the BBT. */
	status = __nand_core_create_bbt(chip);
	if (status != 0)
		return status;

	/* Write the BBT. */
	return nand_finalize();
}

int nand_erase(ulong start, ulong len)
{
	ulong i;
	ulong block_start;
	ulong block_end;
	ulong page_start;
	ulong page_end;
	ulong chip = gchip;
	ulong page_size = chip_table[chip]->page_size;
	ulong max_pages = chip_table[chip]->max_pages;
	int status = 0;

	if (!__nand_core_check_bbt(chip))
		return -1;

	if (!len || (start % (page_size * max_pages)))
		return -1;

	page_start = start / page_size;
	page_end = (start + len) / page_size;
	if ((start + len) % page_size)
		page_end++;

	block_start = page_start / max_pages;
	block_end = page_end / max_pages;
	if (page_end % max_pages)
		block_end++;

	if (block_end > (chip_table[chip]->max_blocks - gmax_reloc_entry))
		block_end = chip_table[chip]->max_blocks - gmax_reloc_entry;

	if ((block_start >= (chip_table[chip]->max_blocks - gmax_reloc_entry))
		|| (block_start >= block_end))
		return -1;

	for (i = block_start; i < block_end; i++) {
		if (i == 0) {
			bbt_table[chip].bbt_state = BBT_CHANGED;
			bbt_table[chip].bbt_slot = max_pages - 1;
			status = __nand_core_erase_nobbt(chip, 0);
			if (status != 0)
				return status; /* Fatal error. */
		} else {
			status = __nand_core_erase(chip, i);
			if (status != 0)
				return status; /* Fatal error. */
		}
	}

	/* Write the BBT. */
	return nand_finalize();
}

