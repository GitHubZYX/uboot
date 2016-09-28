#ifndef __ASM_ARCH_REGS_MFP_H
#define __ASM_ARCH_REGS_MFP_H

#define MFP_REG(x)		(MUX_PIN_BASE + ((x) * 4))

#define MFP_AF_MASK		(0x3 << 0)
#define MFP_AF(x)		(((x) << 0) & MFP_AF_MASK)

#define MFP_PULL_MASK		(0x3 << 2)
#define MFP_PULL(x)		(((x) << 2) & MFP_PULL_MASK)

#define MFP_DS_MASK		(0x7 << 4)
#define MFP_DS(x)		(((x) << 4) & MFP_DS_MASK)

#define MUXPIN_TESTPIN_CTRL0	(MUX_PIN_BASE + 0x4D4)

#endif /* __ASM_ARCH_REGS_MFP_H */
