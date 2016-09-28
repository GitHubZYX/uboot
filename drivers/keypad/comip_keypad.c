
#include <common.h>

#if defined(CONFIG_KEYPAD_GPIO_KEY)
static unsigned int keypad_gpio_get_value(unsigned int keycode)
{
	int val = 0;
	int gpio = KEYCODE_TO_GPIO(keycode);
	int level = KEYCODE_TO_LEVEL(keycode);

	val = gpio_get_value(gpio);
	return (val == level);
}

#elif defined(CONFIG_KEYPAD_MATRIX_KEY)

#define KBS_MTXKEY_AS_REG(row)	(((row) / 4) * 4)
#define KBS_MTXKEY_AS_BIT(row)		(((row) % 4) * 8)
#define KBS_DEBOUNCE_INTVAL_TIME	(5)	/* 1ms/bit. */
#define KBS_DETECT_INTVAL_TIME		(5)	/* 0.5ms/bit. */

static void keypad_bit_writel(unsigned int reg, unsigned int val, unsigned int mask)
{
	unsigned int valo, valn;

	valo = __raw_readl(reg);
	valn = valo & (~mask);
	valn |= val;
	__raw_writel(valn, reg);
}

static void keypad_matrix_init(void)
{
	/* Enable pclk. */
	keypad_bit_writel(AP_PWR_SECPCLK_EN, 1 << 1 | 1 << 17, 1 << 1 | 1 << 17);

	/* Enable mclk. */
	keypad_bit_writel(AP_PWR_SECAPBMCLK_EN, 1 << 0 | 1 << 8, 1 << 0 | 1 << 8);

	/* Set debounce interval. */
	__raw_writel(KBS_DEBOUNCE_INTVAL_TIME, KBS_BASE + KBS_DEBOUNCE_INTVAL);

	/* Set detect interval. */
	__raw_writel(KBS_DETECT_INTVAL_TIME << 8 | KBS_DETECT_INTVAL_TIME,
					KBS_BASE + KBS_DETECT_INTVAL);

	/* mask all row and col. */
	__raw_writel(0xffff, KBS_BASE + KBS_MASK);
}

static void keypad_matrix_config(unsigned int keycode)
{
	unsigned int row = KEYCODE_TO_ROW(keycode);
	unsigned int col = KEYCODE_TO_COL(keycode);
	unsigned int level = KEYCODE_TO_LEVEL(keycode);

	/* Set matrix keys mask. */
	keypad_bit_writel(KBS_BASE + KBS_MASK, 0 << row, 1 << row);
	keypad_bit_writel(KBS_BASE + KBS_MASK, 0 << (col + 8), 1 << (col + 8));

	/* Set matrix pull cfg. */
	keypad_bit_writel(KBS_BASE + KBS_CTL, (!level) << 4, 1 << 4);
}

static int keypad_matrix_scan(unsigned int keycode)
{
	unsigned int val;
	unsigned char col_state = 0;
	unsigned int row = KEYCODE_TO_ROW(keycode);
	unsigned int col = KEYCODE_TO_COL(keycode);

	val = __raw_readl(KBS_BASE + KBS_MTXKEY_ASREG0 + KBS_MTXKEY_AS_REG(row));
	col_state = (val >> (KBS_MTXKEY_AS_BIT(row))) & (1 << col);

	return !!col_state;
}
#endif

int keypad_init(void)
{
	int ret = 0;

#if defined(CONFIG_KEYPAD_GPIO_KEY)
	int pin;

	pin = KEYCODE_TO_GPIO(VOL_UP_KEYCODE);
	comip_mfp_config(pin, MFP_PIN_MODE_GPIO);
	gpio_direction_input(mfp_to_gpio(pin));
#if defined(CONFIG_KEYPAD_GPIO_PULLUP)
	comip_mfp_config_pull(pin, MFP_PULL_UP);
#endif

	pin = KEYCODE_TO_GPIO(VOL_DOWN_KEYCODE);
	comip_mfp_config(pin, MFP_PIN_MODE_GPIO);
	gpio_direction_input(mfp_to_gpio(pin));
#if defined(CONFIG_KEYPAD_GPIO_PULLUP)
	comip_mfp_config_pull(pin, MFP_PULL_UP);
#endif


#if defined(CONFIG_KEYPAD_GPIO_PULLUP)
	/* For rising timing if using internal pullup */
	mdelay(4);
#endif

#elif defined(CONFIG_KEYPAD_MATRIX_KEY)
	keypad_matrix_init();
	keypad_matrix_config(VOL_UP_KEYCODE);
	keypad_matrix_config(VOL_DOWN_KEYCODE);
#endif

	return ret;
}

int keypad_check(void)
{
	int val = 0;
	int ret;

#if defined(CONFIG_KEYPAD_GPIO_KEY)
	/*get gpio input value.*/
	ret = keypad_gpio_get_value(VOL_UP_KEYCODE);
	if (ret)
		val |= KEYPAD_VOL_UP;

	ret = keypad_gpio_get_value(VOL_DOWN_KEYCODE);
	if (ret)
		val |= KEYPAD_VOL_DOWN;
#elif defined(CONFIG_KEYPAD_MATRIX_KEY)
	/*start matrix mode.*/
	keypad_bit_writel(KBS_BASE + KBS_CTL, 0x03, 0x03);
	mdelay(10);

	/*get matrix input value.*/
	ret = keypad_matrix_scan(VOL_UP_KEYCODE);
	if (ret)
		val |= KEYPAD_VOL_UP;

	ret = keypad_matrix_scan(VOL_DOWN_KEYCODE);
	if (ret)
		val |= KEYPAD_VOL_DOWN;

	/*turn off matrix mode.*/
	__raw_writel(0, KBS_BASE + KBS_CTL);
#endif

	return val;
}

