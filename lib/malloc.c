/*
 *  linux/lib/malloc.c
 *
 *  Copyright (C) leadcore
 */
#include <common.h>

#define ALIGN_SIZE (1024)

DECLARE_GLOBAL_DATA_PTR;

/**
 * calloc - malloc area of memory, init with zero.
 * @n: number of continues area
 * @size: size of each area
 *  * returns the address of the area or NULL
 */
void *calloc(unsigned int n, unsigned int size)
{
	static unsigned int mem_addr_start = 0;

	if (!mem_addr_start)
		mem_addr_start = (unsigned int)(gd->start_addr_sp - CONFIG_STACKSIZE);

	if (n * size > mem_addr_start)
		return NULL;

	mem_addr_start = mem_addr_start - n * size;
	mem_addr_start = mem_addr_start - (mem_addr_start % ALIGN_SIZE);

	memset((void *)mem_addr_start, 0x00, n * size);
	return (void *)mem_addr_start;
}

void free(void *ptr)
{
}
