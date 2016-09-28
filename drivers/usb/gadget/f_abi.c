/*
 * drivers/usb/gadget/f_abi.c
 *
 * Use of source code is subject to the terms of the LeadCore license agreement
 * under which you licensed source code. If you did not accept the terms of the
 * license agreement, you are not authorized to use source code. For the terms
 * of the license, please see the license agreement between you and LeadCore.
 *
 * Copyright (c) 2010-2019  LeadCoreTech Corp.
 *
 * PURPOSE:
 *    This file is abi(Android Bootloader Interface) gadget driver.
 *
 * CHANGE HISTORY:
 *    Version  Date           Author         Description
 *    1.0.0    2013-09-02     pengyimin      created
 */

#include <common.h>
#include <asm/errno.h>
#include <linux/usb/gadget.h>
#include <linux/usb/ch9.h>

#include "usbdebug.h"

#define DEVSPEED    USB_SPEED_HIGH
#define USB_BUFSIZ 256

#define GFP_NONE ((gfp_t) 0)

#include "hexdump.c"

#define FASTBOOT_VID     0x18D1
#define FASTBOOT_PID     0x4EE0

#define STRING_MANUFACTURER 0x01
#define STRING_PRODUCT      0x02
#define STRING_SERIALNUMBER 0x03

struct abi_dev_stats
{
	unsigned long rx_packets;
	unsigned long tx_packets;
	unsigned long rx_bytes;
	unsigned long tx_bytes;
	unsigned long rx_errors;
	unsigned long tx_errors;
};

struct abi_dev {
	struct usb_gadget *gadget;
	struct usb_request *req; /*for control response*/

	struct usb_ep *in_ep, *out_ep;
	const struct usb_endpoint_descriptor *in_desc, *out_desc;

	struct usb_request *tx_req, *rx_req;
	unsigned int tx_qlen;

	struct abi_dev_stats stats;
	volatile unsigned int abi_done;
};

DECLARE_GLOBAL_DATA_PTR;
static struct abi_dev l_abidev;
static struct usb_gadget_driver abi_driver;
volatile unsigned int packet_received, packet_sent;

static struct usb_device_descriptor device_desc = {
	.bLength =      sizeof device_desc,
	.bDescriptorType =  USB_DT_DEVICE,

	.bcdUSB =       __constant_cpu_to_le16(0x0200),

	.bDeviceClass =     0x0,
	.bDeviceSubClass =  0x0,
	.bDeviceProtocol =  0x0,

	.idVendor =     __constant_cpu_to_le16(FASTBOOT_VID),
	.idProduct =    __constant_cpu_to_le16(FASTBOOT_PID),
	.iManufacturer = STRING_MANUFACTURER,
	.iProduct = STRING_PRODUCT,
	.iSerialNumber = STRING_SERIALNUMBER,
	.bNumConfigurations = 1,
};

static struct usb_config_descriptor abi_config = {
	.bLength = sizeof abi_config,
	.bDescriptorType = USB_DT_CONFIG,
	.wTotalLength = 0x20,

	.bNumInterfaces = 0x01,
	.bConfigurationValue =  0x01,
	.iConfiguration =  0x00,
	.bmAttributes = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower = 0xFA,
};

static const struct usb_interface_descriptor abi_intf = {
	.bLength = sizeof abi_intf,         /* length */
	.bDescriptorType = USB_DT_INTERFACE,/* descriptor type */
	.bInterfaceNumber = 0x00,           /* interface number */
	.bAlternateSetting = 0x00,          /* alternate setting */
	.bNumEndpoints = 0x02,              /* num endpoints */
	.bInterfaceClass = 0xff,            /* interface class */
	.bInterfaceSubClass = 0x42,         /* interface sub class */
	.bInterfaceProtocol = 0x03,         /* interface protocol */
	.iInterface = 0x00,                 /* interface */
};

