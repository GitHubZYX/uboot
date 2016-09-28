#include <common.h>
#include "usbdebug.h"

typedef unsigned char bool;

static const char hex_asc[] = "0123456789abcdef";
#define hex_asc_lo(x) hex_asc[((x) & 0x0f)]
#define hex_asc_hi(x) hex_asc[((x) & 0xf0) >> 4]

void hex_dump_to_buffer(const void *buf, size_t len, int rowsize,
			int groupsize, char *linebuf, size_t linebuflen,
			bool ascii)
{
	const u8 *ptr = buf;
	u8 ch;
	int j, lx = 0;

	if (rowsize != 16 && rowsize != 32)
		rowsize = 16;

	if (!len)
		goto nil;
	if (len > rowsize)      /* limit to one line at a time */
		len = rowsize;
	if ((len % groupsize) != 0) /* no mixed size output */
		groupsize = 1;

	switch (groupsize) {
	case 8: {
		const u64 *ptr8 = buf;
		int ngroups = len / groupsize;

		for (j = 0; j < ngroups; j++)
			lx += sprintf(linebuf + lx,
					"%s%16.16llx", j ? " " : "",
					(unsigned long long)*(ptr8 + j));
		break;
	}

	case 4: {
		const u32 *ptr4 = buf;
		int ngroups = len / groupsize;

		for (j = 0; j < ngroups; j++)
					lx += sprintf(linebuf + lx,
					"%s%8.8x", j ? " " : "", *(ptr4 + j));
		break;
	}

	case 2: {
		const u16 *ptr2 = buf;
		int ngroups = len / groupsize;

		for (j = 0; j < ngroups; j++)
					lx += sprintf(linebuf + lx,
					"%s%4.4x", j ? " " : "", *(ptr2 + j));
		break;
	}

	default:
		for (j = 0; (j < len) && (lx + 3) <= linebuflen; j++) {
			ch = ptr[j];
			linebuf[lx++] = hex_asc_hi(ch);
			linebuf[lx++] = hex_asc_lo(ch);
			linebuf[lx++] = ' ';
		}
		if (j)
			lx--;

		break;
	}
	if (!ascii)
		goto nil;

nil:
	linebuf[lx++] = '\0';
}

void print_hex_dump(const char *level, const char *prefix_str, int prefix_type,
			int rowsize, int groupsize,
			const void *buf, size_t len, bool ascii)
{
	const u8 *ptr = buf;
	int i, linelen, remaining = len;
	char linebuf[32 * 3 + 2 + 32 + 1];
	//printf("-------- %s+ ----------\n",__func__);
	if (rowsize != 16 && rowsize != 32)
		rowsize = 16;

	for (i = 0; i < len; i += rowsize) {
		linelen = min(remaining, rowsize);
		remaining -= rowsize;

		hex_dump_to_buffer(ptr + i, linelen, rowsize, groupsize,
					linebuf, sizeof(linebuf), ascii);

		switch (prefix_type) {
		case DUMP_PREFIX_ADDRESS:
			printf("%s%s%p: %s\n",
					level, prefix_str, ptr + i, linebuf);
			break;
		case DUMP_PREFIX_OFFSET:
			printf("%s%s%.8x: %s\n", level, prefix_str, i, linebuf);
			break;
		default:
			printf("%s%s%s\n", level, prefix_str, linebuf);
			break;
		}
	}
	//printf("-------- %s- ----------\n",__func__);
}
