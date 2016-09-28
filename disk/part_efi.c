/*
 * Copyright (C) 2008 RuggedCom, Inc.
 * Richard Retanubun <RichardRetanubun@RuggedCom.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Problems with CONFIG_SYS_64BIT_LBA:
 *
 * struct disk_partition.start in include/part.h is sized as ulong.
 * When CONFIG_SYS_64BIT_LBA is activated, lbaint_t changes from ulong to uint64_t.
 * For now, it is cast back to ulong at assignment.
 *
 * This limits the maximum size of addressable storage to < 2 Terra Bytes
 */
#include <asm/unaligned.h>
#include <common.h>
#include <command.h>
#include <ide.h>
#include <part_efi.h>
#include <linux/ctype.h>
#include <mmc.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef HAVE_BLOCK_DEVICE

#define PAD_COUNT(s, pad) ((s - 1) / pad + 1)
#define PAD_SIZE(s, pad) (PAD_COUNT(s, pad) * pad)

#define ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, pad)		\
	char __##name[ROUND(PAD_SIZE(size * sizeof(type), pad), align)  \
		      + (align - 1)];					\
									\
	type *name = (type *) ALIGN((uintptr_t)__##name, align)

#define ALLOC_CACHE_ALIGN_BUFFER_PAD(type, name, size, pad)		\
	ALLOC_ALIGN_BUFFER_PAD(type, name, size, ARCH_DMA_MINALIGN, pad)


/**
 * efi_crc32() - EFI version of crc32 function
 * @buf: buffer to calculate crc32 of
 * @len - length of buf
 *
 * Description: Returns EFI-style CRC32 value for @buf
 */
static inline u32 efi_crc32(const void *buf, u32 len)
{
	return crc32(0, buf, len);
}

/*
 * Private function prototypes
 */

static int pmbr_part_valid(struct efi_partition *part);
static int is_pmbr_valid(legacy_mbr * mbr);
static int is_gpt_valid(block_dev_desc_t * dev_desc, unsigned long long lba,
				gpt_header * pgpt_head, gpt_entry ** pgpt_pte);
static gpt_entry *alloc_read_gpt_entries(block_dev_desc_t * dev_desc,
				gpt_header * pgpt_head);
static int is_pte_valid(gpt_entry * pte);

static char *print_efiname(gpt_entry *pte)
{
	static char name[PARTNAME_SZ + 1];
	int i;
	for (i = 0; i < PARTNAME_SZ; i++) {
		u8 c;
		c = pte->partition_name[i] & 0xff;
		c = (c && !isprint(c)) ? '.' : c;
		name[i] = c;
	}
	name[PARTNAME_SZ] = 0;
	return name;
}

static void uuid_string(unsigned char *uuid, char *str)
{
	static const u8 le[16] = {3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11,
				  12, 13, 14, 15};
	int i;

	for (i = 0; i < 16; i++) {
		sprintf(str, "%02x", uuid[le[i]]);
		str += 2;
		switch (i) {
		case 3:
		case 5:
		case 7:
		case 9:
			*str++ = '-';
			break;
		}
	}
}

static efi_guid_t system_guid = PARTITION_SYSTEM_GUID;

static inline int is_bootable(gpt_entry *p)
{
	return p->attributes.fields.legacy_bios_bootable ||
		!memcmp(&(p->partition_type_guid), &system_guid,
			sizeof(efi_guid_t));
}

#ifdef CONFIG_EFI_PARTITION
/*
 * Public Functions (include/part.h)
 */

void print_part_efi(block_dev_desc_t * dev_desc)
{
	ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_head, 1, dev_desc->blksz);
	gpt_entry *gpt_pte = NULL;
	int i = 0;
	char uuid[37];

	if (!dev_desc) {
		printf("%s: Invalid Argument(s)\n", __func__);
		return;
	}
	/* This function validates AND fills in the GPT header and PTE */
	if (is_gpt_valid(dev_desc, GPT_PRIMARY_PARTITION_TABLE_LBA,
			 gpt_head, &gpt_pte) != 1) {
		printf("%s: *** ERROR: Invalid GPT ***\n", __func__);
		return;
	}

	debug("%s: gpt-entry at %p\n", __func__, gpt_pte);

	printf("Part\tStart LBA\tEnd LBA\t\tName\n");
	printf("\tAttributes\n");
	printf("\tType UUID\n");
	printf("\tPartition UUID\n");

	for (i = 0; i < le32_to_cpu(gpt_head->num_partition_entries); i++) {
		/* Stop at the first non valid PTE */
		if (!is_pte_valid(&gpt_pte[i]))
			break;

		printf("%3d\t0x%08llx\t0x%08llx\t\"%s\"\n", (i + 1),
			le64_to_cpu(gpt_pte[i].starting_lba),
			le64_to_cpu(gpt_pte[i].ending_lba),
			print_efiname(&gpt_pte[i]));
		printf("\tattrs:\t0x%016llx\n", gpt_pte[i].attributes.raw);
		uuid_string(gpt_pte[i].partition_type_guid.b, uuid);
		printf("\ttype:\t%s\n", uuid);
		uuid_string(gpt_pte[i].unique_partition_guid.b, uuid);
		printf("\tuuid:\t%s\n", uuid);
	}

	/* Remember to free pte */
	free(gpt_pte);
	return;
}

