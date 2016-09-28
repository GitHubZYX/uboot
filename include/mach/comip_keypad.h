#ifndef __COMIP_KEYPAD_H__
#define __COMIP_KEYPAD_H__

/* Keys. */
#define KEYPAD_VOL_UP		(1 << 0)
#define KEYPAD_VOL_DOWN		(1 << 1)
#define KEYPAD_VOL_HOME		(1 << 2)
#define KEYPAD_VOL_BACK		(1 << 3)
#define KEYPAD_VOL_MENU		(1 << 4)

/* Key code. */
#define KEYPAD_MATRIX_KEYCODE(row, col, level)		(((row) << 16) | ((col) << 8) | (level))
#define KEYPAD_GPIO_KEYCODE(id, level)		(((id) << 16) | (level))

/* Key parameter. */
#define KEYCODE_TO_ROW(keycode)		(keycode >> 16)
#define KEYCODE_TO_COL(keycode)		((keycode >> 8) & 0x000000ff)
#define KEYCODE_TO_GPIO(keycode)		(keycode >> 16)
#define KEYCODE_TO_LEVEL(keycode)	(keycode & 0x000000ff)

extern int keypad_init(void);
extern int keypad_check(void);

#endif /* __COMIP_KEYPAD_H__ */
