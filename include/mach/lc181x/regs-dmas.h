#ifndef __ASM_ARCH_REGS_DMAS_H
#define __ASM_ARCH_REGS_DMAS_H

#define DMAS_EN 			(0x00)
#define DMAS_CLR			(0x04)
#define DMAS_STA			(0x08)
#define DMAS_INT_RAW		 	(0x0C)
#define DMAS_INT_EN0		 	(0x10)
#define DMAS_INT_EN1		 	(0x14)
#define DMAS_INT_EN2		 	(0x18)
#define DMAS_INT0			(0x1C)
#define DMAS_INT1			(0x20)
#define DMAS_INT2			(0x24)
#define DMAS_INT_CLR		 	(0x28)
#define DMAS_INTV_UNIT		 	(0x2C)
#define DMAS_LP_CTL 		 	(0x3FC)

#define DMAS_CH_REG(ch)			(AP_DMAS_BASE + 0x40 + (ch) * 0x20)
#define DMAS_CH_SAR(ch)			DMAS_CH_REG(ch)
#define DMAS_CH_DAR(ch)			(DMAS_CH_REG(ch) + 0x4)
#define DMAS_CH_CTL0(ch)		(DMAS_CH_REG(ch) + 0x8)
#define DMAS_CH_CTL1(ch)		(DMAS_CH_REG(ch) + 0xC)
#define DMAS_CH_CA(ch)			(DMAS_CH_REG(ch) + 0x10)
#define DMAS_CH_WD(ch)			(AP_DMAS_BASE + 0x240 + (ch) * 4)

#define DMAS_REG(offset)		(AP_DMAS_BASE + offset)

#endif /* __ASM_ARCH_REGS_DMAS_H */