int get_partition_info_efi(block_dev_desc_t * dev_desc, int part,
				disk_partition_t * info)
{
	ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_head, 1, dev_desc->blksz);
	gpt_entry *gpt_pte = NULL;

	/* "part" argument must be at least 1 */
	if (!dev_desc || !info || part < 1) {
		printf("%s: Invalid Argument(s)\n", __func__);
		return -1;
	}

	/* This function validates AND fills in the GPT header and PTE */
	if (is_gpt_valid(dev_desc, GPT_PRIMARY_PARTITION_TABLE_LBA,
			gpt_head, &gpt_pte) != 1) {
		printf("%s: *** ERROR: Invalid GPT ***\n", __func__);
		return -1;
	}

	if (part > le32_to_cpu(gpt_head->num_partition_entries) ||
	    !is_pte_valid(&gpt_pte[part - 1])) {
		printf("%s: *** ERROR: Invalid partition number %d ***\n",
			__func__, part);
		return -1;
	}

	/* The ulong casting limits the maximum disk size to 2 TB */
	info->start = (u64)le64_to_cpu(gpt_pte[part - 1].starting_lba);
	/* The ending LBA is inclusive, to calculate size, add 1 to it */
	info->size = ((u64)le64_to_cpu(gpt_pte[part - 1].ending_lba) + 1)
		     - info->start;
	info->blksz = dev_desc->blksz;

	sprintf((char *)info->name, "%s",
			print_efiname(&gpt_pte[part - 1]));
	sprintf((char *)info->type, "U-Boot");
	info->bootable = is_bootable(&gpt_pte[part - 1]);
#ifdef CONFIG_PARTITION_UUIDS
	uuid_string(gpt_pte[part - 1].unique_partition_guid.b, info->uuid);
#endif

	debug("%s: start 0x%lX, size 0x%lX, name %s", __func__,
		info->start, info->size, info->name);

	/* Remember to free pte */
	free(gpt_pte);
	return 0;
}

int test_part_efi(block_dev_desc_t * dev_desc)
{
	ALLOC_CACHE_ALIGN_BUFFER_PAD(legacy_mbr, legacymbr, 1, dev_desc->blksz);

	/* Read legacy MBR from block 0 and validate it */
	if ((dev_desc->block_read_gpt(dev_desc->dev, 0, 1, (ulong *)legacymbr) != 1)
		|| (is_pmbr_valid(legacymbr) != 1)) {
		return -1;
	}
	return 0;
}

/**
 * set_protective_mbr(): Set the EFI protective MBR
 * @param dev_desc - block device descriptor
 *
 * @return - zero on success, otherwise error
 */
static int set_protective_mbr(block_dev_desc_t *dev_desc)
{
	legacy_mbr *p_mbr;

	/* Setup the Protective MBR */
	p_mbr = calloc(1, sizeof(legacy_mbr));

	if (p_mbr == NULL) {
		printf("%s: calloc failed!\n", __func__);
		return -1;
	}
	/* Append signature */
	p_mbr->signature = MSDOS_MBR_SIGNATURE;
	p_mbr->partition_record[0].sys_ind = EFI_PMBR_OSTYPE_EFI_GPT;
	p_mbr->partition_record[0].start_sect = 1;
	p_mbr->partition_record[0].nr_sects = (u32) dev_desc->lba;

	/* Write MBR sector to the MMC device */
	if (dev_desc->block_write_gpt(dev_desc->dev, 0, 1, p_mbr) != 1) {
		printf("** Can't write to device %d **\n",
			dev_desc->dev);
		free(p_mbr);
		return -1;
	}


	free(p_mbr);
	return 0;
}

/**
 * string_uuid(); Convert UUID stored as string to bytes
 *
 * @param uuid - UUID represented as string
 * @param dst - GUID buffer
 *
 * @return return 0 on successful conversion
 */
