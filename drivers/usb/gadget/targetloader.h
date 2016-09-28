#ifndef __TARGETLOADER_H__
#define __TARGETLOADER_H__

#include <linux/types.h>

#define TL_VERSION (0xB0)

#ifdef CONFIG_TL_NODDR
#define TMP_BUF_SIZE  (27 * 1024ul)
#else
#define TMP_BUF_SIZE  (100 * 1024 * 1024ul)
#endif

#define UP_DATA_SIZE  (4 * 1024ul)
#define UP_EXTRA_SIZE (8)

#define DEF_PACK_LEN 0x06
#define DEF_BLK_SIZE 0x200

#define CMD_CMD_SIZE       1
#define CMD_CRC_SIZE       1
#define CMD_DATA_FILL_SIZE 3 /* fill for 4byte aligned */

#define FLASH_TYPE_BIN    0
#define FLASH_TYPE_YAFFS2 50
#define FLASH_TYPE_OOB    60

/* buffer descriptor */
struct buffer_desc {
	u8 *buf;
	u32 size;
};

/* expect information in next packet */
struct expect_info {
	u8 flag;
	u8 cmd_id;
	u16 size;
};

/*  loader communicate with host and typical packets format as follows:
 *
 * 1. download
 * 1.1 common packet:
 * +--------+--------------+---------+
 * | cmd_id | cmd_data ... | cmd_crc |
 * +--------+--------------+---------+
 * | 1 byte |   6 bytes    | 1 byte  |
 *
 * 1.2 CMD_DATA packet (when zip = 0x0):
 * +----------+-----------+-----------------------+---------+
 * | CMD_DATA | FILL_SIZE |       data ...        | cmd_crc |
 * +----------+-----------+-----------------------+---------+
 * |         4 bytes      |   default 8192 bytes  | 1 byte  |
 *
 * 1.3 CMD_DATA packet (when zip = 0x2):
 * +----------+-----------+-----------+----------+--------------------+--------+
 * | CMD_DATA | FILL_SIZE | data_type | data_len |(fill_val) data ... |cmd_crc |
 * +----------+-----------+-----------+----------+--------------------+--------+
 * |         4 bytes      |  4 bytes  | 4 bytes  | default 8192 bytes | 1 byte |
 *
 * 2. upload
 * 2.1 CMD_DATA packet:
 * +----------+-----------------------+---------+
 * | CMD_DATA |       data ...        | cmd_crc |
 * +----------+-----------------------+---------+
 * | 1 byte   |   default 4096 bytes  | 1 byte  |
 *
 * Data structure to describe a packet.
 */
struct packet {
	u8 direction; /* flag for program */
	u8 cmd_id;
	u16 data_size;
	u8 *data;
};

/* seting infomation when loader communicate with host */
struct loader_set {
	u32 mode; // 0:download 1:upload
	u16 once_data_bytes; // CMD_DATA data segment size, always 0x2000

/*
 * @allpacks_bytes: how many bytes the host will send.
 * Note :
 * It may equal to image file size or not!
 * For a zip image, special chunks (data_type=0x1/0x2) will be sent
 * as a single packet in which valid data only contain CMD_DATA,
 * CMD_DATA_FILL_SIZE, data_type, data_len and (fill_val).
 */
	u64 allpacks_bytes;

/*
 * @is_zip: to discrible image type.
 *    0x0 - raw image
 *    0x2 - yaffs image
 *    0x1 - not defined now.
 * All of the work to unzip yaffs image has been done at host SML tool,
 * so we only need parse the packets received.
 */
	u8 is_zip;

	u64 start_addr;
	u64 cur_addr;
	u64 end_addr;
	u64 addr_offset;
};

enum PROTOCOL_CMD_RESP_IDS {
	/* Command Codes */
	CMD_NONE                = 0x00,
	CMD_LINK_EST            = 0x0A,
	CMD_RVRS_DWNLD          = 0x0B,
	CMD_CHANGE_CONFIG       = 0x0C,
	CMD_DATA                = 0x0D,
	CMD_DEV_PROTECT         = 0x0E,
	CMD_DEV_UNPROTECT       = 0x0F,
	CMD_ERASEFLASH          = 0x10,
	CMD_PROG_DEV_ST_ADDR    = 0x11,
	CMD_PROG_DEV_END_ADDR   = 0x12,
	CMD_DATA_PACKET_SIZE    = 0x13,
	CMD_SW_RESET            = 0x14,
	CMD_HW_RESET            = 0x15,
	CMD_DEVICE_INFO         = 0x16,
	CMD_ERASE_SEG_ST_ADDR   = 0x17,
	CMD_ERASE_SEG_END_ADDR  = 0x18,
	CMD_RAM_CHECK           = 0x19,
	CMD_FLASH_CHECK         = 0x20,
	CMD_DEV_BASE_ADDR       = 0x21,
	CMD_LOCK_SEG_ST_ADDR    = 0x22,
	CMD_LOCK_SEG_END_ADDR   = 0x23,
	CMD_UNLOCK_SEG_ST_ADDR  = 0x24,
	CMD_UNLOCK_SEG_END_ADDR = 0x25,
	CMD_START_EXECUTION     = 0x26,
	CMD_UPLD_DATA_SIZE      = 0x27,
	CMD_NFTL_CHECK_ST       = 0x28,
	CMD_NFTL_CHECK_END      = 0x29,
	CMD_PARTITION_DATA      = 0x2A,
	CMD_UNZIP_INFO          = 0x2D,
	CMD_PART_NAME_LEN       = 0x30,
	CMD_PART_NAME_STRING    = 0x31,
	CMD_PART_REQ_ST         = 0x32,
	CMD_PART_REQ_SZ         = 0x33,

	CMD_DATA_SYNC           = 0x58,

	/* EFUSE command */
	CMD_EFUSE_WRITE_LEN    = 0x5A,
	CMD_EFUSE_WRITE_DATA   = 0x5B,
	CMD_EFUSE_READ         = 0x5C,
	CMD_EFUSE_READ_RSP     = 0x5D,

	/* Invalid command */
	CMD_TL_DOWNLOAD         = 0x55,

	/* Response Codes */
	CMD_CONNECT         = 0x02,
	CMD_ACK             = 0x03,
	CMD_SUCCESS         = 0x04,
	CMD_FAIL            = 0x05,
	CMD_ERASEERR        = 0x06,
	CMD_RESP_DATA       = 0x08,
	CMD_CHECK_SUCCESS   = 0x09,
};

enum BOOLEANS {
	FALSE = 0x0,
	TRUE  = 0x1,
};

enum {
	RESET = 0x0,
	SET   = 0x1,
};

enum DIRECTIONS {
	FROM_HOST,
	TO_HOST,
};

enum DEV_INFO_SUBCMD {
	DEV_FST = 0x01,
	DEV_TYPE,			//storage type, 1=emmc, 2=nand
	DEV_EMMC_OCR = 0x10,
	DEV_EMMC_CID,
	DEV_EMMC_CSD,
	DEV_EMMC_EXT_CSD,
	DEV_EMMC_RCA,
	DEV_EMMC_DSR,
	DEV_EMMC_SCR,
	DEV_NAND_CHIP_ID = 0x20,
	DEV_NAND_TYEP
};

enum {
	NAND_OOB_NONE = 0,
	NAND_OOB_YAFFS,
	NAND_OOB_RAW
};
#endif
