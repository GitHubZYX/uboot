#ifndef __COMIP_UDC_OTG_H__
#define __COMIP_UDC_OTG_H__

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/types.h>

#define STAT_IO_BS_H_READY 0x0
#define STAT_IO_BS_DMABUSY 0x1
#define STAT_IO_BS_DMADONE 0x2
#define STAT_IO_BS_H_BUSY  0x3

#define STAT_IO_BS_SHIFT    30
#define STAT_IO_RTS_SUCCESS 0x0
#define STAT_IN_TS_BUFFLUSH 0x1
#define STAT_IO_RTS_BUFERR  0x3
#define STAT_IO_RTS_SHIFT   28
#define STAT_IO_L         (1 << 27) /* Last buffer */
#define STAT_IO_L_SHIFT   (27)      /* Last buffer */
#define STAT_IO_SP        (1 << 26) /* short packaget */
#define STAT_IO_SP_SHIFT  (26)      /* short packaget */
#define STAT_IO_IOC       (1 << 25) /* Interrupt completed */
#define STAT_IO_IOC_SHIFT (25)      /* Interrupt completed */

#define STAT_OUT_NON_ISO_SR   (1 << 24) /* Setup package received */
#define STAT_OUT_NON_ISO_MTRF (1 << 23)
#define STAT_OUT_ISO_PID_0 0x0
#define STAT_OUT_ISO_PID_1 0x1
#define STAT_OUT_ISO_PID_2 0x2
#define STAT_IN_ISO_PID_1  0x1 /* 1 package, start DATA0 */
#define STAT_IN_ISO_PID_2  0x2 /* 2 package, start DATA1 */
#define STAT_IN_ISO_PID_3  0x3 /* 3 package, start DATA2 */
#define STAT_IO_ISO_PID_SHIFT 23
#define STAT_IO_ISO_FN_SHIFT  12 /* Frame number */
#define STAT_IO_NON_ISO_RTB_MASK 0xffff
#define STAT_OUT_ISO_RB_MASK     0x7ff
#define STAT_IN_ISO_TX_MASK      0xfff

#define COMIP_EP_NUM 15


/* ep0 request buf size */
#define REQ_BUFSIZ          (512)

/* hw wait count */
#define MAX_WAIT_COUNT      (5 * 1000)
#define MAX_FLUSH_FIFO_WAIT (5 * 1000)

struct comip_dma_des {
	unsigned int status;
	dma_addr_t buf_addr;
};

struct comip_request {
	struct usb_request      req;
	struct comip_dma_des    dma_desc;
	struct list_head        queue;
};

enum ep0_state {
	EP0_IDLE,
	EP0_IN_DATA_PHASE,
	EP0_OUT_DATA_PHASE,
	EP0_STALL,
	EP0_IN_FAKE,
	EP0_NO_ACTION
};

#define EP0_MPS         ((unsigned)64)
#define BULK_MPS(speed) ((speed) == USB_SPEED_HIGH)? ((unsigned)512) : ((unsigned)64)
#define ISO_MPS(speed)  ((speed) == USB_SPEED_HIGH)? ((unsigned)1024) : ((unsigned)1023)
#define INT_MPS(speed)  ((speed) == USB_SPEED_HIGH)? ((unsigned)1024) : ((unsigned)8)

#define EP0_FIFO_SIZE  ((unsigned)64)
#define BULK_FIFO_SIZE ((unsigned)512)
#define ISO_FIFO_SIZE  ((unsigned)1024)
#define INT_FIFO_SIZE  ((unsigned)100)

#define GET_DMA_BYTES(len, mps) \
	(((len) % (mps) == 0)? (len) : ((len) / (mps) + 1) * (mps))

struct comip_ep {
	struct usb_ep ep;
	struct comip_udc *dev;
	char name[16];
	const struct usb_endpoint_descriptor *desc;
	struct list_head queue;

	u16	fifo_size;
	u16	ep_num;
	u16	ep_type;

	u16	stopped:1;
	u16	dir_in:1;
	u16	assigned:1;

	struct comip_dma_des *pdma_desc;

	u32	irqs;
};

struct comip_udc {
	struct usb_gadget gadget;
	struct usb_gadget_driver *driver;

	/* ep0 */
	struct comip_request *ep0_req;
	int ep0state;

	struct comip_ep eps[COMIP_EP_NUM];

	struct list_head req_free_list;
	int req_max;
};

#ifdef CONFIG_TL_NODDR
#define UDC_MEMORY_POOL (48 * 1024ul)
#define UDC_DMA_BUF_SIZE (1024 * 9ul)
#else
#define UDC_MEMORY_POOL (1024 * 1024ul)
#ifdef CONFIG_TL_USB
#define UDC_DMA_BUF_SIZE (1024 * 64ul)
#else
#define UDC_DMA_BUF_SIZE (1024 * 32ul)
#endif
#endif

#define UDC_REQ_SIZE ((sizeof(struct comip_request) / 4 + 1) * 4)

#define USBOTG_12M_EN      ((1 << 8) | (1 << 0))
#define USBOTG_12M_DISABLE ((1 << 8) | (0 << 0))

#define val_set(dst, bit, width, val)	do { \
	(dst) &= ~(((0x1 << (width)) - 1) << (bit));\
	(dst) |= ((val) << (bit));\
	} while(0)
#define val_get(dst, bit, width) (((dst) & (((0x1 << (width)) - 1) << (bit))) >> (bit))

#endif
