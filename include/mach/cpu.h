#ifndef __LC18XX_CPU_H__
#define __LC18XX_CPU_H__

#if defined(CONFIG_CPU_LC1810) || defined(CONFIG_CPU_LC1813)
#define MF_ID_BIT			(30)
#define PART_ID_BIT			(18)
#define REV_ID_BIT			(16)
#define VOLT_ID_BIT			(9)
#elif defined(CONFIG_CPU_LC1860)
#define MF_ID_BIT			(22)
#define PART_ID_BIT			(10)
#define REV_ID_BIT			(8)
#define VOLT_ID_BIT			(0)
#endif

#define MF_ID_MASK			(0x3 << MF_ID_BIT)
#define MF_ID_TSMC			(0x0 << MF_ID_BIT)
#define MF_ID_SMIC			(0x1 << MF_ID_BIT)

#define PART_ID_MASK			(0x3ffc << REV_ID_BIT)
#define PART_ID_LC1810			(0x2420 << REV_ID_BIT)
#define PART_ID_LC1813			(0x2460 << REV_ID_BIT)
#define PART_ID_LC1813S			(0x2480 << REV_ID_BIT)
#define PART_ID_LC1860			(0x2820 << REV_ID_BIT)
#define PART_ID_LC1860C			(0x282c << REV_ID_BIT)

#define REV_ID_MASK			(0x3 << REV_ID_BIT)
#define REV_ID_1			(0x1 << REV_ID_BIT)
#define REV_ID_2			(0x2 << REV_ID_BIT)
#define REV_ID_3			(0x3 << REV_ID_BIT)

#if defined(CONFIG_CPU_LC1810) || defined(CONFIG_CPU_LC1813)
#define VOLT_ID_MASK			(0x3 << VOLT_ID_BIT)
#define VOLT_ID_INC1			(0x1 << VOLT_ID_BIT)
#define VOLT_ID_INC2			(0x2 << VOLT_ID_BIT)
#define VOLT_ID_INC3			(0x3 << VOLT_ID_BIT)
#elif defined(CONFIG_CPU_LC1860)
#define VOLT_ID_MASK			(0x7 << VOLT_ID_BIT)
#endif

#if defined(CONFIG_CPU_LC1810)
#define cpu_is_lc1810()		((gd->bb_id & PART_ID_MASK) == PART_ID_LC1810)
#define CHIP_ID_DEFAULT		(MF_ID_TSMC | PART_ID_LC1810)
#else
#define cpu_is_lc1810()		(0)
#endif

#if defined(CONFIG_CPU_LC1813)
#define cpu_is_lc1813()		((gd->bb_id & PART_ID_MASK) == PART_ID_LC1813)
#define cpu_is_lc1813s()	((gd->bb_id & PART_ID_MASK) == PART_ID_LC1813S)
#define cpu_volt_flag()		(gd->bb_id & VOLT_ID_MASK)
#define CHIP_ID_DEFAULT		(MF_ID_TSMC | PART_ID_LC1813)
#else
#define cpu_is_lc1813()		(0)
#define cpu_is_lc1813s()	(0)
#endif

#if defined(CONFIG_CPU_LC1860)
#define cpu_is_lc1860()		((gd->bb_id & PART_ID_MASK) == PART_ID_LC1860)
#define cpu_is_lc1860c()	((gd->bb_id & PART_ID_MASK) == PART_ID_LC1860C)
#define cpu_is_lc1860_eco1()	((gd->bb_id & REV_ID_MASK) == REV_ID_1)
#define cpu_is_lc1860_eco2()	((gd->bb_id & REV_ID_MASK) == REV_ID_2)
#define cpu_volt_flag()		(gd->bb_id & VOLT_ID_MASK)
#define CHIP_ID_DEFAULT		(MF_ID_TSMC | PART_ID_LC1860)
#else
#define cpu_is_lc1860()		(0)
#define cpu_is_lc1860c()	(0)
#define cpu_is_lc1860_eco1()	(0)
#define cpu_is_lc1860_eco2()	(0)
#endif

#endif