static struct usb_endpoint_descriptor fs_source_desc = {
	.bLength =         USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes =     USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_sink_desc = {
	.bLength =         USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes =     USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor hs_source_desc = {
	.bLength =         USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bmAttributes =   USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = __constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_sink_desc = {
	.bLength =         USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bmAttributes =   USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = __constant_cpu_to_le16(512),
};

static const struct usb_descriptor_header *hs_abi_function[] = {
	(struct usb_descriptor_header *) &abi_intf,
	(struct usb_descriptor_header *) &hs_sink_desc,
	(struct usb_descriptor_header *) &hs_source_desc,
	NULL,
};
/* static strings, in UTF-8 */
static struct usb_string strings[] = {
	{ STRING_MANUFACTURER,  "LeadcoreTech", },
	{ STRING_PRODUCT, "Fastboot", },
	{ STRING_SERIALNUMBER,  "comip-lc18xx", },
	{ }     /* end of list */
};

static struct usb_gadget_strings stringtab = {
	.language   = 0x0409,   /* en-us */
	.strings = strings,
};

static const struct usb_descriptor_header *fs_abi_function[] = {
	(struct usb_descriptor_header *) &abi_intf,
	(struct usb_descriptor_header *) &fs_sink_desc,
	(struct usb_descriptor_header *) &fs_source_desc,
	NULL,
};

static inline struct usb_endpoint_descriptor *
ep_desc(struct usb_gadget *g, struct usb_endpoint_descriptor *hs,
        struct usb_endpoint_descriptor *fs)
{
	if (gadget_is_dualspeed(g) && g->speed == USB_SPEED_HIGH)
		return hs;
	return fs;
}

size_t strlcpy(char *dest, const char *src, size_t size)
{
	size_t ret = strlen(src);

	if (size) {
		size_t len = (ret >= size) ? size - 1 : ret;
		memcpy(dest, src, len);
		dest[len] = '\0';
	}
	return ret;
}

static int config_buf(struct usb_gadget *g, u8 *buf, u8 type, unsigned index, int is_otg)
{
	int len;
	const struct usb_config_descriptor  *config;
	const struct usb_descriptor_header  **function;
	int hs = 0;

	if (gadget_is_dualspeed(g)) {
		hs = (g->speed == USB_SPEED_HIGH);
		if (type == USB_DT_OTHER_SPEED_CONFIG)
			hs = !hs;
	}
#define which_fn(t) (hs ? hs_ ## t ## _function : fs_ ## t ## _function)

	if (index >= device_desc.bNumConfigurations)
		return -EINVAL;

	config = &abi_config;
	function = which_fn(abi);
	debug("%s, NOTE: %p ?= %p\n",__func__,function,hs_abi_function);

	len = usb_gadget_config_buf(config, buf, USB_BUFSIZ, function);
	if (len < 0)
		return len;

	((struct usb_config_descriptor *) buf)->bDescriptorType = type;
	return len;
}

static void abi_setup_complete(struct usb_ep *ep, struct usb_request *req)
{
	if (req->status || req->actual != req->length)
		debug("%s --> %d, %d/%d\n",__func__,
				req->status, req->actual, req->length);
}

static void abi_unbind(struct usb_gadget *gadget)
{
	/* Not used */
}

static int abi_bind(struct usb_gadget *gadget)
{
	struct abi_dev *dev = &l_abidev;
	struct usb_ep *in_ep = NULL;
	struct usb_ep *out_ep = NULL;

	/* all we really need is bulk IN/OUT */
	usb_ep_autoconfig_reset(gadget);
	in_ep = usb_ep_autoconfig(gadget, &fs_source_desc);
	if (!in_ep) {
autoconf_fail:
		error("can't autoconfigure on %s\n", gadget->name);
		return -ENODEV;
	}
	in_ep->driver_data = dev; /* claim */
	dev->in_desc = ep_desc(gadget, &hs_source_desc, &fs_source_desc);

	out_ep = usb_ep_autoconfig(gadget, &fs_sink_desc);
	if (!out_ep)
		goto autoconf_fail;
	out_ep->driver_data = dev;/* claim */
	dev->out_desc = ep_desc(gadget, &hs_sink_desc, &fs_sink_desc);

	/* usb_ep_autoconfig will reset the wMaxPacketSize to 0. */
	if (!fs_sink_desc.wMaxPacketSize)
		fs_sink_desc.wMaxPacketSize = __constant_cpu_to_le16(64);
	if (!fs_source_desc.wMaxPacketSize)
		fs_source_desc.wMaxPacketSize = __constant_cpu_to_le16(64);

	device_desc.bMaxPacketSize0 = gadget->ep0->maxpacket;
	usb_gadget_set_selfpowered(gadget);

	if (gadget_is_dualspeed(gadget)) {
		/* and that all endpoints are dual-speed */
		hs_source_desc.bEndpointAddress = fs_source_desc.bEndpointAddress;
		hs_sink_desc.bEndpointAddress = fs_sink_desc.bEndpointAddress;
	}

	dev->in_ep = in_ep;
	dev->out_ep = out_ep;

	/* preallocate control message data and buffer */
	dev->req = usb_ep_alloc_request(gadget->ep0, GFP_NONE);
	if (!dev->req)
		goto fail;

	/*initilize usb_request */
	dev->req->zero = 0;
	dev->req->length = 0;
	dev->req->complete = abi_setup_complete;
	//control_req=dev->req.buf

	/* finish hookup to lower layer ... */
	dev->gadget = gadget;
	set_gadget_data(gadget, dev);
	gadget->ep0->driver_data = dev;

	return 0;
fail:
	debug("%s failed!\n", __func__);
	abi_unbind(gadget);
	return -1;
}

static int abi_setup(struct usb_gadget *gadget, const struct usb_ctrlrequest *ctrl)
{
	struct abi_dev *dev = get_gadget_data(gadget);
	struct usb_request *req = dev->req;
	int value = -EOPNOTSUPP;
	u16 w_index = le16_to_cpu(ctrl->wIndex);
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16 w_length = le16_to_cpu(ctrl->wLength);

	/* partial re-init of the response message; the function or the
	 * gadget might need to intercept e.g. a control-OUT completion
	 * when we delegate to it.
	 */
	switch (ctrl->bRequest) {

	/* we handle all standard USB descriptors */
	case USB_REQ_GET_DESCRIPTOR:
		if (ctrl->bRequestType != USB_DIR_IN)
			goto unknown;
		switch (w_value >> 8) {

		case USB_DT_DEVICE:
			value = min(w_length, (u16) sizeof(device_desc));
			if (gadget->speed == USB_SPEED_HIGH) {
				device_desc.bcdUSB =  __constant_cpu_to_le16(0x0200);
			} else {
				device_desc.bcdUSB =  __constant_cpu_to_le16(0x0110);
			}
			memcpy((void *)req->buf, (void *)&device_desc, value);
			break;
		/*case USB_DT_DEVICE_QUALIFIER:
			if (!gadget_is_dualspeed(gadget))
				break;
			device_qual(cdev);
			value = min_t(int, w_length, sizeof(struct usb_qualifier_descriptor));
			break;
		case USB_DT_OTHER_SPEED_CONFIG:
			if (!gadget_is_dualspeed(gadget))
				break;*/
			/* FALLTHROUGH */
		case USB_DT_CONFIG:
			value = config_buf(gadget, req->buf,
								w_value >> 8,
								w_value & 0xff,gadget_is_otg(gadget));
			if (value >= 0)
				value = min(w_length, (u16) value);
			break;
		case USB_DT_STRING:
			value = usb_gadget_get_string(&stringtab,
										w_value & 0x0ff, req->buf);
			if (value >= 0)
				value = min(w_length, (u16) value);
			break;
		default:
			goto unknown;
			break;
		}
		break;

	/* any number of configs can work */
	case USB_REQ_SET_CONFIGURATION:
		if (ctrl->bRequestType != 0)
		goto unknown;
		value = 0;
		usb_ep_enable(dev->in_ep, dev->in_desc);
		usb_ep_enable(dev->out_ep,dev->out_desc);
		dev->abi_done = 1;
		break;
	default:
unknown:
		printf("non-core control req%02x.%02x v%04x i%04x l%d\n",
				ctrl->bRequestType, ctrl->bRequest,
				w_value, w_index, w_length);
		break;
	}

	/* respond with data transfer before status phase? */
	if (value >= 0) {
		req->length = value;
		req->zero = value < w_length;
		debug("%s -> usb_ep_queue\n",__func__);
		value = usb_ep_queue(gadget->ep0, req, GFP_NONE);
		if (value < 0) {
			debug("ep_queue --> %d\n", value);
			req->status = 0;
			abi_setup_complete(gadget->ep0, req);
		}
		if (1 == dev->abi_done)
			dev->abi_done = 2;
	}

	return value;
}

static void abi_disconnect(struct usb_gadget *gadget)
{
	/* Not used */
}

static void abi_suspend(struct usb_gadget *gadget)
{
	/* Not used */
}

static void abi_resume(struct usb_gadget *gadget)
{
	/* Not used */
}

static int alloc_requests(struct abi_dev *dev, gfp_t gfp_flags)
{
	dev->tx_req = usb_ep_alloc_request(dev->in_ep, 0);
	if (!dev->tx_req)
		goto fail1;

	dev->rx_req = usb_ep_alloc_request(dev->out_ep, 0);
	if (!dev->rx_req)
		goto fail2;

	return 0;

fail2:
	usb_ep_free_request(dev->in_ep, dev->tx_req);
fail1:
	debug("can't alloc requests");
	return -1;
}

static void tx_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct abi_dev  *dev = ep->driver_data;
	debug("%s: status %s\n", __func__, (req->status) ? "failed" : "ok");
	switch (req->status) {
	default:
		dev->stats.tx_errors++;
		debug("tx err %d\n", req->status);
		/* FALLTHROUGH */
	case -ECONNRESET:       /* unlink */
	case -ESHUTDOWN:        /* disconnect etc */
		break;
	case 0:
		dev->stats.tx_bytes += req->length;
	}
	dev->stats.tx_packets++;

	packet_sent = 1;
}

static void rx_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct abi_dev  *dev = ep->driver_data;

	debug("%s: status %d\n", __func__, req->status);
	switch (req->status) {
	/* normal completion */
	case 0:
		dev->stats.rx_packets++;
		dev->stats.rx_bytes += req->length;
		break;

	/* software-driven interface shutdown */
	case -ECONNRESET:       /* unlink */
	case -ESHUTDOWN:        /* disconnect etc */
	/* for hardware automagic (such as pxa) */
	case -ECONNABORTED:     /* endpoint reset */
			break;

	/* data overrun */
	case -EOVERFLOW:
		/* FALLTHROUGH */
	default:
		dev->stats.rx_errors++;
		break;
	}

	packet_received = 1;
}

int usb_abi_init(void)
{
	unsigned char index = 0;
	struct usb_string	*s = NULL;
	struct abi_dev *dev = &l_abidev;
	struct usb_gadget *gadget;

	debug("%s +\n",__func__);

	for (index = 0 ;  index < sizeof(strings)/sizeof(strings[0]) ; index++) {
		s = &strings[index];

		if (s->id == STRING_SERIALNUMBER) {
			s->s = (char *)gd->fastboot_device_id;
		}
	}

	if (usb_gadget_register_driver(&abi_driver) < 0) {
		printf("usb_gadget_register_driver failed\n");
		return -1;
	}

	dev->abi_done = 0;

	gadget = dev->gadget;
	if (usb_gadget_connect(gadget)) {
		printf("usb_gadget_connect failed\n");
		return -1;
	} else {
		printf("plug-in usb cable and continue ...\n");
	}

	while (2 != dev->abi_done)
		;

	debug("%s -\n",__func__);

	alloc_requests(dev, 0);
	packet_received = 0;

	return 0;
}

int usb_abi_send(void* packet, int length)
{
	struct abi_dev *dev = &l_abidev;
	struct usb_request  *req = dev->tx_req;
	int retval;

	req->length = length;
	memcpy((void *)req->buf,(void *)packet,length);
	req->complete = tx_complete;
	//dump_msg(packet,length);
	packet_sent = 0;

	//debug("%s -> usb_ep_queue\n",__func__);
	retval = usb_ep_queue(dev->in_ep, req, GFP_NONE);
	if (!retval)
		debug("%s: packet queued\n", __func__);

	while (!packet_sent)
		;

	return req->actual;
}

#define USB_DMA_BUFFER_LENGTH   (32*1024ul)
int usb_abi_recv(void * packet, int length)
{
	int ret=0;
	struct abi_dev *dev = &l_abidev;
	struct usb_request  *req = dev->rx_req;
	if(length>USB_DMA_BUFFER_LENGTH){
		printf("length>USB_DMA_BUFFER_LENGTH\n");
		return -1;
	}

	req->length = length;
	req->complete = rx_complete;
	ret = usb_ep_queue(dev->out_ep, req, 0);
	if (ret)
		debug("rx submit --> req(0x%p) %d", req, ret);

	while(!packet_received)
		;

	debug("%s: packet received 0x%x\n", __func__,req->actual);

	memcpy(packet,req->buf,req->actual);
	//dump_msg(packet,length);
	packet_received = 0;
	ret = req->actual;

	return ret;
}

/*
static void usb_abi_halt(void){}
*/

static struct usb_gadget_driver abi_driver = {
	.speed      = DEVSPEED,

	.bind       = abi_bind,
	.unbind     = abi_unbind,

	.setup      = abi_setup,
	.disconnect = abi_disconnect,

	.suspend    = abi_suspend,
	.resume     = abi_resume,
};
