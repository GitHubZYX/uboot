#ifndef _TL420_H_
#define _TL420_H_

#define TL420_DUMP_DEBUG		(0x01 << 0)
#define TL420_DUMP_ENABLE		(0x01 << 1)
#define TL420_DUMP_FORCE		(0x01 << 2)
#define TL420_PRINT_ENABLE		(0x01 << 4)
#define TL420_GPIO_DBG_ENABLE		(0x01 << 5)
#define TL420_OSCEN_SHOW_ENABLE		(0x01 << 6)
#define TL420_MEMCTL_CKE_FIX		(0x01 << 8)
#define TL420_MEMCTL_DQS_EN		(0x01 << 16)
#define TL420_MEMCTL_LP_EN		(0x01 << 17)
#define TL420_MEMCTL_PD_EN		(0x01 << 18)
#define TL420_MEMCTL_USE_0		(0x01 << 24)
#define TL420_MEMCTL_USE_1		(0x01 << 31)

enum {
	DUMP_SERIAL_DEBUG = 0,
	DUMP_USB_WRITE,
	DUMP_EMMC_WRITE,
	DUMP_TYPE_MAX
};

struct tl420_args {
	int          sleep_gpio;
	int          debug_gpio;
	int          debug_gpio_ddr;
	unsigned int bb_id;
	unsigned int flags;
	unsigned int dump_type;
	unsigned int gpio_sleep_status;
	unsigned int memctl_para_size;
	unsigned int memctl_para_addr;
	unsigned int memctl0_dqs_size;
	unsigned int memctl0_dqs_addr;
	unsigned int memctl1_dqs_size;
	unsigned int memctl1_dqs_addr;
	unsigned int memctl0_lp_size;
	unsigned int memctl0_lp_addr;
	unsigned int memctl1_lp_size;
	unsigned int memctl1_lp_addr;
};

#endif /* _TL420_H_ */

