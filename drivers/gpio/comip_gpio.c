
#include <common.h>

static unsigned int gpio_bases[] = {
	[0] = GPIO_BANK0_BASE,
#ifdef GPIO_BANK1_BASE
	[1] = GPIO_BANK1_BASE,
#endif
};

static inline unsigned int comip_gpio_to_base(int gpio)
{
	if (gpio >= GPIO_NUM)
		return 0;

	return gpio_bases[gpio_to_bank(gpio)];
}

int gpio_request(int id, const char *label)
{
	return 0;
}

void gpio_free(int id)
{

}

int gpio_direction_input(int id)
{
	unsigned int reg;
	unsigned int bit;
	unsigned int val;

	if (id >= GPIO_NUM)
		return -1;

	reg = comip_gpio_to_base(id) + GPIO_PORT_DDR(id);
	bit = id % 16;

	val = (1 << (bit + 16));
	writel(val, reg);

	return 0;
}

int gpio_direction_output(int id, int value)
{
	unsigned int reg;
	unsigned int bit;
	unsigned int val;

	if (id >= GPIO_NUM)
		return -1;

	/* Set output value. */
	gpio_set_value(id, value);

	/* Set direction. */
	reg = comip_gpio_to_base(id) + GPIO_PORT_DDR(id);
	bit = id % 16;

	val = (1 << (bit + 16)) | (1 << bit);
	writel(val, reg);

	return 0;
}

int gpio_get_value(int id)
{
	unsigned int reg;
	unsigned int bit;
	unsigned int val;

	if (id >= GPIO_NUM)
		return -1;

	reg = comip_gpio_to_base(id) + GPIO_EXT_PORT(id);
	bit = id % 32;

	val = readl(reg) & (1 << bit);

	return !!val;
}

void gpio_set_value(int id, int value)
{
	unsigned int reg;
	unsigned int bit;
	unsigned int val;

	if (id >= GPIO_NUM)
		return;

	reg = comip_gpio_to_base(id) + GPIO_PORT_DR(id);
	bit = id % 16;

	val = (1 << (bit + 16));
	if(value)
		val |= 1 << bit;
	writel(val, reg);
}

void gpio_set_debounce(int id, int en)
{
	unsigned int reg;
	unsigned int bit;
	unsigned int val;

	if (id >= GPIO_NUM)
		return;

	reg = comip_gpio_to_base(id) + GPIO_DEBOUNCE(id);
	bit = id % 16;

	val = (1 << (bit + 16));
	if(en)
		val |= 1 << bit;
	writel(val, reg);
}