static int string_uuid(char *uuid, u8 *dst)
{
	efi_guid_t guid;
	u16 b, c, d;
	u64 e;
	u32 a;
	u8 *p;
	u8 i;

	const u8 uuid_str_len = 36;

	/* The UUID is written in text: */
	/* 1        9    14   19   24 */
	/* xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx */

	debug("%s: uuid: %s\n", __func__, uuid);

	if (strlen(uuid) != uuid_str_len)
		return -1;

	for (i = 0; i < uuid_str_len; i++) {
		if ((i == 8) || (i == 13) || (i == 18) || (i == 23)) {
			if (uuid[i] != '-')
				return -1;
		} else {
			if (!isxdigit(uuid[i]))
				return -1;
		}
	}

	a = (u32)simple_strtoul(uuid, NULL, 16);
	b = (u16)simple_strtoul(uuid + 9, NULL, 16);
	c = (u16)simple_strtoul(uuid + 14, NULL, 16);
	d = (u16)simple_strtoul(uuid + 19, NULL, 16);
	e = (u64)simple_strtoull(uuid + 24, NULL, 16);

	p = (u8 *) &e;
	guid = EFI_GUID(a, b, c, d >> 8, d & 0xFF,
			*(p + 5), *(p + 4), *(p + 3),
			*(p + 2), *(p + 1) , *p);

	memcpy(dst, guid.b, sizeof(efi_guid_t));

	return 0;
}


int write_gpt_table(block_dev_desc_t *dev_desc,
		gpt_header *gpt_h, gpt_entry *gpt_e)
{
	const int pte_blk_cnt = BLOCK_CNT((gpt_h->num_partition_entries
					   * sizeof(gpt_entry)), dev_desc);
	u32 calc_crc32;
	u64 val;

	debug("max lba: %x\n", (u32) dev_desc->lba);
	/* Setup the Protective MBR */
	if (set_protective_mbr(dev_desc) < 0)
		goto err;

	/* Generate CRC for the Primary GPT Header */
	calc_crc32 = efi_crc32((const unsigned char *)gpt_e,
			      le32_to_cpu(gpt_h->num_partition_entries) *
			      le32_to_cpu(gpt_h->sizeof_partition_entry));
	gpt_h->partition_entry_array_crc32 = cpu_to_le32(calc_crc32);

	calc_crc32 = efi_crc32((const unsigned char *)gpt_h,
			      le32_to_cpu(gpt_h->header_size));
	gpt_h->header_crc32 = cpu_to_le32(calc_crc32);

	/* Write the First GPT to the block right after the Legacy MBR */
	if (dev_desc->block_write_gpt(dev_desc->dev, 1, 1, gpt_h) != 1)

		goto err;


	if (dev_desc->block_write_gpt(dev_desc->dev, 2, pte_blk_cnt, gpt_e)
	    != pte_blk_cnt)
		goto err;

	/* recalculate the values for the Second GPT Header */
	val = le64_to_cpu(gpt_h->my_lba);
	gpt_h->my_lba = gpt_h->alternate_lba;
	gpt_h->alternate_lba = cpu_to_le64(val);
	gpt_h->header_crc32 = 0;

	calc_crc32 = efi_crc32((const unsigned char *)gpt_h,
			      le32_to_cpu(gpt_h->header_size));
	gpt_h->header_crc32 = cpu_to_le32(calc_crc32);

	if (dev_desc->block_write_gpt(dev_desc->dev,
				  le32_to_cpu(gpt_h->last_usable_lba + 1),
				  pte_blk_cnt, gpt_e) != pte_blk_cnt)
		goto err;

	if (dev_desc->block_write_gpt(dev_desc->dev,
				  le32_to_cpu(gpt_h->my_lba), 1, gpt_h) != 1)
		goto err;

	debug("GPT successfully written to block device!\n");
	return 0;

 err:
	printf("** Can't write to device %d **\n", dev_desc->dev);
	return -1;
}

int gpt_fill_pte(gpt_header *gpt_h, gpt_entry *gpt_e,
		disk_partition_t *partitions, int parts)
{
	u32 offset = (u32)le32_to_cpu(gpt_h->first_usable_lba);
	ulong start;
	int i, k;
	size_t name_len;
#ifdef CONFIG_PARTITION_UUIDS
	char *str_uuid;
#endif


	for (i = 0; i < parts; i++) {
		/* partition starting lba */
		start = partitions[i].start;
		if (start && (start < offset)) {
			printf("Partition overlap\n");
			return -1;
		}
		
		if (start) {
			gpt_e[i].starting_lba = cpu_to_le64(start);
			offset = start + partitions[i].size;
		} else {
			gpt_e[i].starting_lba = cpu_to_le64(offset);
			offset += partitions[i].size;
		}
		
		if (offset >= gpt_h->last_usable_lba) {
			printf("Partitions layout exceds disk size\n");
			return -1;
		}
		/* partition ending lba */
		if ((i == parts - 1) && (partitions[i].size == 0))
			/* extend the last partition to maximuim */
			gpt_e[i].ending_lba = gpt_h->last_usable_lba;
		else
			gpt_e[i].ending_lba = cpu_to_le64(offset - 1);

		/* partition type GUID */
		memcpy(gpt_e[i].partition_type_guid.b,
			&PARTITION_BASIC_DATA_GUID, 16);

#ifdef CONFIG_PARTITION_UUIDS
		str_uuid = partitions[i].uuid;
		if (string_uuid(str_uuid, gpt_e[i].unique_partition_guid.b)) {
			printf("Partition no. %d: invalid guid: %s\n",
				i, str_uuid);
			return -1;
		}
#endif

		/* partition attributes */
		memset(&gpt_e[i].attributes, 0,
		       sizeof(gpt_entry_attributes));

		/* partition name */
		name_len = sizeof(gpt_e[i].partition_name)
			/ sizeof(efi_char16_t);
		for (k = 0; k < name_len; k++)
			gpt_e[i].partition_name[k] =
				(efi_char16_t)(partitions[i].name[k]);

		debug("%s: name: %s offset[%d]: 0x%x size[%d]: 0x%lx\n",
		      __func__, partitions[i].name, i,
		      offset, i, partitions[i].size);
	}

	return 0;
}

