#ifndef __MEMCTL_MULTI_DDR_H__
#define __MEMCTL_MULTI_DDR_H__


#include <common.h>
#include "comip_board_info.h"

struct memctl_multi_ddr_struct{
	char			*name;
	unsigned int		mid;
	unsigned int		did;
	struct memctl_data 	*memctl_data;
	const unsigned int	length;
	u64			memctl0_cs0_size;
	u64			memctl0_cs1_size;
	u64			memctl1_cs0_size;
	u64			memctl1_cs1_size;
};

#endif
