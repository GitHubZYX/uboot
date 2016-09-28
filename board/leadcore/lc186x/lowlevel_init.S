#include <config.h>
#include <version.h>

/*_TEXT_BASE:*/
/*	.word	TEXT_BASE*/	/* sdram load addr from config.mk */

.globl cpu1_loop
cpu1_loop:
	b .

.globl lowlevel_init
lowlevel_init:
	/* set smp bit in Auxiliary Ctl reg */
	mrc	p15, 0, r0, c1, c0, 1
	orr	r0, r0, #(1 << 6)	@smp bit
	mcr	p15, 0, r0, c1, c0, 1

#ifdef CONFIG_COMIP_LC1860_MENTOR
	/* sa and ha-cpux boot */
	mrc	p15, 0, r0, c0, c0, 5	@get cpu id
	mov	r1, r0
	and	r1, r1, #0x00000f00
	cmp	r1, #0x100
	bne	a7_cb
	and	r0, r0, #0x03
	cmp	r0, #0
	beq	ha0_boot
	cmp	r0, #1
	beq	ha1_wb
	cmp	r0, #2
	beq	ha2_wb
	cmp	r0, #3
	beq	ha3_wb

ha0_boot:
	/* judge ha-cpu0 cold/warm boot firstly */
	ldr	r0, =0xA0110014	@CPU3_CBOOT_ADDR as boot flag
	ldr	r0, [r0]
	cmp	r0, #0xf8
	bne	a7_cb
	ldr	r0, =0xA0110000	@CTL_AP_HA7_CORE0_WBOOT_ADDR
	ldr	pc, [r0]

a7_cb:
#endif
	/* back to arch calling code */
	mov	pc, lr

#ifdef CONFIG_COMIP_LC1860_MENTOR
	/*ha-cpux warm boot*/
ha1_wb:
	ldr	r1, =0xA0110008	@CTL_AP_HA7_CORE1_WBOOT_ADDR
	ldr	pc, [r1]
ha2_wb:
	ldr	r1, =0xA0110010	@CTL_AP_HA7_CORE2_WBOOT_ADDR
	ldr	pc, [r1]
ha3_wb:
	ldr	r1, =0xA0110018	@CTL_AP_HA7_CORE3_WBOOT_ADDR
	ldr	pc, [r1]
#endif

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
