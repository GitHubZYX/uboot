#ifndef _MEMCTL_INITS_H_
#define _MEMCTL_INITS_H_

#if (CONFIG_DDR_BUS_CLK == 663000000)
#include "memctl_inits_663.h"
#elif (CONFIG_DDR_BUS_CLK == 533000000)
#include "memctl_inits_553.h"
#elif (CONFIG_DDR_BUS_CLK == 520000000)
#include "memctl_inits_520.h"
#elif (CONFIG_DDR_BUS_CLK == 331500000)
#include "memctl_inits_332.h"
#elif (CONFIG_DDR_BUS_CLK == 221000000)
#include "memctl_inits_221.h"
#elif (CONFIG_DDR_BUS_CLK == 165750000)
#include "memctl_inits_166.h"
#else
#error "undef CONFIG_DDR_BUS_CLK"
#endif

#endif