int gpt_fill_header(block_dev_desc_t *dev_desc, gpt_header *gpt_h,
		char *str_guid, int parts_count)
{
	gpt_h->signature = cpu_to_le64(GPT_HEADER_SIGNATURE);
	gpt_h->revision = cpu_to_le32(GPT_HEADER_REVISION_V1);
	gpt_h->header_size = cpu_to_le32(sizeof(gpt_header));
	gpt_h->my_lba = cpu_to_le64(1);
	gpt_h->alternate_lba = cpu_to_le64(dev_desc->lba - 1);
	gpt_h->first_usable_lba = cpu_to_le64(34);
	gpt_h->last_usable_lba = cpu_to_le64(dev_desc->lba - 34);
	gpt_h->partition_entry_lba = cpu_to_le64(2);
	gpt_h->num_partition_entries = cpu_to_le32(GPT_ENTRY_NUMBERS);
	gpt_h->sizeof_partition_entry = cpu_to_le32(sizeof(gpt_entry));
	gpt_h->header_crc32 = 0;
	gpt_h->partition_entry_array_crc32 = 0;


	if (string_uuid(str_guid, gpt_h->disk_guid.b))
		return -1;

	return 0;
}

int gpt_restore(block_dev_desc_t *dev_desc, char *str_disk_guid,
		disk_partition_t *partitions, int parts_count)
{
	int ret;

	gpt_header *gpt_h = calloc(1, PAD_TO_BLOCKSIZE(sizeof(gpt_header),
						       dev_desc));
	gpt_entry *gpt_e;

	if (gpt_h == NULL) {
		printf("%s: calloc failed!\n", __func__);
		return -1;
	}

	gpt_e = calloc(1, PAD_TO_BLOCKSIZE(GPT_ENTRY_NUMBERS
					       * sizeof(gpt_entry),
					       dev_desc));

		
	if (gpt_e == NULL) {
		printf("%s: calloc failed!\n", __func__);
		free(gpt_h);
		return -1;
	}

	/* Generate Primary GPT header (LBA1) */
	ret = gpt_fill_header(dev_desc, gpt_h, str_disk_guid, parts_count);
	if (ret)
		goto err;

	/* Generate partition entries */
	ret = gpt_fill_pte(gpt_h, gpt_e, partitions, parts_count);
	if (ret)
		goto err;

	
	/* Write GPT partition table */
	ret = write_gpt_table(dev_desc, gpt_h, gpt_e);

err:
	free(gpt_e);
	free(gpt_h);
	return ret;
}
#endif

/*
 * Private functions
 */
/*
 * pmbr_part_valid(): Check for EFI partition signature
 *
 * Returns: 1 if EFI GPT partition type is found.
 */
static int pmbr_part_valid(struct efi_partition *part)
{
	if (part->sys_ind == EFI_PMBR_OSTYPE_EFI_GPT &&
		get_unaligned_le32(&part->start_sect) == 1UL) {
		return 1;
	}

	return 0;
}

/*
 * is_pmbr_valid(): test Protective MBR for validity
 *
 * Returns: 1 if PMBR is valid, 0 otherwise.
 * Validity depends on two things:
 *  1) MSDOS signature is in the last two bytes of the MBR
 *  2) One partition of type 0xEE is found, checked by pmbr_part_valid()
 */
static int is_pmbr_valid(legacy_mbr * mbr)
{
	int i = 0;

	if (!mbr || le16_to_cpu(mbr->signature) != MSDOS_MBR_SIGNATURE)
		return 0;

	for (i = 0; i < 4; i++) {
		if (pmbr_part_valid(&mbr->partition_record[i])) {
			return 1;
		}
	}
	return 0;
}

/**
 * is_gpt_valid() - tests one GPT header and PTEs for validity
 *
 * lba is the logical block address of the GPT header to test
 * gpt is a GPT header ptr, filled on return.
 * ptes is a PTEs ptr, filled on return.
 *
 * Description: returns 1 if valid,  0 on error.
 * If valid, returns pointers to PTEs.
 */
