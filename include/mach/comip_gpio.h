#ifndef __COMIP_GPIO_H__
#define __COMIP_GPIO_H__

extern int gpio_request(int id, const char *label);
extern void gpio_free(int id);
extern int gpio_direction_input(int id);
extern int gpio_direction_output(int id, int value);
extern int gpio_get_value(int id);
extern void gpio_set_value(int id, int value);
extern void gpio_set_debounce(int id, int en);

#endif /* __COMIP_GPIO_H__ */
