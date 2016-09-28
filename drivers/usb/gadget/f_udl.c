/*
 * drivers/usb/gadget/f_udl.c
 *
 * Use of source code is subject to the terms of the LeadCore license agreement
 * under which you licensed source code. If you did not accept the terms of the
 * license agreement, you are not authorized to use source code. For the terms
 * of the license, please see the license agreement between you and LeadCore.
 *
 * Copyright (c) 2010-2019  LeadCoreTech Corp.
 *
 * PURPOSE:
 *    This file is udl(USB download) gadget driver.
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
#include "comip_udc_otg.h"

#define DEVSPEED    USB_SPEED_HIGH
#define USB_BUFSIZ 256
#define UDL_RX_REQ_NUM	(2)

#define GFP_NONE ((gfp_t) 0)

#include "hexdump.c"

struct udl_dev_stats
{
	unsigned long rx_packets;
	unsigned long tx_packets;
	unsigned long rx_bytes;
	unsigned long tx_bytes;
	unsigned long rx_errors;
	unsigned long tx_errors;
};

struct udl_dev {
	struct usb_gadget *gadget;
	struct usb_request *req;    /*for control response*/

	struct usb_ep *in_ep, *out_ep;
	const struct usb_endpoint_descriptor *in_desc, *out_desc;

	struct usb_request *tx_req;
	struct usb_request *rx_req[UDL_RX_REQ_NUM];
	unsigned int tx_qlen;
	unsigned int rx_req_index;

	struct udl_dev_stats stats;
	volatile unsigned int udl_done;
};

static struct udl_dev l_udldev;
static struct usb_gadget_driver udl_driver;
volatile unsigned packet_received, packet_sent;

static struct usb_device_descriptor device_desc = {
	.bLength =      sizeof device_desc,
	.bDescriptorType =  USB_DT_DEVICE,

	.bcdUSB =       __constant_cpu_to_le16(0x0200),

	.bDeviceClass =     0xff,
	.bDeviceSubClass =  0xff,
	.bDeviceProtocol =  0xff,

	.idVendor =     __constant_cpu_to_le16(0x1ab7),
	.idProduct =    __constant_cpu_to_le16(0x2100),
	.bcdDevice =    __constant_cpu_to_le16(0x0100),
	.iManufacturer = 0x00,
	.iProduct = 0x00,
	.iSerialNumber = 0x00,
	.bNumConfigurations = 1,
};

static struct usb_config_descriptor udl_config = {
	.bLength = sizeof udl_config,
	.bDescriptorType = USB_DT_CONFIG,

	.bNumInterfaces = 0x01,
	.bConfigurationValue =  0x01,
	.iConfiguration =  0x00,
	.bmAttributes = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower = 0xFA,
};

static const struct usb_interface_descriptor udl_intf = {
	.bLength = sizeof udl_intf,       /* length */
	.bDescriptorType = USB_DT_INTERFACE,/* descriptor type */
	.bInterfaceNumber = 0x00,           /* interface number */
	.bAlternateSetting = 0x00,          /* alternate setting */
	.bNumEndpoints = 0x02,              /* num endpoints */
	.bInterfaceClass = 0xff,            /* interface class */
	.bInterfaceSubClass = 0xff,         /* interface sub class */
	.bInterfaceProtocol = 0xff,         /* interface protocol */
	.iInterface = 0x00,                 /* interface */
};

static struct usb_endpoint_descriptor fs_source_desc = {
	.bLength =          USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =  USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes =     USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =   __constant_cpu_to_le16(64),
};

static struct usb_endpoint_descriptor fs_sink_desc = {
	.bLength =          USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =  USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes =     USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =   __constant_cpu_to_le16(64),
};

