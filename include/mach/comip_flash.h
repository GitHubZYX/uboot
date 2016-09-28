#ifndef __COMIP_FLASH_H__
#define __COMIP_FLASH_H__

struct partition {
	ulong sector_size;
	ulong start;
	ulong length;
	const char *name;
};

extern int flash_init(void);
extern int flash_partition_string(char *buffer);
extern void flash_partition_fixup(u64 capacity);
extern int flash_partition_read(const char *name, uchar *buffer, ulong size);
extern int flash_partition_read_all(const char *name, uchar *buffer, ulong size);
extern int flash_partition_start_find(const char *name);

#if defined(CONFIG_FALSH_WRITE)
extern int flash_partition_write(const char *name, uchar *buffer, ulong size);
extern int flash_partition_erase_write(const char *name, uchar *buffer, ulong size);
extern int flash_partition_erase(const char *name);
#endif

#if defined(CONFIG_COMIP_TARGETLOADER)
extern struct mmc * flashex_get_device(void);
extern int flashex_erase(u64 start, u64 len);
extern int flashex_partiton_find_by_name(char *name, u32 *blk_start, u32 *blk_len, u32 *blk_size);
extern int flashex_partition_check_switch(u64 start, u64 length);
extern int flashex_write(u64 start, void *buf, u64 len);
extern int flashex_read(u64 start, void *buf, u32 len);
#endif
#endif /* __COMIP_FLASH_H__ */
