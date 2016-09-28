#include <common.h>
#include "comip_board_info.h"

/*
 * abs() handles unsigned and signed longs, ints, shorts and chars.  For all
 * input types abs() returns a signed long.
 * abs() should not be used for 64-bit types (s64, u64, long long) - use abs64()
 * for those.
 */
#define abs(x) ({						\
		long ret;					\
		if (sizeof(x) == sizeof(long)) {		\
			long __x = (x);				\
			ret = (__x < 0) ? -__x : __x;		\
		} else {					\
			int __x = (x);				\
			ret = (__x < 0) ? -__x : __x;		\
		}						\
		ret;						\
	})

extern int pmic_get_adc_conversion(int channel_no);

/* *********************************************
 HARD_VER voltage = Vadc* R1552  / (R1552 + R1551)
 Vadc = 2850 mV ;   R1551 = 10K;

 POP P0: R1552 = 10K; voltage = 1425mV;
 POP P1: R1552 = 15K; voltage = 1710mV;
 BGA P0: R1552 = 24K; voltage = 2011mV;
 BGA P1: R1552 = 47K; voltage = 2350mV;
**********************************************/
#define BOARD_VALUE_0   (1567)
#define BOARD_VALUE_1   (1860)
#define BOARD_VALUE_2   (2180)

#define HARDWARE_BOARD_ADC              (3)  // ADCI1

struct lte26007_board_id_info {
        char *name;
        int id;
        int r1551;
        int r1552;
        int delta;
};



static struct lte26007_board_id_info board_id_map[] = {
        /*Board Name,   id,	     R1551, R1552*/
        {"LTE26007M10", LTE26007M10, 10000, 10000},
        {"LTE26007M11", LTE26007M11, 10000, 15000},
        {"LTE26007M20", LTE26007M20, 10000, 24000},
        {"LTE26047M10", LTE26047M10, 10000, 47000},
        {"LTE26047M11", LTE26047M11, 15000, 10000},
        {"LTE26047M20", LTE26047M20, 24000, 10000},
};

int comip_board_info_get(char *name)
{
	int i,volt, r1551, r1552, voltage;
	int Vadc = 2850;//mv
	int min_index = 0;
	volt = pmic_get_adc_conversion(HARDWARE_BOARD_ADC);
	for(i = 0; i < ARRAY_SIZE(board_id_map); i++) {
		r1551 = board_id_map[i].r1551;
		r1552 = board_id_map[i].r1552;
		voltage = Vadc * r1552 / (r1552 + r1551);
		board_id_map[i].delta = abs(volt - voltage);
		if(i > 0 && board_id_map[i].delta < board_id_map[min_index].delta)
			min_index = i;
	}

	if(name) {
		strcpy(name, board_id_map[min_index].name);
	}

	printf("board id:%s\n", board_id_map[min_index].name);

	return board_id_map[min_index].id;
}