static int is_gpt_valid(block_dev_desc_t * dev_desc, unsigned long long lba,
			gpt_header * pgpt_head, gpt_entry ** pgpt_pte)
{
	u32 crc32_backup = 0;
	u32 calc_crc32;
	unsigned long long lastlba;

	if (!dev_desc || !pgpt_head) {
		printf("%s: Invalid Argument(s)\n", __func__);
		return 0;
	}

	/* Read GPT Header from device */
	if (dev_desc->block_read_gpt(dev_desc->dev, lba, 1, pgpt_head) != 1) {
		printf("*** ERROR: Can't read GPT header ***\n");
		return 0;
	}
	

	/* Check the GPT header signature */
	if (le64_to_cpu(pgpt_head->signature) != GPT_HEADER_SIGNATURE) {
		printf("GUID Partition Table Header signature is wrong:"
			"0x%llX != 0x%llX\n",
			le64_to_cpu(pgpt_head->signature),
			GPT_HEADER_SIGNATURE);
		return 0;
	}

	/* Check the GUID Partition Table CRC */
	memcpy(&crc32_backup, &pgpt_head->header_crc32, sizeof(crc32_backup));
	memset(&pgpt_head->header_crc32, 0, sizeof(pgpt_head->header_crc32));

	calc_crc32 = efi_crc32((const unsigned char *)pgpt_head,
		le32_to_cpu(pgpt_head->header_size));

	memcpy(&pgpt_head->header_crc32, &crc32_backup, sizeof(crc32_backup));

	if (calc_crc32 != le32_to_cpu(crc32_backup)) {
		printf("GUID Partition Table Header CRC is wrong:"
			"0x%x != 0x%x\n",
		       le32_to_cpu(crc32_backup), calc_crc32);
		return 0;
	}

	/* Check that the my_lba entry points to the LBA that contains the GPT */
	if (le64_to_cpu(pgpt_head->my_lba) != lba) {
		printf("GPT: my_lba incorrect: %llX != %llX\n",
			le64_to_cpu(pgpt_head->my_lba),
			lba);
		return 0;
	}

	/* Check the first_usable_lba and last_usable_lba are within the disk. */
	lastlba = (unsigned long long)dev_desc->lba;
	if (le64_to_cpu(pgpt_head->first_usable_lba) > lastlba) {
		printf("GPT: first_usable_lba incorrect: %llX > %llX\n",
			le64_to_cpu(pgpt_head->first_usable_lba), lastlba);
		return 0;
	}
	if (le64_to_cpu(pgpt_head->last_usable_lba) > lastlba) {
		printf("GPT: last_usable_lba incorrect: %llX > %llX\n",
			(u64) le64_to_cpu(pgpt_head->last_usable_lba), lastlba);
		return 0;
	}

	debug("GPT: first_usable_lba: %llX last_usable_lba %llX last lba %llX\n",
		le64_to_cpu(pgpt_head->first_usable_lba),
		le64_to_cpu(pgpt_head->last_usable_lba), lastlba);

	/* Read and allocate Partition Table Entries */
	*pgpt_pte = alloc_read_gpt_entries(dev_desc, pgpt_head);
	if (*pgpt_pte == NULL) {
		printf("GPT: Failed to allocate memory for PTE\n");
		return 0;
	}

	/* Check the GUID Partition Table Entry Array CRC */
	calc_crc32 = efi_crc32((const unsigned char *)*pgpt_pte,
		le32_to_cpu(pgpt_head->num_partition_entries) *
		le32_to_cpu(pgpt_head->sizeof_partition_entry));

	if (calc_crc32 != le32_to_cpu(pgpt_head->partition_entry_array_crc32)) {
		printf("GUID Partition Table Entry Array CRC is wrong:"
			"0x%x != 0x%x\n",
			le32_to_cpu(pgpt_head->partition_entry_array_crc32),
			calc_crc32);

		free(*pgpt_pte);
		return 0;
	}

	/* We're done, all's well */
	return 1;
}

/**
 * alloc_read_gpt_entries(): reads partition entries from disk
 * @dev_desc
 * @gpt - GPT header
 *
 * Description: Returns ptes on success,  NULL on error.
 * Allocates space for PTEs based on information found in @gpt.
 * Notes: remember to free pte when you're done!
 */
