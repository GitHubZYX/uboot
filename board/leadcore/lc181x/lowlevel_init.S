#include <config.h>
#include <version.h>

/*_TEXT_BASE:*/
/*	.word	TEXT_BASE*/	/* sdram load addr from config.mk */

.globl cpu1_loop
cpu1_loop:
	b .

.globl lowlevel_init
lowlevel_init:
	/*set smp bit in Auxiliary Ctl reg*/
	mrc	p15, 0, r0, c1, c0, 1
	orr	r0, r0, #(1 << 6)	@smp bit
	mcr	p15, 0, r0, c1, c0, 1

	/* back to arch calling code */
	mov	pc, lr

/*
DOMAIN_IO:
	.word	0x2

DOMAIN_USER:
	.word	0x1
*/

.equ	CONFIG_PCC_CONF,	0xB4
.equ	CONFIG_MODE1,		0x10
.equ	CONFIG_MODE2,		0x14
.equ	CONF_MOD_UART1_CLK_MODE_R,	0x0A

/* misc values */
.equ	IRQ_MASK,		0x80           /* IRQ mask value */
.equ	FIQ_MASK,		0x40           /* FIQ mask value */
