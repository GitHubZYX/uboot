#ifndef __LEDS_H
#define __LEDS_H

enum led_brightness {
	LED_OFF		= 0,
	LED_HALF	= 127,
	LED_FULL	= 255,
};

enum led_colors {
	RED = 0,
	GREEN,
	BLUE,
	COLOR_MAX,
};

#if defined(CONFIG_LEDS_AW2013)
extern int led_set_blink(enum led_colors color,
		unsigned long delay_on, unsigned long delay_off);
extern int led_set_brightness(enum led_colors color,
		enum led_brightness brightness);
#else
static inline int led_set_blink(enum led_colors color,
		unsigned long delay_on, unsigned long delay_off){return 0;}
static inline int led_set_brightness(enum led_colors color,
		enum led_brightness brightness){return 0;}
#endif

#endif /* __LEDS_H */