static gpt_entry *alloc_read_gpt_entries(block_dev_desc_t * dev_desc,
					 gpt_header * pgpt_head)
{
	size_t count = 0, blk_cnt;
	gpt_entry *pte = NULL;

	if (!dev_desc || !pgpt_head) {
		printf("%s: Invalid Argument(s)\n", __func__);
		return NULL;
	}

	count = le32_to_cpu(pgpt_head->num_partition_entries) *
		le32_to_cpu(pgpt_head->sizeof_partition_entry);

	debug("%s: count = %u * %u = %zu\n", __func__,
	      (u32) le32_to_cpu(pgpt_head->num_partition_entries),
	      (u32) le32_to_cpu(pgpt_head->sizeof_partition_entry), count);

	/* Allocate memory for PTE, remember to FREE */
	if (count != 0) {
//		pte = memalign(ARCH_DMA_MINALIGN,
//			       PAD_TO_BLOCKSIZE(count, dev_desc));

		pte = calloc(1,
			   PAD_TO_BLOCKSIZE(count, dev_desc));

	}

	if (count == 0 || pte == NULL) {
		printf("%s: ERROR: Can't allocate 0x%zX "
		       "bytes for GPT Entries\n",
			__func__, count);
		return NULL;
	}

	/* Read GPT Entries from device */
	blk_cnt = BLOCK_CNT(count, dev_desc);
	if (dev_desc->block_read_gpt(dev_desc->dev,
		le64_to_cpu(pgpt_head->partition_entry_lba),
		(lbaint_t) (blk_cnt), pte)
		!= blk_cnt) {

		printf("*** ERROR: Can't read GPT Entries ***\n");
		free(pte);
		return NULL;
	}
	return pte;
}

/**
 * is_pte_valid(): validates a single Partition Table Entry
 * @gpt_entry - Pointer to a single Partition Table Entry
 *
 * Description: returns 1 if valid,  0 on error.
 */
static int is_pte_valid(gpt_entry * pte)
{
	efi_guid_t unused_guid;

	if (!pte) {
		printf("%s: Invalid Argument(s)\n", __func__);
		return 0;
	}

	/* Only one validation for now:
	 * The GUID Partition Type != Unused Entry (ALL-ZERO)
	 */
	memset(unused_guid.b, 0, sizeof(unused_guid.b));

	if (memcmp(pte->partition_type_guid.b, unused_guid.b,
		sizeof(unused_guid.b)) == 0) {

		debug("%s: Found an unused PTE GUID at 0x%08X\n", __func__,
		      (unsigned int)(uintptr_t)pte);

		return 0;
	} else {
		return 1;
	}
}

int gpt_find_part_by_name(block_dev_desc_t * dev_desc, const char *name ,struct partition *part)
{
	ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_head, 1, dev_desc->blksz);
	gpt_entry *gpt_pte = NULL;
	int i ;

	/* "part" argument must be at least 1 */
	if (!dev_desc || !name || ! part) {
		printf("%s: Invalid Argument(s)\n", __func__);
		return -1;
	}

	/* This function validates AND fills in the GPT header and PTE */
	if (is_gpt_valid(dev_desc, GPT_PRIMARY_PARTITION_TABLE_LBA,
			gpt_head, &gpt_pte) != 1) {
		printf("%s: *** ERROR: Invalid GPT ***\n", __func__);
		return -1;
	}

	for(i =0; i<le32_to_cpu(gpt_head->num_partition_entries) ; i++){

		if (!strcmp(name, print_efiname(&gpt_pte[i]))){
			part->sector_size = dev_desc->blksz ;
			part->start = (u64)le64_to_cpu(gpt_pte[i].starting_lba);
			part->length = ((u64)le64_to_cpu(gpt_pte[i].ending_lba) + 1)
		     - part->start;
			debug("%s:name=%s,start=%d,len=%d,block size=%d\n", __func__,name,(int) part->start ,(int) part->length,(int) part->sector_size) ;
			free(gpt_pte);
			return 1 ;
		}			
	}
	
	printf("%s: can't find partition named:%s\n",__func__ , name) ;
	free(gpt_pte);
	return 0 ;

}

