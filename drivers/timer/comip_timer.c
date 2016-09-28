
#include <common.h>

#define TIMER_ID	(0)

/*
 * delay x useconds AND perserve advance timstamp value
 * config timer0 to 26M and thus 26 tick make on usec.
 */
void __udelay(unsigned long usec)
{
	volatile unsigned long val;
	volatile unsigned long val2;
	unsigned long span;

	span = usec * 13;

	val = REG32(COMIP_TIMER_TCV(TIMER_ID));
	if (val > span) {
		val2 = val - span;
		while (val >= val2)
			val = REG32(COMIP_TIMER_TCV(TIMER_ID));
	} else {
		val2 = 0xFFFFFFFF - (span - val);
		while ((val < 0x80000000) || (val >= val2))
			val = REG32(COMIP_TIMER_TCV(TIMER_ID));
	}
}

void __mdelay(ulong ms)
{
	ulong i;

	for (i = 0; i < ms; i++)
		__udelay(1000);
}


/*
 * Get timebase clock frequency (like cpu_clk in Hz)
 */
unsigned long get_tbclk(void)
{
	return (26 * 1024 * 1024);
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On Blackfin it just returns the timer value.
 */
unsigned long long get_ticks(void)
{
	return REG32(COMIP_TIMER_TCV(TIMER_ID));
}

ulong get_timer (ulong base)
{
	return (REG32(COMIP_TIMER_TCV(TIMER_ID)) - base +1);
}

#if defined(CONFIG_DEBUG_TIMER_ID)
unsigned long debug_timer_getus(void)
{
	unsigned long time_past = 0xffffffff;
	unsigned long time_us = 0;

	time_past = time_past - REG32(COMIP_TIMER_TCV(CONFIG_DEBUG_TIMER_ID));
	time_us = time_past/13;

	return time_us;
}

void debug_timer_init(void)
{
	REG32(AP_PWR_TIMER0CLKCTL + CONFIG_DEBUG_TIMER_ID*4) = 0x30000000;
	REG32(COMIP_TIMER_TCR(CONFIG_DEBUG_TIMER_ID)) = 0x4;
	REG32(COMIP_TIMER_TLC(CONFIG_DEBUG_TIMER_ID)) = 0xFFFFFFFF;
	REG32(COMIP_TIMER_TCR(CONFIG_DEBUG_TIMER_ID)) = 0x5;
}
#endif

int timer_init (void)
{
	REG32(AP_PWR_TIMER0CLKCTL) = 0x30000000;
	REG32(COMIP_TIMER_TCR(TIMER_ID)) = 0x4;
	REG32(COMIP_TIMER_TLC(TIMER_ID)) = 0xFFFFFFFF;
	REG32(COMIP_TIMER_TCR(TIMER_ID)) = 0x5;

	return 0;
}