static struct usb_endpoint_descriptor hs_source_desc = {
	.bLength =          USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =  USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes =     USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =   __constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_sink_desc = {
	.bLength =          USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =  USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes =     USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =   __constant_cpu_to_le16(512),
};

static const struct usb_descriptor_header *hs_udl_function[] = {
	(struct usb_descriptor_header *) &udl_intf,
	(struct usb_descriptor_header *) &hs_sink_desc,
	(struct usb_descriptor_header *) &hs_source_desc,
	NULL,
};

static const struct usb_descriptor_header *fs_udl_function[] = {
	(struct usb_descriptor_header *) &udl_intf,
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

static int
config_buf(struct usb_gadget *g, u8 *buf, u8 type, unsigned index, int is_otg)
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

	config = &udl_config;
	function = which_fn(udl);

	len = usb_gadget_config_buf(config, buf, USB_BUFSIZ, function);
	if (len < 0)
		return len;
	((struct usb_config_descriptor *) buf)->bDescriptorType = type;

	if(function == fs_udl_function)
		printf("use full-speed descriptor!\n");

	//dump_msg(buf,len);
	return len;
}

static void udl_setup_complete(struct usb_ep *ep, struct usb_request *req)
{
	if (req->status || req->actual != req->length)
		debug("%s --> %d, %d/%d\n",__func__,
				req->status, req->actual, req->length);
}

static void udl_unbind(struct usb_gadget *gadget)
{
	/* Not used */
}

static int udl_bind(struct usb_gadget *gadget)
{
	struct udl_dev *dev = &l_udldev;
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
		hs_source_desc.bEndpointAddress =
				fs_source_desc.bEndpointAddress;
		hs_sink_desc.bEndpointAddress =
				fs_sink_desc.bEndpointAddress;
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
	dev->req->complete = udl_setup_complete;
	//control_req=dev->req.buf

	/* finish hookup to lower layer ... */
	dev->gadget = gadget;
	set_gadget_data(gadget, dev);
	gadget->ep0->driver_data = dev;

	return 0;

fail:
	printf("%s failed!\n", __func__);
	udl_unbind(gadget);
	return -1;
}

static int udl_setup(struct usb_gadget *gadget, const struct usb_ctrlrequest *ctrl)
{
	struct udl_dev *dev = get_gadget_data(gadget);
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
			value = min_t(int, w_length,
				sizeof(struct usb_qualifier_descriptor));
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
		/*case USB_DT_STRING:
			value = get_string(cdev, req->buf,
					w_index, w_value & 0xff);
			if (value >= 0)
				value = min(w_length, (u16) value);
			break;*/
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
		dev->udl_done = 1;
		break;
	default:
unknown:
		printf("non-core control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
		break;
	}

	/* respond with data transfer before status phase? */
	if (value >= 0)
	{
		req->length = value;
		req->zero = value < w_length;
		value = usb_ep_queue(gadget->ep0, req, GFP_NONE);
		if (value < 0)
		{
			debug("ep_queue --> %d\n", value);
			req->status = 0;
			udl_setup_complete(gadget->ep0, req);
		}
		if (1 == dev->udl_done)
			dev->udl_done = 2;
	}

	return value;
}

static void udl_disconnect(struct usb_gadget *gadget)
{
	/* Not used */
}

static void udl_suspend(struct usb_gadget *gadget)
{
	/* Not used */
}

static void udl_resume(struct usb_gadget *gadget)
{
	/* Not used */
}

static int alloc_requests(struct udl_dev *dev, gfp_t gfp_flags)
{
	int i;

	dev->tx_req = usb_ep_alloc_request(dev->in_ep, 0);
	if (!dev->tx_req)
		goto fail1;

	for(i=0; i<UDL_RX_REQ_NUM; i++) {
		dev->rx_req[i] = usb_ep_alloc_request(dev->out_ep, 0);
		if (!dev->rx_req[i])
			goto fail2;
	}

	return 0;

fail2:
	for(;i>0;i--)
		usb_ep_free_request(dev->in_ep, dev->rx_req[i-1]);
	usb_ep_free_request(dev->in_ep, dev->tx_req);
fail1:
	printf("can't alloc requests");
	return -1;
}

static void tx_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct udl_dev  *dev = ep->driver_data;

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
	struct udl_dev  *dev = ep->driver_data;

	switch (req->status) {
	/* normal completion */
	case 0:
		dev->stats.rx_packets++;
		dev->stats.rx_bytes += req->actual;
		break;

	/* software-driven interface shutdown */
	case -ECONNRESET:       /* unlink */
	case -ESHUTDOWN:        /* disconnect etc */
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

static int rx_submit(struct udl_dev *dev, struct usb_request *req, gfp_t gfp_flags)
{
	int retval = 0;

	/* Fix me:
	 * Set the req->length to UDC_DMA_BUF_SIZE(32k or 9K), no matter
	 * how many bytes received. Upper layer will parse the rightness
	 * of the data content. */
	req->length = UDC_DMA_BUF_SIZE;


	req->complete = rx_complete;
	retval = usb_ep_queue(dev->out_ep, req, gfp_flags);
	if (retval)
		debug("rx submit --> req(0x%p) %d", req, retval);

	return retval;
}

int usb_udl_init(void)
{
	struct udl_dev *dev = &l_udldev;
	struct usb_gadget *gadget;

	debug("%s +\n",__func__);

	if (usb_gadget_register_driver(&udl_driver) < 0)
		goto fail;

	dev->udl_done = 0;

	gadget = dev->gadget;
	usb_gadget_connect(gadget);

	while (2 != dev->udl_done) { }
	debug("%s -\n",__func__);

	alloc_requests(dev, 0);
	packet_received = 0;
	rx_submit(dev, dev->rx_req[dev->rx_req_index], 0);

	return 0;

fail:
	return -1;
}

int usb_udl_send(void* packet, int length)
{
	struct udl_dev *dev = &l_udldev;
	struct usb_request  *req = dev->tx_req;
	int retval;

	req->length = length;
	memcpy((void *)req->buf,(void *)packet,length);
	req->complete = tx_complete;
	//dump_msg(packet,length);
	packet_sent = 0;

	retval = usb_ep_queue(dev->in_ep, req, GFP_NONE);
	if (!retval)
		debug("%s: packet queued\n", __func__);
	while (!packet_sent)
		;

	return req->actual;
}

int usb_udl_recv(char **packet, int length)
{
	struct udl_dev *dev = &l_udldev;
	struct usb_request  *req = dev->rx_req[dev->rx_req_index];
	int ret = 0;

	while(!packet_received)
		;

	if (length != req->actual){
		/* SML now add extra 3 bytes(00 00 00) to CMD_DATA, the warning always output. */
		//printf("received unexpected size (%d/%d)\n",req->actual,length);
		//dump_msg(packet,64);
	}

	*packet = req->buf;
	ret = req->actual;

	packet_received = 0;
	dev->rx_req_index ^= 0x01;
	if(dev->rx_req_index >= UDL_RX_REQ_NUM) {
			debug("%s rx_req_index error\n",__func__);
	}
	rx_submit(dev, dev->rx_req[dev->rx_req_index], 0);

	return ret;
}

/*
static void usb_udl_halt() {}
*/

static struct usb_gadget_driver udl_driver = {
	.speed      = DEVSPEED,

	.bind       = udl_bind,
	.unbind     = udl_unbind,

	.setup      = udl_setup,
	.disconnect = udl_disconnect,

	.suspend    = udl_suspend,
	.resume     = udl_resume,
};
