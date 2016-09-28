#include <common.h>
#include "comip_board_info.h"

DECLARE_GLOBAL_DATA_PTR;

#include "memctl_multi_ddr.h"

extern struct memctl_data *memctl_init_datas;
extern const unsigned int memctl_init_datas_length;

#if CONFIG_SAMSUNG_KMR310001M
extern struct memctl_multi_ddr_struct memctl_multi_data_KMR310001M;
#endif

#if CONFIG_HYNIX_H9TQ65A8GTMCUR
extern struct memctl_multi_ddr_struct memctl_multi_data_H9TQ65A8GTMCUR;
extern struct memctl_multi_ddr_struct memctl_multi_data_H9TQ65A8GTMCUR2;
#endif

#if CONFIG_MICRON_MT29TZZZ5D6YKFAH
extern struct memctl_multi_ddr_struct memctl_multi_data_MT29TZZZ5D6YKFAH;
extern struct memctl_multi_ddr_struct memctl_multi_data_MT29TZZZ5D6YKFAH2;
extern struct memctl_multi_ddr_struct memctl_multi_data_H9TQ17ABJTMCUR;
extern struct memctl_multi_ddr_struct memctl_multi_data_H9TQ26ABJTMCUR;
#endif


#define MICRON_MT29TZZZ8D4BKFRL (0xfe014e50)  //"Micron MT29TZZZ8D4BKFRL-125 W.94K" //1GB-2CS
#define MEMCTL_MULTI_DATA_END	((void*)0xFFFFFFFF)
struct memctl_multi_ddr_struct *memctl_multi_data[]=
{
#if CONFIG_SAMSUNG_KMR310001M
	&memctl_multi_data_KMR310001M,
#endif
#if CONFIG_HYNIX_H9TQ65A8GTMCUR
	&memctl_multi_data_H9TQ65A8GTMCUR,
	&memctl_multi_data_H9TQ65A8GTMCUR2,
#endif
#if CONFIG_MICRON_MT29TZZZ5D6YKFAH
	&memctl_multi_data_MT29TZZZ5D6YKFAH,
	&memctl_multi_data_MT29TZZZ5D6YKFAH2,
	&memctl_multi_data_H9TQ17ABJTMCUR,
	&memctl_multi_data_H9TQ26ABJTMCUR,
#endif
	MEMCTL_MULTI_DATA_END
};

extern int flash_get_devinfo(unsigned int *mid, unsigned int *did);
struct memctl_multi_ddr_struct* memctl_multi_ddr_get_data(void)
{
	int i;
	unsigned int mid = 0;
	unsigned int did = 0;
	if (flash_get_devinfo(&mid, &did) < 0) {
		printf("get flash vendor info error!\n");
		return NULL;
	}
	printf("flash_get_devinfo:mid:0x%x ,did:0x%x\n",mid,did);
	for (i=0; MEMCTL_MULTI_DATA_END!=memctl_multi_data[i]; i++) {
		if ((mid == memctl_multi_data[i]->mid)
			&& (did == memctl_multi_data[i]->did)) {

			if (memctl_init_datas_length != memctl_multi_data[i]->length) {
				printf("ddr data length error! expect:%d but real:%d\n",
					memctl_init_datas_length,
					memctl_multi_data[i]->length);
				hang();
			}

			printf("use %s\n", memctl_multi_data[i]->name);
			return memctl_multi_data[i];
		}
	}

	return NULL;
}

#if defined(CONFIG_GATE_TRAINING_SUPPORT)
int memctl_check_need_gate_training(void)
{
	gd->need_gate_training = 1;
	debug("need gate_training!\n");

	return gd->need_gate_training;
}
#endif

