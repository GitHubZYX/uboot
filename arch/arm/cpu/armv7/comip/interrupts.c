
#include <common.h>

#define GIC_CPU_CTRL			0x00
#define GIC_CPU_PRIMASK			0x04
#define GIC_CPU_BINPOINT		0x08
#define GIC_CPU_INTACK			0x0c
#define GIC_CPU_EOI			0x10
#define GIC_CPU_RUNNINGPRI		0x14
#define GIC_CPU_HIGHPRI			0x18

#define GIC_DIST_CTRL			0x000
#define GIC_DIST_CTR			0x004
#define GIC_DIST_ENABLE_SET		0x100
#define GIC_DIST_ENABLE_CLEAR		0x180
#define GIC_DIST_PENDING_SET		0x200
#define GIC_DIST_PENDING_CLEAR		0x280
#define GIC_DIST_ACTIVE_BIT		0x300
#define GIC_DIST_PRI			0x400
#define GIC_DIST_TARGET			0x800
#define GIC_DIST_CONFIG			0xc00
#define GIC_DIST_SOFTINT		0xf00

#define GIC_N_IRQS			NR_COMIP_IRQS

struct _irq_handler {
	void                *m_data;
	void (*m_func)( void *data);
};

DECLARE_GLOBAL_DATA_PTR;

static struct _irq_handler IRQ_HANDLER[GIC_N_IRQS];

static void gic_init(void)
{
	unsigned int i;
	unsigned int gic_irqs = GIC_N_IRQS;
	unsigned long dist_base = ICDDCR_BASEADDR;
	unsigned long base = ICCICR_BASEADDR;
	u32 cpumask;
	u32 cpu = 0;

	/******************************************************
	*		gic_dis_init
	*******************************************************/
	cpumask = 1 << cpu;
	cpumask |= cpumask << 8;
	cpumask |= cpumask << 16;

	writel(0, dist_base + GIC_DIST_CTRL);

	/*
	 * Clear all pending interrupts.
	 */
	for (i = 32; i < gic_irqs; i += 32)
		writel(0xffffffff, dist_base + GIC_DIST_PENDING_CLEAR + i * 4 / 32);

	/*
	 * Set all global interrupts to be level triggered, active low.
	 */
	for (i = 32; i < gic_irqs; i += 16)
		writel(0, dist_base + GIC_DIST_CONFIG + i * 4 / 16);

	/*
	 * Set all global interrupts to this CPU only.
	 */
	for (i = 32; i < gic_irqs; i += 4)
		writel(cpumask, dist_base + GIC_DIST_TARGET + i * 4 / 4);

	/*
	 * Set priority on all global interrupts.
	 */
	for (i = 32; i < gic_irqs; i += 4)
		writel(0xa0a0a0a0, dist_base + GIC_DIST_PRI + i * 4 / 4);

	/*
	 * Disable all interrupts.  Leave the PPI and SGIs alone
	 * as these enables are banked registers.
	 */
	for (i = 32; i < gic_irqs; i += 32)
		writel(0xffffffff, dist_base + GIC_DIST_ENABLE_CLEAR + i * 4 / 32);

	writel(1, dist_base + GIC_DIST_CTRL);


	/******************************************************
	*		gic_cpu_init
	*******************************************************/
	/*
	 * Deal with the banked PPI and SGI interrupts - disable all
	 * PPI interrupts, ensure all SGI interrupts are enabled.
	 */
	writel(0xffffffff, dist_base + GIC_DIST_PENDING_CLEAR);
	writel(0xffff0000, dist_base + GIC_DIST_ENABLE_CLEAR);
	writel(0x0000ffff, dist_base + GIC_DIST_ENABLE_SET);

	/*
	 * Set priority on PPI and SGI interrupts
	 */
	for (i = 0; i < 32; i += 4)
		writel(0xa0a0a0a0, dist_base + GIC_DIST_PRI + i * 4 / 4);

	writel(0xf0, base + GIC_CPU_PRIMASK);
	writel(1, base + GIC_CPU_CTRL);
}

static void default_isr(void *data)
{
	printf("default_isr():  called for IRQ %d, Interrupt Status 0x%08x\n",
	       (int)data, readl(ICCICR_BASEADDR + GIC_CPU_INTACK));
}

void enable_irq(unsigned int irq)
{
	if (irq >= GIC_N_IRQS)
		return;

	writel((1 << (irq % 32)), ICDDCR_BASEADDR + GIC_DIST_ENABLE_SET + (irq / 32) * 4);
}

void disable_irq(unsigned int irq)
{
	if (irq >= GIC_N_IRQS)
		return;

	writel((1 << (irq % 32)), ICDDCR_BASEADDR + GIC_DIST_ENABLE_CLEAR + (irq / 32) * 4);
}

void do_irq(struct pt_regs *pt_regs)
{
	int irq = readl(ICCICR_BASEADDR + GIC_CPU_INTACK);

	if (irq < GIC_N_IRQS)
		IRQ_HANDLER[irq].m_func(IRQ_HANDLER[irq].m_data);

	/* Clera interrupt. */
	writel(irq, ICCICR_BASEADDR + GIC_CPU_EOI);
}

void irq_install_handler(int irq, interrupt_handler_t handle_irq, void *data)
{
	if (irq >= GIC_N_IRQS || !handle_irq)
		return;

	IRQ_HANDLER[irq].m_data = data;
	IRQ_HANDLER[irq].m_func = handle_irq;
}

void irq_free_handler(int irq)
{
	if (irq >= GIC_N_IRQS)
		return;

	disable_irq(irq);

	IRQ_HANDLER[irq].m_data = NULL;
	IRQ_HANDLER[irq].m_func = NULL;
}

int arch_interrupt_init(void)
{
	int i;
	unsigned long vector_addr = gd->relocaddr;

	/* Set vector address in CP15 VBAR register */
	__asm__ __volatile__("mcr p15, 0, %0, c12, c0, 0\n"
			     : "=r" (vector_addr)
			     :
			     : "memory");

	/* install default interrupt handlers */
	for (i = 0; i < GIC_N_IRQS; i++)
		irq_install_handler(i, default_isr, (void *)i);

	/* configure interrupts for IRQ mode */
	gic_init();

	return (0);
}

