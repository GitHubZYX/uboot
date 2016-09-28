/* 
 **
 ** Use of source code is subject to the terms of the LeadCore license agreement under
 ** which you licensed source code. If you did not accept the terms of the license agreement,
 ** you are not authorized to use source code. For the terms of the license, please see the
 ** license agreement between you and LeadCore.
 **
 ** Copyright (c) 2013-2020  LeadCoreTech Corp.
 **
 **  PURPOSE:
 **
 **  CHANGE HISTORY:
 **
 **  Version		Date		Author		Description
 **  1.0.0		2013-06-05	zouqiao		created
 **
 */

#ifndef	__NAND_DEV_H__
#define __NAND_DEV_H__

enum {
	NAND_ECC_1BIT,
	NAND_ECC_4BIT,
	NAND_ECC_8BIT
};

enum {
	NAND_IO_8BIT,
	NAND_IO_16BIT
};

struct nand_oobfree {
	ulong offset;
	ulong length;
};

struct nand_ecclayout {
	ulong eccbytes;
	ulong eccpos[448];
	struct nand_oobfree oobfree[32];
};

struct nand_timing {
	uchar tWP;		// the nWE pulse width, ns
	uchar tWC;		// write cycle time, ns
	uchar tCLS;		// time of control signal to WE setup, ns
	uchar tCLH;		// time of control signal hold, ns

	uchar tRP;		// the nRE pulse width, ns
	uchar tRC;		// read cycle time, ns
	uchar tCS;		// CS setup, ns
	uchar tCH;		// CS hold, ns

	ulong nFEAT;		// the poll time of feature operaion, us
	ulong nREAD;		// the poll time of read, us
	ulong nPROG;		// the poll time of program, us
	ulong nERASE;		// the poll time of erase, us
	ulong nRESET;		// the poll time of reset, us
};

struct nand_command {
	uchar CMD_READ_STATUS;
	uchar CMD_READ1;
	uchar CMD_READ2;
	uchar CMD_READx;	// for some special nand which divides page into 2 part
	uchar CMD_READs;	// for some special nand which reads spare area in a page
	uchar CMD_WRITE1;
	uchar CMD_WRITE2;
	uchar CMD_ERASE1;
	uchar CMD_ERASE2;
	uchar CMD_RANDOM_IN;
	uchar CMD_RANDOM_OUT1;
	uchar CMD_RANDOM_OUT2;
	uchar MASK_WRITE;	// IO bit poll whether write failed, it usually be (0x1 << 0)
	uchar MASK_READY;	// IO bit poll whether chip is ready, it usually be (0x1 << 6)
};

struct nand_chip {
	char *name;

	uchar maker_code;
	uchar device_code;
	uchar third_code;
	uchar forth_code;

	uchar col_addr;		// the format of address input
	uchar row_addr;		// how many byte be needed to input address

	ulong max_planes;	// the number of planes
	ulong max_blocks;	// the number of blocks
	ulong max_bad_blocks;	// the possible number of initial invalid blocks
	ulong max_pages;		// the number of pages in a block
	ulong page_size;		// the size of main area in a page
	ulong spare_size;	// the size of spare area in a page
	uchar data_width;	// the IO width
	uchar ecc_bit;
	ulong bad_block_pos;	//which byte is the flag of identifying initial invalid block,

	struct nand_timing timing;
	struct nand_command command;
};

static struct nand_chip micron4GbX16 = {
	"Micron 4Gb 16-bit",

	0x2C,
	0xBC,
	0x90,
	0x66,

	2,			/* the format of address input. */
	3,			/* how many byte be needed to input address. */

	1,			/* the number of planes. */
	2048,			/* the number of blocks. */
	80,			/* the possible number of bad blocks. */
	64,			/* the number of pages in a block. */
	4096,			/* the size of main area in a page. */
	224,			/* the size of spare area in a page. */
	NAND_IO_16BIT,		/* the IO width. */
	NAND_ECC_8BIT,		/* the ECC bits. */
	4096,			/* which byte is the flag of identifying initial invalid block. */

	{
		25, /* the nWE pulse width, ns. */
		45, /* write cycle, ns. */
		35, /* control signal to WE setup, ns. */
		10, /* control signal hold, ns. */

		25, /* the nRE pulse width, ns. */
		45, /* read cycle, ns. */
		35, /* CS setup, ns. */
		10, /* CS hold, ns. */

		10, /* the poll time of feature operation, us. */
		70, /* the poll time of read, us. */
		700, /* the poll time of program, us. */
		5000, /* the poll time of erase, us. */
		500, /* the poll time of reset, us. */
	},

	{
		0x70,		/* CMD_READ_STATUS. */
		0x00,		/* CMD_READ1. */
		0x30,		/* CMD_READ2. */
		0,		/* CMD_READx. */
		0,		/* CMD_READs. */
		0x80,		/* CMD_WRITE1. */
		0x10,		/* CMD_WRITE2. */
		0x60,		/* CMD_ERASE1. */
		0xD0,		/* CMD_ERASE2. */
		0x85,		/* CMD_RANDOM_IN. */
		0x05,		/* CMD_RANDOM_OUT1. */
		0xE0,		/* CMD_RANDOM_OUT2. */
		0x01,		/* MASK_WRITE. */
		0x40,		/* MASK_READY. */
	},
};

static struct nand_chip micron8GbX16 = {
	"Micron 8Gb 16-bit",

	0x2C,
	0xB3,
	0x90,
	0x66,

	2,			/* the format of address input. */
	3,			/* how many byte be needed to input address. */

	1,			/* the number of planes. */
	4096,			/* the number of blocks. */
	80,			/* the possible number of bad blocks. */
	64,			/* the number of pages in a block. */
	4096,			/* the size of main area in a page. */
	224,			/* the size of spare area in a page. */
	NAND_IO_16BIT,		/* the IO width. */
	NAND_ECC_8BIT,		/* the ECC bits. */
	4096,			/* which byte is the flag of identifying initial invalid block. */

	{
		25, /* the nWE pulse width, ns. */
		45, /* write cycle, ns. */
		35, /* control signal to WE setup, ns. */
		10, /* control signal hold, ns. */

		25, /* the nRE pulse width, ns. */
		45, /* read cycle, ns. */
		35, /* CS setup, ns. */
		10, /* CS hold, ns. */

		10, /* the poll time of feature operation, us. */
		70, /* the poll time of read, us. */
		700, /* the poll time of program, us. */
		5000, /* the poll time of erase, us. */
		500, /* the poll time of reset, us. */
	},

	{
		0x70,		/* CMD_READ_STATUS. */
		0x00,		/* CMD_READ1. */
		0x30,		/* CMD_READ2. */
		0,		/* CMD_READx. */
		0,		/* CMD_READs. */
		0x80,		/* CMD_WRITE1. */
		0x10,		/* CMD_WRITE2. */
		0x60,		/* CMD_ERASE1. */
		0xD0,		/* CMD_ERASE2. */
		0x85,		/* CMD_RANDOM_IN. */
		0x05,		/* CMD_RANDOM_OUT1. */
		0xE0,		/* CMD_RANDOM_OUT2. */
		0x01,		/* MASK_WRITE. */
		0x40,		/* MASK_READY. */
	},
};

/* User should add the chip information in this table and before NAND_CHIP_TABLE_TAIL. */
static struct nand_chip *NAND_SUPPORT_TABLE[] = {
	&micron4GbX16,
	&micron8GbX16,
	NULL,
};

#endif /*__NAND_DEV_H__*/
