
#include <stdarg.h>
#include <common.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <errno.h>

#if defined(CONFIG_DEBUG_TIMER_ID)
DECLARE_GLOBAL_DATA_PTR;
#endif

void putc(const char c)
{
	serial_putc(c);
}

void puts(const char *s)
{
	serial_puts(s);
}

int getc(void)
{
	return serial_getc();
}

int tstc(void)
{
	return serial_tstc();
}

int ctrlc(void)
{
	if (tstc()) {
		switch (getc()) {
		case 0x03:		/* ^C - Control C */
			return 1;
		default:
			break;
		}
	}

	return 0;
}

int vprintf(const char *fmt, va_list args)
{
	uint i;
	char printbuffer[CONFIG_SYS_PBSIZE];

	/* For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf(printbuffer, fmt, args);

	/* Print the string */
	puts(printbuffer);
	return i;
}

int printf(const char *fmt, ...)
{
	va_list args;
	uint i;
	char printbuffer[CONFIG_SYS_PBSIZE];

#if defined(CONFIG_DEBUG_TIMER_ID)
	unsigned long current_time = debug_timer_getus();
	sprintf(printbuffer, "[%8ld]{%8ld}", current_time,
		current_time - gd->showtime);
	gd->showtime = current_time;
	puts(printbuffer);
#endif

	va_start(args, fmt);

	/* For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf(printbuffer, fmt, args);
	va_end(args);

	/* Print the string */
	puts(printbuffer);
	return i;
}

