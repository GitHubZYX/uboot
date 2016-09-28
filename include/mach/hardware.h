#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include <common.h>
#include <asm/io.h>

#define __REG(x)        (*((volatile u32 *)(x)))

#define __REG32(x)      (*((volatile u32 *)(x)))
#define __REG16(x)      (*((volatile u16 *)(x)))
#define __REG8(x)       (*((volatile u8 *)(x)))

/*
 * common register operation
 */
#define REG32(reg)	(__REG32(reg))
#define REG16(reg)	(__REG16(reg))
#define REG8(reg)	(__REG8(reg))

#endif /* __HARDWARE_H__ */