#if 0
void efi_part_test()
{
	char str_disk_guid[37] = {0} ; /*xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (8-4-4-4-12)*/

	disk_partition_t partitions[20] ;
	disk_partition_t partitions_read ;
	int parts_count = 18 , i ;
	volatile int suspend = 1 ;

	struct mmc *mmc = find_mmc_device(0);

	strcpy(str_disk_guid, "20130617-1104-0000-0000-000000000000") ;
	memset(partitions , 0 , sizeof(partitions[15])) ;


	/*lcboot*/
	partitions[0].start = 0x400 ;
	partitions[0].size = 0x400 ;
	partitions[0].blksz = 0x200 ;
	strcpy(partitions[0].name , "lcboot") ;
	partitions[0].bootable = 0 ;
	strcpy(partitions[0].uuid, "20130530-1104-0000-0000-000000000000") ;


	/*logo*/
	partitions[1].start = 0x800 ;
	partitions[1].size = 0x2000 ;
	partitions[1].blksz = 0x200 ;
	strcpy(partitions[1].name , "logo") ;
	partitions[1].bootable = 0 ;
	strcpy(partitions[1].uuid, "20130530-1104-0000-0000-000000000001") ;

	/*fota*/	
	partitions[2].start = 0x2800 ;
	partitions[2].size = 0x800 ;
	partitions[2].blksz = 0x200 ;
	strcpy(partitions[2].name , "fota") ;
	partitions[2].bootable = 0 ;
	strcpy(partitions[2].uuid, "20130530-1104-0000-0000-000000000002") ;
			
	/*panic*/	
	partitions[3].start = 0x3000 ;
	partitions[3].size = 0x800 ;
	partitions[3].blksz = 0x200 ;
	strcpy(partitions[3].name , "panic") ;
	partitions[3].bootable = 0 ;
	strcpy(partitions[3].uuid, "20130530-1104-0000-0000-000000000003") ;

	/*amt*/	
	partitions[4].start = 0x3800 ;
	partitions[4].size = 0x1000 ;
	partitions[4].blksz = 0x200 ;
	strcpy(partitions[4].name , "amt") ;
	partitions[4].bootable = 0 ;
	strcpy(partitions[4].uuid, "20130530-1104-0000-0000-000000000004") ;

	/*modemarm*/	
	partitions[5].start = 0x4800 ;
	partitions[5].size = 0x8000 ;
	partitions[5].blksz = 0x200 ;
	strcpy(partitions[5].name , "modemarm") ;
	partitions[5].bootable = 0 ;
	strcpy(partitions[5].uuid, "20130530-1104-0000-0000-000000000006") ;

	/*modemdsp0*/	
	partitions[6].start = 0xC800 ;
	partitions[6].size = 0x1620 ;
	partitions[6].blksz = 0x200 ;
	strcpy(partitions[6].name , "modemdsp0") ;
	partitions[6].bootable = 0 ;
	strcpy(partitions[6].uuid, "20130530-1104-0000-0000-000000000007") ;

	/*modemdsp1*/	
	partitions[7].start = 0xDE20 ;
	partitions[7].size = 0x9E0 ;
	partitions[7].blksz = 0x200 ;
	strcpy(partitions[7].name , "modemdsp1") ;
	partitions[7].bootable = 0 ;
	strcpy(partitions[7].uuid, "20130530-1104-0000-0000-000000000008") ;

	/*kernel*/	
	partitions[8].start = 0xE800 ;
	partitions[8].size = 0x2000 ;
	partitions[8].blksz = 0x200 ;
	strcpy(partitions[8].name , "kernel") ;
	partitions[8].bootable = 0 ;
	strcpy(partitions[8].uuid, "20130530-1104-0000-0000-000000000009") ;

	/*ramdisk*/
	partitions[9].start = 0x10800 ;
	partitions[9].size = 0x800 ;
	partitions[9].blksz = 0x200 ;
	strcpy(partitions[9].name , "ramdisk") ;
	partitions[9].bootable = 0 ;
	strcpy(partitions[9].uuid, "20130530-1104-0000-0000-000000000010") ;

	/*ramdisk_amt1*/	
	partitions[10].start = 0x11000 ;
	partitions[10].size = 0x800 ;
	partitions[10].blksz = 0x200 ;
	strcpy(partitions[10].name , "ramdisk_amt1") ;
	partitions[10].bootable = 0 ;
	strcpy(partitions[10].uuid, "20130530-1104-0000-0000-000000000011") ;

	/*ramdisk_amt3*/	
	partitions[11].start = 0x11800 ;
	partitions[11].size = 0x800 ;
	partitions[11].blksz = 0x200 ;
	strcpy(partitions[11].name , "ramdisk_amt3") ;
	partitions[11].bootable = 0 ;
	strcpy(partitions[11].uuid, "20130530-1104-0000-0000-000000000012") ;


	/*ramdisk_recovery*/	
	partitions[12].start = 0x12000 ;
	partitions[12].size = 0x800 ;
	partitions[12].blksz = 0x200 ;
	strcpy(partitions[12].name , "ramdisk_recovery") ;
	partitions[12].bootable = 0 ;
	strcpy(partitions[12].uuid, "20130530-1104-0000-0000-000000000011") ;

	/*kernel_recovery*/	
	partitions[13].start = 0x12800 ;
	partitions[13].size = 0x2000 ;
	partitions[13].blksz = 0x200 ;
	strcpy(partitions[13].name , "kernel_recovery") ;
	partitions[13].bootable = 0 ;
	strcpy(partitions[13].uuid, "20130530-1104-0000-0000-000000000012") ;



	/*misc*/	
	partitions[14].start = 0x18000 ;
	partitions[14].size = 0x1000 ;
	partitions[14].blksz = 0x200 ;
	strcpy(partitions[14].name , "misc") ;
	partitions[14].bootable = 0 ;
	strcpy(partitions[14].uuid, "20130530-1104-0000-0000-000000000013") ;

	/*cache*/	
	partitions[15].start = 0x20000 ;
	partitions[15].size = 0x40000 ;
	partitions[15].blksz = 0x200 ;
	strcpy(partitions[15].name , "cache") ;
	partitions[15].bootable = 0 ;
	strcpy(partitions[15].uuid, "20130530-1104-0000-0000-000000000014") ;

	/*system*/	
	partitions[16].start = 0x60000 ;
	partitions[16].size = 0xc0000 ;
	partitions[16].blksz = 0x200 ;
	strcpy(partitions[16].name , "system") ;
	partitions[16].bootable = 0 ;
	strcpy(partitions[16].uuid, "20130530-1104-0000-0000-000000000015") ;

	/*userdata*/	
	partitions[17].start = 0x120000 ;
	partitions[17].size = 0x600000 ;
	partitions[17].blksz = 0x200 ;
	strcpy(partitions[17].name , "userdata") ;
	partitions[17].bootable = 0 ;
	strcpy(partitions[17].uuid, "20130530-1104-0000-0000-000000000016") ;


	gpt_restore(&mmc->block_dev, (char*)str_disk_guid,
			(disk_partition_t *)partitions,  parts_count) ;

#if 1

	for(i = 1 ; i<= parts_count ; i++)
	{
		memset(&partitions_read, 0 , sizeof(partitions_read)) ;
		get_partition_info_efi(&mmc->block_dev , i, &partitions_read) ;


		printf("\n\npartitions[%d].start=0x%x", i , partitions_read.start) ;
		printf("\npartitions[%d].size=0x%x", i , partitions_read.size) ;
		printf("\npartitions[%d].blksz=0x%x", i , partitions_read.blksz) ;
		printf("\npartitions[%d].name=%s", i , partitions_read.name) ;
		printf("\npartitions[%d].bootable=%d", i , partitions_read.bootable) ;
		printf("\npartitions[%d].uuid=%s\n", i , partitions_read.uuid) ;

	}

	printf("\n\n\n\n\nsuspending ~~~~\n") ;
//	while(suspend) ;
#endif
}
#if 0
void sdcardtest()
{
#define test_block 0x1
	struct mmc *mmc = find_mmc_device(0);

	char block[513] = {0} ;
	int i = 0 ; 
	int ret = 0 ;
	int flag = 0 ;

	int j = 0 ;

	enum {
		PART_USER = 0,
		PART_BOOT_1 = 1,
		PART_BOOT_2 = 2
	};

	mmc_switch_partition(0, PART_BOOT_1);

	memset(block , 0xAA , 512) ;
	ret =mmc->block_dev.block_write(0 ,0 ,1 , block) ;

	mmc_switch_partition(0, PART_USER);

	memset(block , 0x55 , 512) ;
	ret =mmc->block_dev.block_write(0 ,mmc->block_dev.lba-1,1 , block) ;
	printf("ret of write lba =%d\n", mmc->block_dev.lba) ;

	memset(block , 0x00 , 512) ;
	ret =mmc->block_dev.block_read(0 ,mmc->block_dev.lba-1, 1, block) ;
	printf("ret of read lba =%d\n", mmc->block_dev.lba) ;

	printf("~~~~~in user ~~~\n") ;
	for( i = 0 ; i<512 ; i++)
	{
		printf("0x%x ", block[i]) ;

		if(i%16==0 && i!=0)
			printf("\n") ;


	}


	mmc_switch_partition(0, PART_BOOT_1);

	memset(block , 0x00 , 512) ;
	ret =mmc->block_dev.block_read(0 ,0 ,1 , block) ;

	printf("~~~~~in boot ~~~\n") ;
	for( i = 0 ; i<512 ; i++)
	{
		printf("0x%x ", block[i]) ;

		if(i%16==0 && i!=0)
			printf("\n") ;


	}


	while(1);

	for(j=0 ; j< 0x120000 ; j++)
	{
		flag =1 ;
			
		for( i = 0 ; i<512 ; i++)
		{
			block[i] = i%0xff ;
		}
		
		ret =mmc->block_dev.block_write(0 ,j ,1 , block) ;
		
		printf("%s,ret=%d\n" ,__func__,ret) ;
		
		for( i = 0 ; i<512 ; i++)
		{
			block[i] = 0 ;
		}
		
		ret =mmc->block_dev.block_read(0 ,j ,1 , block) ;
		
		printf("%s,ret=%d\n" ,__func__,ret) ;
		
		
		for( i = 0 ; i<512 ; i++)
		{
			if(block[i] != i%0xff){
				printf("check failed ! block[%d]=%d\n", i , block[i]) ;
				flag =1 ;
				break ;
				
			}
		
		}

		flag = 0 ;
		if(flag == 0);
			//printf("@@@@@@@@@@@@@@ check pass @@@@@@@@@@@@@@\n") ;



	}

	while(1) ;


	for( i = 0 ; i<512 ; i++)
	{
		printf("0x%x ", block[i]) ;

		if(i%16==0)
			printf("\n") ;


	}
	

	printf("oh yeah ,stop here !!!") ;
	while(1) ;
}
#endif

#endif

#endif
