#ifndef __USBDEBUG_H__
#define __USBDEBUG_H__

#define DD_DBG_EMERG	0 /* system is unusable */
#define DD_DBG_ALERT	1 /* action must be taken immediately */
#define DD_DBG_CRIT		2 /* critical conditions */
#define DD_DBG_ERR		3 /* error conditions */
#define DD_DBG_WARNING	4 /* warning conditions */
#define DD_DBG_NOTICE	5 /* normal but significant condition */
#define DD_DBG_INFO		6 /* informational */
#define DD_DBG_DEBUG	7 /* debug-level messages */
#define DD_DBG_VDEBUG	8 /* verbose debug-level messages */

#define DD_PRINTK_DEFAULT_THRESHOLD DD_DBG_WARNING

enum {
	DUMP_PREFIX_NONE,
	DUMP_PREFIX_ADDRESS,
	DUMP_PREFIX_OFFSET
};


#if ((CONFIG_USB_DEBUG) \
	&&((DD_PRINTK_DEFAULT_THRESHOLD == DD_DBG_VDEBUG) \
		||(DD_PRINTK_DEFAULT_THRESHOLD == DD_DBG_DEBUG)))

#define dump_msg(/* const u8 * */ buf, /* unsigned */ length) do { \
	print_hex_dump("DBG", "", DUMP_PREFIX_OFFSET, \
					16, 1, buf, length, 0); \
} while (0)

#else
#define dump_msg(...)
#endif


#define usb_dprintf	printf
#define GET_TIME()	0

#define USBVDBG(...)	do { \
	if (DD_DBG_VDEBUG <= DD_PRINTK_DEFAULT_THRESHOLD){ \
		usb_dprintf("%4d<%5d>:",__LINE__, \
					GET_TIME()); \
		usb_dprintf(__VA_ARGS__); \
	} \
}while(0)

#define USBDBG(...)		do { \
	if (DD_DBG_DEBUG <= DD_PRINTK_DEFAULT_THRESHOLD){ \
		usb_dprintf("%4d<%5d>:",__LINE__, \
					GET_TIME()); \
		usb_dprintf(__VA_ARGS__); \
	} \
}while(0)

#define USBINFO(...)	do { \
	if (DD_DBG_INFO <= DD_PRINTK_DEFAULT_THRESHOLD){ \
		usb_dprintf("%4d<%5d>:",__LINE__, \
					GET_TIME()); \
		usb_dprintf(__VA_ARGS__); \
	} \
}while(0)

#define USBERR(...)		do { \
	if (DD_DBG_ERR <= DD_PRINTK_DEFAULT_THRESHOLD){ \
		usb_dprintf("%4d<%5d>:",__LINE__, \
					GET_TIME()); \
		usb_dprintf(__VA_ARGS__); \
	} \
}while(0)


#define DIE(con)		do{ \
	if(con) { \
		usb_dprintf("\nDIE:%s:%d\n con \n",__FILE__,__LINE__); \
				while(1); \
	} \
}while(0);

#endif
