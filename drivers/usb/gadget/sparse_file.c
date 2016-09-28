/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <common.h>
#include <asm/errno.h>
#include <command.h>
#include "sparse_format.h"
#include <mmc.h>
#include "error.h"
DECLARE_GLOBAL_DATA_PTR;

//#define FASTBOOT_DEBUG
#ifdef FASTBOOT_DEBUG
#define fb_printf(fmt, args...) printf(fmt, ##args)
#else
#define fb_printf(fmt, args...) do {} while(0)
#endif

#define SPARSE_HEADER_MAJOR_VER 1

int check_sparse_image(void *data)
{
	return (((struct sparse_header *)data)->magic == SPARSE_HEADER_MAGIC);
}

ulong flash_write(uchar *source, unsigned long long size,
			unsigned long long start, unsigned long long partition_size,
			ulong flags)
{
	int ret = -EIO;
#if defined(CONFIG_COMIP_MMC)
	int is_pad=0x0;
	unsigned int pad_count =0x0;
	struct mmc *mmc = find_mmc_device(0);

	if (size % mmc->write_bl_len ==  0) {
		is_pad=0;
	}  else {
		is_pad =1;
		pad_count = mmc->write_bl_len - size % mmc->write_bl_len ;
	}

	ret = mmc->block_dev.block_write(0, start / mmc->write_bl_len,
			(size % mmc->write_bl_len) ?
			(size / mmc->write_bl_len + 1) :
			(size / mmc->write_bl_len),
			source);
	ret *= mmc->write_bl_len;

	if (is_pad) {
		ret -= pad_count ;
	}

#elif defined(CONFIG_COMIP_NAND)
	if (!nand_write(buffer, start, size, flags))
		ret = size;
#endif

	return ret;
}

int write_unsparse(uchar *source, u64 size,
			u64 start, u64 partition_size,
			ulong flags)
{
	sparse_header_t *header = (void *) source;
	u32 i=0x0;
	u64 outlen =0x0;
	u64 temp=0x0;
	u64 temp_a=0x0;
	u64 temp_b=0x0;

	fb_printf("sparse_header:\n");
	fb_printf("\t         magic=0x%08X\n", header->magic);
	fb_printf("\t       version=%u.%u\n", header->major_version,
						header->minor_version);
	fb_printf("\t file_hdr_size=%u\n", header->file_hdr_sz);
	fb_printf("\tchunk_hdr_size=%u\n", header->chunk_hdr_sz);
	fb_printf("\t        blk_sz=%u\n", header->blk_sz);
	fb_printf("\t    total_blks=%u\n", header->total_blks);
	fb_printf("\t  total_chunks=%u\n", header->total_chunks);
	fb_printf("\timage_checksum=%u\n", header->image_checksum);

	if (header->magic != SPARSE_HEADER_MAGIC) {
		printf("sparse: bad magic\n");
		put_error("20001");
		return 1;
	}

	temp_a=header->total_blks;
	temp_b=header->blk_sz;
	temp= temp_a* temp_b ;
	printf("sparse_file_size=%lluMB\n",temp/(1024*1024ull));
	printf("partition_size=%lluMB\n",partition_size/(1024*1024ull));
	if (temp> partition_size) {
		put_error("20002");
		printf("sparse: section size %llu MB limit: exceeded\n",
				partition_size/(1024*1024ull));
		return 1;
	}

	if ((header->major_version != SPARSE_HEADER_MAJOR_VER) ||
	    (header->file_hdr_sz != sizeof(sparse_header_t)) ||
	    (header->chunk_hdr_sz != sizeof(chunk_header_t))) {
		printf("sparse: incompatible format\n");
		put_error("20003");
		return 1;
	}

	/* Skip the header now */
	source += header->file_hdr_sz;

	for (i = 0; i < header->total_chunks; i++) {
		u64 clen = 0;
		chunk_header_t *chunk = (void *) source;

		fb_printf("chunk_header:\n");
		fb_printf("\t    chunk_type=%u\n", chunk->chunk_type);
		fb_printf("\t      chunk_sz=%u\n", chunk->chunk_sz);
		fb_printf("\t      total_sz=%u\n", chunk->total_sz);
		/* move to next chunk */
		source += sizeof(chunk_header_t);

		switch (chunk->chunk_type) {
		case CHUNK_TYPE_RAW:
			temp_a=chunk->chunk_sz;
			temp_b=header->blk_sz;
			clen = temp_a * temp_b ;
			fb_printf("sparse: RAW blk=%d bsz=%d:"
			       " write(addr=0x%llx,clen=%llu)\n",
			       chunk->chunk_sz, header->blk_sz, start, clen);

			if (chunk->total_sz != (clen + sizeof(chunk_header_t))) {
				printf("sparse: bad chunk size for"
				       " chunk %d, type Raw\n", i);
				put_error("20004");
				return 1;
			}

			outlen += clen;
			if (outlen > partition_size) {
				printf("sparse: section size %llu MB limit:"
				       " exceeded\n", partition_size/(1024*1024));
				put_error("20005");
				return 1;
			}


			temp_a=flash_write(source, clen, start, partition_size, flags);
			if ( temp_a!= clen) {
				printf("sparse: block write to addr 0x%llx"
					" of %llu bytes failed\n",
					start, clen);
				return 1;
			}
			start += clen;
			source += clen;
			break;

		case CHUNK_TYPE_DONT_CARE:
			if (chunk->total_sz != sizeof(chunk_header_t)) {
				printf("sparse: bogus DONT CARE chunk\n");
				put_error("20007");
				return 1;
			}
			temp_a=chunk->chunk_sz;
			temp_b=header->blk_sz;
			clen = temp_a * temp_b ;
			fb_printf("sparse: DONT_CARE blk=%d bsz=%d:"
			       " skip(addr=0x%llx,clen=%llu)\n",
			       chunk->chunk_sz, header->blk_sz, start, clen);

			outlen += clen;
			if (outlen > partition_size) {
				printf("sparse: section size %llu MB limit:"
				       " exceeded\n", partition_size/(1024*1024));
				put_error("20008");
				return 1;
			}
			//sector += (clen / blksz);
			start += clen;
			break;

		default:
			printf("sparse: unknown chunk ID %04x\n",
			       chunk->chunk_type);
			return 1;
		}
	}

	fb_printf("sparse: out-length %llu MB\n", outlen/(1024*1024));
	return 0;
}

