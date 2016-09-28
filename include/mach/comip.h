#ifndef __ASM_ARCH_COMIP_REGS_H
#define __ASM_ARCH_COMIP_REGS_H

#if defined(CONFIG_CPU_LC1810) || defined(CONFIG_CPU_LC1813)
#include "lc181x/comip-lc181x.h"
#include "lc181x/irqs.h"
#elif defined(CONFIG_CPU_LC1860)
#include "lc186x/comip-lc186x.h"
#include "lc186x/irqs.h"
#endif

#include "cpu.h"
#include "hardware.h"
#include "comip_flash.h"
#include "comip_gpio.h"
#include "comip_irq.h"
#include "comip_i2c.h"
#include "comip_keypad.h"
#include "comip_memctl.h"
#include "comip_mfp.h"
#include "comip_mmc.h"
#include "comip_nand.h"
#include "comip_pmic.h"
#include "comip_recovery.h"
#include "comip_setup.h"
#include "comip_leds.h"

#if defined(CONFIG_DEBUG_TIMER_ID)
extern void debug_timer_init(void);
extern unsigned long debug_timer_getus(void);
#endif

#endif /* __ASM_ARCH_COMIP_REGS_H */
