/*
 * arch/arm/mach-comip/include/mach/irqs.h
 *
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** Copyright (c) 2010-2019 LeadCoreTech Corp.
 **
 ** PURPOSE: irq.
 **
 ** CHANGE HISTORY:
 **
 ** Version	Date		Author		Description
 ** 1.0.0		2012-03-06	gaojian		created
 **
 */

#ifndef __MACH_COMIP_IRQS_H
#define __MACH_COMIP_IRQS_H

#define INT_GIC_BASE		0

#define INT_USB_CTL			(INT_GIC_BASE + 32)
#define INT_AP_DMAG			(INT_GIC_BASE + 33)
#define INT_AP_DMAS			(INT_GIC_BASE + 34)
#define INT_AP_DMAC			(INT_GIC_BASE + 35)
#define INT_AP_PWR			(INT_GIC_BASE + 36)
#define INT_DDR_PWR			(INT_GIC_BASE + 37)
#define INT_MIPI			(INT_GIC_BASE + 38)
#define INT_SMMU0			(INT_GIC_BASE + 39)
#define INT_SMMU1			(INT_GIC_BASE + 40)
#define INT_AP_TIMER		(INT_GIC_BASE + 41)
#define INT_2DACC			(INT_GIC_BASE + 42)
#define INT_GPU0			(INT_GIC_BASE + 43)
#define INT_GPU1			(INT_GIC_BASE + 44)
#define INT_GPU3			(INT_GIC_BASE + 45)
#define INT_VIDEOACC0		(INT_GIC_BASE + 46)

#define INT_VIDEOACC1		(INT_GIC_BASE + 48)
#define INT_USB_OTG			(INT_GIC_BASE + 49)
#define INT_USB_HSIC		(INT_GIC_BASE + 50)
#define INT_LCDC0			(INT_GIC_BASE + 51)
#define INT_LCDC1			(INT_GIC_BASE + 52)
#define INT_ISP				(INT_GIC_BASE + 53)
#define INT_SECURITY		(INT_GIC_BASE + 54)
#define INT_KBS				(INT_GIC_BASE + 55)
#define INT_NFC				(INT_GIC_BASE + 56)
#define INT_CIPHER			(INT_GIC_BASE + 57)
#define INT_TPZCTL			(INT_GIC_BASE + 58)
#define INT_AP_WDT1			(INT_GIC_BASE + 59)
#define INT_AP_WDT2			(INT_GIC_BASE + 60)
#define INT_AP_WDT3			(INT_GIC_BASE + 61)
#define INT_I2C0			(INT_GIC_BASE + 62)
#define INT_I2C1			(INT_GIC_BASE + 63)
#define INT_I2C2			(INT_GIC_BASE + 64)
#define INT_I2C3			(INT_GIC_BASE + 65)
#define INT_SDMMC0			(INT_GIC_BASE + 66)
#define INT_SDMMC1			(INT_GIC_BASE + 67)
#define INT_SDMMC2			(INT_GIC_BASE + 68)
#define INT_SSI0			(INT_GIC_BASE + 69)
#define INT_SSI1			(INT_GIC_BASE + 70)
#define INT_SSI2			(INT_GIC_BASE + 71)
#define INT_UART0			(INT_GIC_BASE + 72)
#define INT_UART1			(INT_GIC_BASE + 73)
#define INT_UART2			(INT_GIC_BASE + 74)
#define INT_AP_TIMER0		(INT_GIC_BASE + 75)
#define INT_AP_TIMER1		(INT_GIC_BASE + 76)
#define INT_AP_TIMER2		(INT_GIC_BASE + 77)
#define INT_AP_TIMER3		(INT_GIC_BASE + 78)
#define INT_AP_TIMER4		(INT_GIC_BASE + 79)
#define INT_AP_TIMER5		(INT_GIC_BASE + 80)
#define INT_AP_TIMER6		(INT_GIC_BASE + 81)
#define INT_AP_SA7			(INT_GIC_BASE + 82)
#define INT_AP_HA7_0		(INT_GIC_BASE + 83)
#define INT_AP_HA7_1		(INT_GIC_BASE + 84)
#define INT_AP_HA7_3		(INT_GIC_BASE + 85)
#define INT_AP_HA7_4		(INT_GIC_BASE + 86)
#define INT_GPIO			(INT_GIC_BASE + 87)
#define INT_COM_I2C			(INT_GIC_BASE + 88)
#define INT_COM_UART		(INT_GIC_BASE + 89)
#define INT_COM_I2S			(INT_GIC_BASE + 90)
#define INT_AP_I2S			(INT_GIC_BASE + 91)
#define INT_COM_PCM			(INT_GIC_BASE + 92)
#define INT_TOP_DMAS		(INT_GIC_BASE + 93)
#define INT_TOP_MAILBOX0	(INT_GIC_BASE + 94)
#define INT_TOP_DMAG		(INT_GIC_BASE + 95)
#define INT_AP_HA7_PMU0		(INT_GIC_BASE + 96)
#define INT_AP_SA7_PMU		(INT_GIC_BASE + 97)
#define INT_AP_WDT0			(INT_GIC_BASE + 98)
#define INT_AP_HA7_PMU1		(INT_GIC_BASE + 99)
#define INT_AP_HA7_PMU2		(INT_GIC_BASE + 100)
#define INT_AP_HA7_PMU3		(INT_GIC_BASE + 101)
#define INT_AP_SA7_AXI		(INT_GIC_BASE + 102)
#define INT_AP_HA7_AXI		(INT_GIC_BASE + 103)
#define INT_CCI400			(INT_GIC_BASE + 104)
#define INT_TOP_MAILBOX1	(INT_GIC_BASE + 105)

#define NR_COMIP_IRQS		(106)

#endif /* __MACH_COMIP_IRQS_H */
