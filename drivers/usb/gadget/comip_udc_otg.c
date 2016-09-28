/*
 * drivers/usb/gadget/comip_udc_otg.c
 *
 * Use of source code is subject to the terms of the LeadCore license agreement
 * under which you licensed source code. If you did not accept the terms of the
 * license agreement, you are not authorized to use source code. For the terms
 * of the license, please see the license agreement between you and LeadCore.
 *
 * Copyright (c) 2010-2019  LeadCoreTech Corp.
 *
 * PURPOSE:
 *    This file is for the comip udc controller driver.
 *
 * CHANGE HISTORY:
 *    Version  Date           Author         Description
 *    1.0.0    2013-09-02     pengyimin      created
 *    1.0.1    2014-01-02     pengyimin      refine the file.
 */

#include <common.h>
#include <asm/errno.h>
#include <linux/list.h>
#include <malloc.h>
#include <linux/types.h>

#include "comip_udc_otg.h"
#include "usbdebug.h"

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_CPU_LC1860
#define AP_PWR_USBCLK_EN AP_PWR_USBCLKDIV_CTL
#define CTL_USB_OTG_PHY_SUSPEND CTL_OTGPHY_SUSPENDM_CTRL
#define CTL_USB_OTG_PHY_RST_CTRL CTL_POR_OTGPHY_CTRL
#endif

static struct usb_endpoint_descriptor ep0_out_desc = {
	.bLength = sizeof(struct usb_endpoint_descriptor),
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_XFER_CONTROL,
};

/*
static struct usb_endpoint_descriptor ep0_in_desc = {
	.bLength = sizeof(struct usb_endpoint_descriptor),
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_CONTROL,
	.wMaxPacketSize =  64,
};
*/

static int comip_pullup(struct usb_gadget *gadget, int is_on);
static s32 comip_ep_enable(struct usb_ep *_ep, const struct usb_endpoint_descriptor *desc);
static s32 comip_ep_disable(struct usb_ep *_ep);
static s32 comip_ep_queue(struct usb_ep *_ep, struct usb_request *_req, gfp_t gfp_flags);
static struct usb_request * comip_ep_alloc_request(struct usb_ep *_ep, gfp_t gfp_flags);
static void comip_ep_free_request(struct usb_ep *_ep, struct usb_request *_req);

static void comip_ep_in_start(struct comip_ep *ep, struct comip_request *req, int zero);
static int comip_ep_out_start(struct comip_ep *ep, struct comip_request *req);
static void comip_handle_ep(struct comip_ep *ep, u32 epintsts);
static void comip_handle_ep0(struct comip_udc *dev);

static void comip_udc_setaddress(u16 address);
static void comip_udc_setup_complete(struct usb_ep *_ep, struct usb_request *req);
static void comip_udc_change_mps(enum usb_device_speed speed);

static int comip_udc_wait_dma_done(struct comip_ep *ep,struct comip_dma_des *dma_des);
static void comip_udc_rx_dma_handle(struct comip_ep *ep, struct comip_dma_des *dma_des, struct comip_request *req);

static void usb_ep_int_mask(u8 ep_num, u8 mask);

static void done(struct comip_ep *ep, struct comip_request *req, s32 status);

static struct usb_gadget_ops comip_udc_ops = {
	.pullup = comip_pullup,
};

static struct usb_ep_ops comip_ep_ops = {
	.enable = comip_ep_enable,
	.disable = comip_ep_disable,
	.queue = comip_ep_queue,
	.alloc_request = comip_ep_alloc_request,
	.free_request = comip_ep_free_request,
};

static struct comip_udc controller = {
	.gadget = {
		.name = "comip_udc",
	}
};

static inline void comip_udc_write_reg32(u32 value, u32 paddr)
{
	REG32(paddr) = value;
}

static inline unsigned int comip_udc_read_reg32(u32 paddr)
{
	return REG32(paddr);
}

static s32 comip_ep_disable(struct usb_ep *_ep)
{
	struct comip_ep *ep;

	ep = container_of(_ep, struct comip_ep, ep);
	if (!_ep || !ep->desc)
		return -EINVAL;
	ep->assigned=0;
	return 0;
}

static struct usb_request * comip_ep_alloc_request(struct usb_ep *_ep, gfp_t gfp_flags)
{
	struct comip_udc *dev = &controller;
	struct comip_request * req = NULL;

	if (!list_empty(&dev->req_free_list)) {
		req = list_first_entry(&dev->req_free_list,
			struct comip_request, queue);
		list_del(&req->queue);
	}
	/* can't memset comip_request for dma_desc is fixed in u-boot. */
	memset(&req->req, 0, sizeof(req->req));
	INIT_LIST_HEAD(&req->queue);

	/* No MMU:
	 * Both usb_request(.buf, dma) and comip_request(dma_desc.buf_addr)
	 * are same value. */
	req->req.buf = (void *)(req->dma_desc.buf_addr);
	req->req.dma = req->dma_desc.buf_addr;
	USBDBG("%s: %s get req %p\n",__func__,_ep->name,req);

	return &req->req;
}

static void comip_ep_free_request(struct usb_ep *_ep, struct usb_request *_req)
{
	struct comip_ep *ep = container_of(_ep, struct comip_ep, ep);
	struct comip_udc *dev = &controller;
	struct comip_request * req = NULL;

	list_for_each_entry(req,&ep->queue,queue) {
		list_del(&req->queue);
		list_add_tail(&req->queue, &dev->req_free_list);
	}

	USBDBG("put req:%p\n", req);
}

static s32 comip_ep_queue(struct usb_ep *_ep, struct usb_request *_req, gfp_t gfp_flags)
{
	struct comip_ep *ep = container_of(_ep, struct comip_ep, ep);
	struct comip_request *req = container_of(_req, struct comip_request, req);
	struct comip_udc *dev = ep->dev;

	usb_ep_int_mask(ep->ep_num, 1);

	_req->status = -EINPROGRESS;
	_req->actual = 0;

	if (list_empty(&ep->queue) && (!ep->stopped)) {
		USBDBG("EP%d,ep0state:%d,ep->dir_in:%d req %p\n",
				ep->ep_num, dev->ep0state, ep->dir_in,req);
		if (ep->ep_num == 0) {
			switch (dev->ep0state) {
			case EP0_IN_DATA_PHASE:
				comip_ep_in_start(ep, req, 0);
				break;
			case EP0_NO_ACTION:
				dev->ep0state = EP0_IDLE;
				req = 0;
				break;
			case EP0_IN_FAKE:
				dev->ep0state = EP0_IDLE;
				USBERR("EP0_IN_FAKE???");
				done(ep,req,0);
				req = 0;
				break;
			case EP0_IDLE:
			case EP0_OUT_DATA_PHASE:
			default:
				_req->length = dev->eps[0].ep.maxpacket;
				comip_ep_out_start(ep, req);
				break;
			}
	} else {
			if (ep->dir_in) {
				/*IN*/
				comip_ep_in_start(ep, req, 0);
			} else {
				/*OUT*/
				comip_ep_out_start(ep, req);
			}
		}
	}

	/* pio or dma irq handler advances the queue. */
	if (req) {
		USBDBG("comip_ep_queue:Add list EP%d :%p,0x%x,0x%x\n",
				ep->ep_num, req, _req->length, _req->zero);
		list_add_tail(&req->queue, &ep->queue);
	}

	usb_ep_int_mask(ep->ep_num, 0);
	return 0;
}

static int comip_udc_eps_init(void)
{
	struct comip_udc *dev = &controller;
	int i = 0;

	/* initialize ep0 */
	sprintf(dev->eps[0].name, "ep0");
	dev->eps[0].desc = &ep0_out_desc;
	dev->eps[0].ep_num = 0;
	dev->eps[0].ep_type = USB_ENDPOINT_XFER_CONTROL;
	dev->eps[0].fifo_size = EP0_FIFO_SIZE;
	dev->eps[0].dev = dev;
	dev->eps[0].assigned = 1;
	INIT_LIST_HEAD(&dev->eps[0].queue);
	dev->eps[0].ep.maxpacket = EP0_FIFO_SIZE;
	dev->eps[0].ep.name = dev->eps[0].name;
	dev->eps[0].ep.ops = &comip_ep_ops;

	/* initilize gadget ep0 pointer */
	dev->gadget.ep0 = &(dev->eps[0].ep);

	/*initilize other eps*/
	INIT_LIST_HEAD(&dev->gadget.ep_list);
	for(i = 1; i < COMIP_EP_NUM; i++)
	{
		struct comip_ep *pcomip_ep = &dev->eps[i];

		memset(pcomip_ep, 0, sizeof(struct comip_ep));

		if (i <= (COMIP_EP_NUM / 2)) { //in ep
			sprintf(pcomip_ep->name, "ep%i%s", i, "in");
			pcomip_ep->dir_in = 1;
		} else if ((i > (COMIP_EP_NUM / 2)) && i< COMIP_EP_NUM) { //out ep
			sprintf(pcomip_ep->name, "ep%i%s", i, "out");
			pcomip_ep->dir_in = 0;
		}

		pcomip_ep->ep.name = pcomip_ep->name;
		pcomip_ep->ep_num = i;
		pcomip_ep->dev = dev;
		pcomip_ep->assigned = 0;
		INIT_LIST_HEAD(&pcomip_ep->queue);
		pcomip_ep->ep.ops = &comip_ep_ops;

		list_add_tail(&dev->eps[i].ep.ep_list, &dev->gadget.ep_list);
	}

	return 0;
}

static int comip_udc_reqs_init(void)
{
	struct comip_udc *dev = &controller;
	struct comip_request *req = NULL;
	u32 buff_addr = 0;

	int req_num = 0;

	void * mem_start = calloc(1, UDC_MEMORY_POOL);
	if (NULL == mem_start)
		USBINFO("calloc mem space failed!\n");

	dev->req_max = UDC_MEMORY_POOL / (UDC_REQ_SIZE + UDC_DMA_BUF_SIZE);
	buff_addr = (u32)mem_start;// + (dev->req_max) * UDC_REQ_SIZE;
	req = (struct comip_request *)(buff_addr + (dev->req_max) * UDC_DMA_BUF_SIZE);
	INIT_LIST_HEAD(&dev->req_free_list);

	USBINFO("UDC_REQ_SIZE(4x): %d comip_req size: %d\n",UDC_REQ_SIZE,sizeof(struct comip_request));
	while (req_num < dev->req_max) {
		memset(req, 0 ,sizeof(struct comip_request));
		INIT_LIST_HEAD(&req->queue);
		req->dma_desc.status = 0;
		req->dma_desc.buf_addr = buff_addr;
		list_add_tail(&req->queue, &dev->req_free_list);

		USBDBG("[%02d]%p -> 0x%8lx\n", \
				req_num, req, (long unsigned int)(req->dma_desc.buf_addr));

		req_num++;
		buff_addr += UDC_DMA_BUF_SIZE;
		req = (struct comip_request *)((u32)req + UDC_REQ_SIZE);
	}
	USBINFO("total req:%d, max:%d\n",req_num, dev->req_max);

	dev->ep0state = EP0_IDLE;

	/* init ep0 control request */
	dev->ep0_req =(struct comip_request *)comip_ep_alloc_request(&dev->eps[0].ep, ((gfp_t)0));
	if (!dev->ep0_req) {
		USBDBG("not enough req.(ep0_req = null)\n");
		return -1;
	}

	INIT_LIST_HEAD(&dev->ep0_req->queue);
	dev->ep0_req->req.actual = 0;
	dev->ep0_req->req.complete = comip_udc_setup_complete;
	dev->ep0_req->dma_desc.status = 0;
	dev->ep0_req->req.length = dev->eps[0].ep.maxpacket;
	dev->ep0_req->req.status = 0;
	dev->ep0_req->req.zero = 0;

	return 0;
}

static void comip_udc_reinit(struct comip_udc *dev)
{
	u16 i;
	dev->ep0state = EP0_IDLE;

	/* basic endpoint records init */
	for (i = 0; i < COMIP_EP_NUM; i++) {
		struct comip_ep *ep = &dev->eps[i];
		ep->stopped = 0;
		ep->irqs = 0;
	}
}

static void comip_ep_in_start(struct comip_ep *ep, struct comip_request *req, int zero)
{
	unsigned int val;
	DIE(req == NULL);

	ep->pdma_desc = &req->dma_desc;

	/* status init */
	val_set(ep->pdma_desc->status, STAT_IO_BS_SHIFT,  2, STAT_IO_BS_H_BUSY);
	val_set(ep->pdma_desc->status, STAT_IO_L_SHIFT,   1, 1);
	val_set(ep->pdma_desc->status, STAT_IO_IOC_SHIFT, 1, 1);
	if (zero)
		val_set(ep->pdma_desc->status, 0, 16,0);
	else
		val_set(ep->pdma_desc->status, 0, 16,req->req.length);
		val_set(ep->pdma_desc->status, STAT_IO_BS_SHIFT, 2, STAT_IO_BS_H_READY);

	/* init desc dma addr */
	comip_udc_write_reg32((u32)(&(ep->pdma_desc->status)), USB_DIEPDMA(ep->ep_num));

	/* update actual length */
	if (!zero)
		req->req.actual += req->req.length;

	USBVDBG("IN ep%d dma:%p,sts:%x buf:%x\n", ep->ep_num, ep->pdma_desc,ep->pdma_desc->status, ep->pdma_desc->buf_addr);

	/* start ep in */
	val = comip_udc_read_reg32(USB_DIEPCTL(ep->ep_num));
	val |= USB_DIOEPCTL_CNAK | USB_DIOEPCTL_EPEN;
	comip_udc_write_reg32(val, USB_DIEPCTL(ep->ep_num));
}

static void comip_udc_enum_done(void)
{
	unsigned int val;
	struct comip_udc *dev = &controller;
	val = comip_udc_read_reg32(USB_DSTS);

	/*1:to get the usb speed */
	switch ((val & USB_DSTS_ENUMSPD) >> 1) {
	case 0:
		dev->gadget.speed = USB_SPEED_HIGH;
		break;
	case 1:
		dev->gadget.speed = USB_SPEED_FULL;
		break;
	case 2:
		dev->gadget.speed = USB_SPEED_LOW;
		break;
	default:
		return;
	}

	/*2:to set up the mps */
	val = comip_udc_read_reg32(USB_DIEPCTL0);
	val_set(val, 0, 2, 0);  /*to set the mps=64 */
	comip_udc_write_reg32(val, USB_DIEPCTL0);

	/* prepare ep0 out for receive */
	comip_ep_out_start(&dev->eps[0], dev->ep0_req);
	//INIT_LIST_HEAD(&dev->eps[0].queue);
	//list_add_tail(&dev->ep0_req->queue, &dev->eps[0].queue);
	//comip_ep_queue(&dev->eps[0].ep, &(dev->ep0_req->req), ((gfp_t)0));
	//struct comip_request *req = container_of(dev->eps[0].queue.next, struct comip_request, queue);
	//printf("add ep0_req to ep0.queue : %x\n", dev->ep0_req);

	/*4. enable ep0 interrupt*/
	val = 0x10001; /*enable input/output ep0 interrupt */
	comip_udc_write_reg32(val, USB_DAINTMSK);
}

static s32 get_mps(s32 speed, u8 bmAttributes)
{
	switch (bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) {
	case USB_ENDPOINT_XFER_CONTROL:
		return (EP0_MPS);
	case USB_ENDPOINT_XFER_ISOC:
		return (ISO_MPS(speed));
	case USB_ENDPOINT_XFER_BULK:
		return (BULK_MPS(speed));
	case USB_ENDPOINT_XFER_INT:
		return (INT_MPS(speed));
	default:
		return 0;
	}
}

static void comip_udc_change_mps(enum usb_device_speed speed)
{
	unsigned i;
	struct comip_ep *ep = NULL;
	struct comip_udc *dev = &controller;

	/* find all validate EPs and change the MPS */
	for (i = 1; i < COMIP_EP_NUM; i++) {

		if (dev->eps[i].assigned) {
			ep = &(dev->eps[i]);
			switch (ep->ep_type & USB_ENDPOINT_XFERTYPE_MASK) {
			case USB_ENDPOINT_XFER_CONTROL:
				ep->ep.maxpacket = EP0_MPS;
				break;
			case USB_ENDPOINT_XFER_ISOC:
				ep->ep.maxpacket = ISO_MPS(speed);
				break;
			case USB_ENDPOINT_XFER_BULK:
				ep->ep.maxpacket = BULK_MPS(speed);
				break;
			case USB_ENDPOINT_XFER_INT:
				ep->ep.maxpacket = INT_MPS(speed);
				break;
			default:
				break;
			}
		}
	}
}

static void comip_udc_setup_complete(struct usb_ep *_ep, struct usb_request *req)
{
	/*Not used*/
}

static void done(struct comip_ep *ep, struct comip_request *req, s32 status)
{
	list_del_init(&req->queue);
	req->req.status = status;
	if (req->req.complete)
	{
		USBVDBG("req->req.complete:0x%p\n",(req->req.complete));
		req->req.complete(&ep->ep, &req->req);
	}
}

static s32 comip_udc_do_request(struct usb_ctrlrequest *ctrl, struct comip_ep *ep)
{
	struct comip_udc   *dev = &controller;
	struct usb_request *_req = &dev->ep0_req->req;
	s32 value = -EOPNOTSUPP;
	s32 pseudo = 0, ret = 0;
	s32 ep_num;

	switch (ctrl->bRequest) {
	case USB_REQ_SET_ADDRESS:
		USBINFO("%s, USB_REQ_SET_ADDRESS 0x%x\n", __func__, ctrl->wValue);
		comip_udc_setaddress(ctrl->wValue);
		pseudo = 1;
		value = 0;
		break;
	case USB_REQ_CLEAR_FEATURE:
		USBINFO("%s,ep%d USB_REQ_CLEAR_FEATURE\n", __func__, ep->ep_num);
		pseudo = 1;
		value = 0;
		break;
	case USB_REQ_SET_FEATURE:
		USBDBG("%s:SET_FEATURE:bRequestType=0x%x,wValue=0x%x,wIndex=0x%x\n",
				__func__, ctrl->bRequestType, ctrl->wValue, ctrl->wIndex);
		pseudo = 1;
		value = 0;
		break;
	case USB_REQ_GET_STATUS:
		USBDBG("%s: GET_STATUS\n", __func__);
		//if(ctrl->wIndex & 0xf)
		//break;
		if (le16_to_cpu(ctrl->wLength) != 2
			|| le16_to_cpu(ctrl->wValue) != 0) {
			USBINFO("Error wValue:0x%x wLength:0x%x!\n",
				ctrl->wValue, ctrl->wLength);
			break;
		}

		pseudo = 1;
		value = 2;
		memset(_req->buf, 0, value);
		if ((ctrl->bRequestType &  USB_RECIP_MASK) == USB_RECIP_DEVICE) {
			*(u16*)(_req->buf) = (1 << USB_DEVICE_SELF_POWERED) |
									(1 << USB_DEVICE_REMOTE_WAKEUP);
		} else if ((ctrl->bRequestType &  USB_RECIP_MASK) == USB_RECIP_ENDPOINT) {
			memset(_req->buf, 0, value);
			ep_num = le16_to_cpu(ctrl->wIndex) & USB_ENDPOINT_NUMBER_MASK;
			*((u16 *)(_req->buf)) = (comip_udc_read_reg32(USB_DIEPCTL(ep_num))
				& USB_DIOEPCTL_STALL) ? 1 : 0 ;
			USBINFO("USB_REQ_GET_STATUS ENDPOINT: ep%d %s\n", ep->ep_num,
				(*((u16 *)(_req->buf)))?"halted":"idle");
		} else if ((ctrl->bRequestType &  USB_RECIP_MASK) == USB_RECIP_INTERFACE) {
			USBINFO("USB_REQ_GET_STATUS INTERFACE\n");
		} else {
			USBINFO("unrecognize bRequestType:0x%x!\n", ctrl->bRequestType);
		}
		USBDBG("handle usb_req_get_status packet at ep%d, buf 0x%p, *buf 0x%x,\n",
			ep->ep_num, _req->buf, *(u16*)(_req->buf));
		break;
	default:
		pseudo = 0;
		USBERR("%s, unrecognize request\n", __func__);
		break;
	}

	if (pseudo) {
		_req->length = value;
		_req->no_interrupt = 0;
		_req->zero = (value <= ctrl->wLength) &&
						((value % ep->ep.maxpacket) == 0);
		USBDBG("%s, comip_ep_queue zero:%d, value:%d, ctrl->wLength:%d, ep->ep.maxpacket:%d\n", \
				__func__, _req->zero, value, ctrl->wLength, ep->ep.maxpacket);

		_req->complete = comip_udc_setup_complete;
		ret = comip_ep_queue(&dev->eps[0].ep, _req, ((gfp_t)0));
		if (ret < 0) {
			_req->status = ret;
			comip_udc_setup_complete(&ep->ep, _req);
		}
		return value;
	} else {
		return -1;
	}
}

static void ep_txtimeout(struct comip_ep *ep)
{
	USBINFO("ep%x timeout!", ep->ep_num);
	return;
}

static void ep_txemp(struct comip_ep *ep)
{
	unsigned int val;

	USBINFO("ep%x txemp!", ep->ep_num);
	val = USB_DIOEPINT_INTXFEMP;
	comip_udc_write_reg32(val, USB_DIEPINT(ep->ep_num));

	return;
}

static void ep_txfifoemp(struct comip_ep *ep)
{
	unsigned int val;

	val = comip_udc_read_reg32(USB_DTXFSTS(ep->ep_num));
	USBINFO("%s:DTXFSTS=0x%x\n", __func__, val);
}

static void ep_txnak(struct comip_ep *ep)
{
	USBINFO("ep%x txnak!", ep->ep_num);
	return;
}

static void ep_txundn(struct comip_ep *ep)
{
	USBINFO("ep%x txundn!", ep->ep_num);
	return;
}

static void ep_rxepdis(struct comip_ep *ep)
{
	USBINFO("ep%x rxepdis!", ep->ep_num);
	return;
}

static void ep_rxpkterr(struct comip_ep *ep)
{
	USBINFO("ep%x rxpkterr!", ep->ep_num);
	return;
}

static void comip_handle_ep0(struct comip_udc *dev)
{
	unsigned int val;
	u32 usb_diepint0;
	u32 usb_doepint0;
	s32 i = 0;
	u32 setup[2];
	struct comip_ep *ep = &dev->eps[0];
	struct comip_request *req;
	struct usb_ctrlrequest r;
	u32 *buf;
	u32 doepmsk;
	u32 wait_count = 0;

	USBDBG("%s, enter\n", __func__);

	val = 0x00;
	usb_diepint0 = comip_udc_read_reg32(USB_DIEPINT0);
	usb_doepint0 = comip_udc_read_reg32(USB_DOEPINT0);

	/*clear USB_DIEPINT0 or USB_DOEPINT0*/
	comip_udc_write_reg32(usb_diepint0, USB_DIEPINT0);
	comip_udc_write_reg32(usb_doepint0, USB_DOEPINT0);

	if (list_empty(&ep->queue)) {
		USBINFO("ep0 no req!\n");
		req = 0;
	}
	else
		req = container_of(ep->queue.next, struct comip_request, queue);

	switch (dev->ep0state) {
	case EP0_NO_ACTION:
		/*Fall through */
		USBDBG("EP0_NO_ACTION\n");
	case EP0_IDLE:
		USBDBG("EP0_IDLE\n");
		if (usb_diepint0 & USB_DIOEPINT_XFCOMPL) {
			USBDBG("usb_diepint0 USB_DIOEPINT_XFCOMPL\n");

			val = USB_DIOEPINT_XFCOMPL;
			comip_udc_write_reg32(val, USB_DIEPINT0);

			comip_ep_out_start(&dev->eps[0], dev->ep0_req);
		}

		if (usb_doepint0 & USB_DIOEPINT_XFCOMPL) {
			USBDBG("usb_doepint0 USB_DIOEPINT_XFCOMPL\n");

			if (!(dev->ep0_req->dma_desc.status & STAT_OUT_NON_ISO_SR)) {
				USBDBG("%s, dev->ep0_out_desc->status & STAT_OUT_NON_ISO_SR = 0 start rx\n", __func__);
				comip_ep_out_start(&dev->eps[0], dev->ep0_req);
			}
		}
		if (!(usb_doepint0 & USB_DIOEPINT_SETUP)) {
			USBDBG("not setup. break\n");
			break;
		}

		USBDBG("%s, receive setup package\n", __func__);
		/*
		if (!comip_udc_ep0_state_check(EP0_TRAN_OUT|EP0_TRAN_COMP)) {
			USBERR("ep0transfer :0x%x\n", dev->ep0transfer);
		}
		*/
		val = USB_DIOEPINT_SETUP;   /*Clear */
		comip_udc_write_reg32(val, USB_DOEPINT0);

		buf = (u32 *)dev->ep0_req->dma_desc.buf_addr;
		setup[0] = *buf;
		setup[1] = *(buf + 1);

		*buf = 0;
		*(buf + 1) = 0;

		r.bRequest = (setup[0] >> 8) & 0xff;
		r.bRequestType = (setup[0]) & 0xff;
		r.wValue = (setup[0] >> 16) & 0xffff;
		r.wLength = (setup[1] >> 16) & 0xffff;
		r.wIndex = (setup[1]) & 0xffff;

		USBINFO("setup: bRequest:%x,bRequestType:%x,wValue:%x, wLength:%x,wIndex:%x\n",
				r.bRequest, r.bRequestType, r.wValue, r.wLength, r.wIndex);

		if (r.bRequestType & USB_DIR_IN)
			dev->ep0state = EP0_IN_DATA_PHASE;
		else
			dev->ep0state = EP0_OUT_DATA_PHASE;

		if (r.wLength == 0)
			dev->ep0state = EP0_IN_DATA_PHASE;

		USBDBG("%s: dev->ep0state=0x%x\n", __func__, dev->ep0state);
		if ((r.bRequest == USB_REQ_SET_ADDRESS)
			||((r.bRequest == USB_REQ_CLEAR_FEATURE)
				&& ((r.bRequestType == 0x00)
					||(r.bRequestType == 0x01)
					||(r.bRequestType == 0x02)))
			||((r.bRequest == USB_REQ_SET_FEATURE)
				&& (r.bRequestType == USB_DIR_OUT)
				&& (r.wValue == 2))
			||((r.bRequest == USB_REQ_GET_STATUS)
				&&((r.bRequestType == (USB_DIR_IN|USB_RECIP_DEVICE))
					||(r.bRequestType == (USB_DIR_IN|USB_RECIP_ENDPOINT))
					||(r.bRequestType == (USB_DIR_IN|USB_RECIP_INTERFACE))))) {
			USBDBG("%s, comip_udc_do_request\n", __func__);
			i = comip_udc_do_request(&r, ep);
		} else {
			USBDBG("%s setup(%p) speed(%d)\n", __func__, dev->driver->setup, dev->gadget.speed);
			i = -1;
			i = dev->driver->setup(&dev->gadget, &r);
		}
		USBDBG("%s setup return: %d\n", __func__, i);

		if (i >= 0) {
			if (r.bRequest == USB_REQ_SET_CONFIGURATION) {
				/* first unmask endpoint 0 interrupt */
				int data = 0x00010001;

				USBINFO("%s USB_REQ_SET_CONFIGURATION", __func__);

				/* unmask endpoint interrupt */
				for (i = 1; i <= (COMIP_EP_NUM / 2); i++) {
					if(dev->eps[i].assigned)
						data |= (1 << i);
				}

				for (i= (COMIP_EP_NUM / 2) + 1 ; i < COMIP_EP_NUM; i++) {
					if(dev->eps[i].assigned)
						data |= (1 << (i + 16));
				}
				comip_udc_write_reg32(data, USB_DAINTMSK);

			} else if (r.bRequest == USB_REQ_CLEAR_FEATURE) {
				USBDBG("%s USB_REQ_CLEAR_FEATURE", __func__);

				if ((r.bRequestType & USB_RECIP_MASK) == USB_RECIP_ENDPOINT) {
					if ((r.wIndex & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN) {
						USBDBG("%s USB_REQ_CLEAR_FEATURE: USB_DIR_IN", __func__);
						val = comip_udc_read_reg32(USB_DIEPCTL(r.wIndex & 0x7f));
						val |= USB_DIOEPCTL_SETD0PID;
						if (val & USB_DIOEPCTL_STALL)
							val &= ~USB_DIOEPCTL_STALL;

						comip_udc_write_reg32(val, USB_DIEPCTL(r.wIndex & 0x7f));
						val = comip_udc_read_reg32(USB_GRSTCTL);
						val_set(val, 6, 5, (r.wIndex & 0x7f));  /*Tx FIFO flush */
						val |= USB_GRSTCTL_TXFFLSH;
						comip_udc_write_reg32(val, USB_GRSTCTL);

						val = comip_udc_read_reg32(USB_GRSTCTL);

						wait_count = 0;
						while (val & USB_GRSTCTL_TXFFLSH) {
							val = comip_udc_read_reg32(USB_GRSTCTL);

							wait_count++;
							if (wait_count > MAX_WAIT_COUNT) {
								USBINFO("%s: wait USB_GRSTCTL_TXFFLSH  time out\n", __func__);
								return;
							}
						}
					} else {
						USBDBG("%s USB_REQ_CLEAR_FEATURE: USB_DIR_OUT", __func__);

						val = comip_udc_read_reg32(USB_DOEPCTL(r.wIndex & 0x7f));
						val |= USB_DIOEPCTL_SETD0PID;
						if (val & USB_DIOEPCTL_STALL)
							val &=~USB_DIOEPCTL_STALL;

						comip_udc_write_reg32(val, USB_DOEPCTL(r.wIndex &0x7f));
					}
				}
			} else if (r.bRequest == USB_REQ_SET_FEATURE) {
				USBINFO("%s USB_REQ_SET_FEATURE", __func__);

				if ((r.bRequestType == USB_DIR_OUT) && (r.wValue == 2)
					/*&& ((ctrl->wIndex & 0xff) == 0)*/) {
					int test_mode = (r.wIndex >> 8);
					unsigned long test_mask = 0x7;
					unsigned long test_val;

					USBINFO("%s: enter test_mode=0x%x\n", __func__, test_mode);

					test_val = comip_udc_read_reg32(USB_DCTL);
					test_val &= ~(test_mask << USB_DCTL_TSTCTL_SHIFT);
					test_val |= (test_mode << USB_DCTL_TSTCTL_SHIFT);
					comip_udc_write_reg32(test_val, USB_DCTL);
				}
			}
		}
		if (i < 0) {
			/* hardware automagic preventing STALL... */
			//stall:
			USBINFO("setup: bRequest:%x,bRequestType:%x,wValue:%x, wLength:%x,wIndex:%x\n",
				r.bRequest, r.bRequestType, r.wValue, r.wLength, r.wIndex);
			val = comip_udc_read_reg32(USB_DIEPCTL0);
			val |= USB_DIEPCTL0_STALL;
			comip_udc_write_reg32(val, USB_DIEPCTL0);
			dev->ep0state = EP0_IDLE;
			USBINFO("comip_handle_ep0:STALL!! do nothing\n");

			dev->ep0_req->req.length = dev->eps[0].ep.maxpacket;
			comip_ep_out_start(&dev->eps[0], dev->ep0_req);
		}
		break;
	case EP0_IN_DATA_PHASE: /* GET_DESCRIPTOR etc */
		USBDBG("%s: EP0_IN_DATA_PHASE \n", __func__);

		if (usb_diepint0 & USB_DIOEPINT_XFCOMPL) {  /*tx finish */
			USBDBG("%s: tx finish\n", __func__);

			/* clear interrupt status */
			val = USB_DIOEPINT_XFCOMPL;
			comip_udc_write_reg32(val, USB_DIEPINT0);

			if (req) {
				//USBDBG("%s: start send req\n", __func__);
				USBDBG("%s: has req %p and than done \n", __func__, req);
				//comip_ep_in_start(ep, req, 0);
				done(ep, req, 0);
				req = 0;
			}

			{
				USBDBG("%s: start rx, ep0_idle\n", __func__);
				dev->ep0_req->req.length = dev->eps[0].ep.maxpacket;
				comip_ep_out_start(&dev->eps[0], dev->ep0_req);
				dev->ep0state = EP0_IDLE;
			}
		}
		break;
	case EP0_OUT_DATA_PHASE:    /* SET_DESCRIPTOR etc */
		USBDBG("%s: EP0_OUT_DATA_PHASE \n", __func__);
		if (usb_doepint0 & USB_DIOEPINT_XFCOMPL) {
			int length;
			USBDBG("%s: rx complete\n", __func__);

			length = ep->ep.maxpacket - \
				(dev->ep0_req->dma_desc.status & STAT_IO_NON_ISO_RTB_MASK);
			if (length > REQ_BUFSIZ) {
				USBERR("%s:EP0_OUT_DATA_PHASE: length larger than 0x%x, stall!!!!\n",\
					__func__, length);
				dev->ep0state = EP0_STALL;
				break;
			}

			req = dev->ep0_req;
			req->req.actual += length;

			if (req->req.actual < req->req.length) {
				USBDBG("%s: req->req.actual < req->req.length start to receive again\n", __func__);
				dev->ep0_req->req.length = dev->eps[0].ep.maxpacket;
				comip_ep_out_start(&dev->eps[0], dev->ep0_req);
				//comip_ep_queue(&dev->eps[0], dev->ep0_req);
				break;
			}

			doepmsk = comip_udc_read_reg32(USB_DOEPMSK);
			if (doepmsk & USB_DIOEPINT_SPR) {
				wait_count = 0;
				while(!(usb_doepint0 & USB_DIOEPINT_SPR)) {
					usb_doepint0 = comip_udc_read_reg32(USB_DOEPINT0);

					wait_count++;
					if (wait_count > MAX_WAIT_COUNT) {
						USBERR("%s: wait USB_DIOEPINT_SPR  time out\n", __func__);
						return ;
					}
				}
				val = USB_DIOEPINT_SPR;
				comip_udc_write_reg32(val, USB_DOEPINT0);
			}

			dev->ep0state = EP0_IDLE;
		}
		break;
	case EP0_STALL:
		USBINFO("comip_handle_ep0:stall!!");
		break;
	case EP0_IN_FAKE:
		USBINFO("comip_handle_ep0:fake!");
		break;
	default:
		break;
	}
#if 0
	if(usb_doepint0 & USB_DIOEPINT_SETUP) {
		/*clear setup int*/
		USBINFO("clear setup int\n");
		val = USB_DIOEPINT_SETUP;
		comip_udc_write_reg32(val, USB_DOEPINT0);
	}
	else if(usb_diepint0 & USB_DIOEPINT_TIMEOUT) {
		/*clear timeout int*/
		USBINFO("clear timeout int\n");
		val = USB_DIOEPINT_TIMEOUT;
		comip_udc_write_reg32(val, USB_DIEPINT0);
	}
#endif
	/*clear BNA int*/
	if (usb_diepint0 & USB_DIOEPINT_BNA) {
		USBERR("USB_DIOEPINT_BNA usb_diepint0:%x\n", usb_diepint0);
		/*dma_status = dev->ep0_in_desc->status;
		if ( (dma_status >> STAT_IO_BS_SHIFT)) {
			dev->ep0_in_desc->status &=~  ((1 << 30) | (1<<31));
		}*/
	}

	if (usb_doepint0 & USB_DIOEPINT_BNA) {
		USBERR("USB_DIOEPINT_BNA usb_doepint0:%x\n", usb_doepint0);
		/*dma_status = dev->ep0_out_desc->status;
		if ( (dma_status >> STAT_IO_BS_SHIFT)) {
			dev->ep0_out_desc->status &=~  ((1 << 30) | (1<<31));
		}*/
	}
}

static void comip_handle_ep(struct comip_ep *ep, u32 epintsts)
{
	unsigned int val;
	struct comip_request *req, *req_next;
	struct comip_dma_des *dma_des;
	unsigned long dma_status;

	USBVDBG("comip_handle_ep ep%d 0x%x\n", ep->ep_num, epintsts);

	dma_des = ep->pdma_desc;

	if (!list_empty(&ep->queue)) {
		req = container_of(ep->queue.next, struct comip_request, queue);
	} else {
		req = NULL;
		USBINFO("req = NULL\n");
	}

	/*clear USB_DIEPINT and  USB_DOEPINT*/
	if (ep->dir_in) {
		val = comip_udc_read_reg32(USB_DIEPINT(ep->ep_num));
		comip_udc_write_reg32(val, USB_DIEPINT(ep->ep_num));
	} else {
		val = comip_udc_read_reg32(USB_DOEPINT(ep->ep_num));
		comip_udc_write_reg32(val, USB_DOEPINT(ep->ep_num));
	}

	if (epintsts & USB_DIOEPINT_AHBERR) {
		USBERR("USB_DIOEPINT_AHBERR epintsts:0x%x\n", epintsts);
		val = USB_DIOEPINT_AHBERR;
		if (ep->dir_in) /* IN */
			comip_udc_write_reg32(val, USB_DIEPINT(ep->ep_num));
		else
			comip_udc_write_reg32(val, USB_DOEPINT(ep->ep_num));

		comip_ep_enable(&ep->ep, ep->desc);
	}

	if (ep->dir_in) {
		if (epintsts & USB_DIOEPINT_TIMEOUT)
			ep_txtimeout(ep);
		if (epintsts & USB_DIOEPINT_INTXFEMP)
			ep_txemp(ep);
		if (epintsts & USB_DIOEPINT_INNAKEFF)
			ep_txnak(ep);
		if (epintsts & USB_DIOEPINT_TXFUNDN)
			ep_txundn(ep);
		if (epintsts & USB_DIOEPINT_TXFEMP)
			ep_txfifoemp(ep);
	} else {
		if (epintsts & USB_DIOEPINT_OUTEPDIS)
			ep_rxepdis(ep);
		if (epintsts & USB_DIOEPINT_OUTPKTERR)
			ep_rxpkterr(ep);
	}

	/*handle DMA error*/
	if (epintsts & USB_DIOEPINT_BNA) {
		dma_status = dma_des->status;
		USBERR("USB_DIOEPINT_BNA error! dma_status:0x%lx\n", dma_status);
		if ((dma_status >> STAT_IO_BS_SHIFT)) {
			dma_des->status &= ~((1 << 30) | (1 << 31));
			USBINFO("%s USB_DIOEPINT_BNA epintsts:0x%x, dma_des->status:0x%x\n",
					__func__, epintsts, dma_des->status);
		}
	}

	if (epintsts & USB_DIOEPINT_XFCOMPL) {
		if (req) {
			USBDBG("ep%d-in:%d,%s\n",ep->ep_num,ep->dir_in,
					((req->queue.next != &ep->queue)?"More Pack":"Last pack"));

			if (req->queue.next != &ep->queue) {/*Next package? */
				req_next = container_of(req->queue.next, struct comip_request, queue);
				if (ep->dir_in) { /* IN */

					comip_udc_wait_dma_done(ep, dma_des);

					/* clear interrupt */
					val = USB_DIOEPINT_XFCOMPL;
					comip_udc_write_reg32(val, USB_DIEPINT(ep->ep_num));

					/* ep complete */
					done(ep, req, 0);

					/* start next ep */
					comip_ep_in_start(ep, req_next, 0);
				} else { /* OUT */
					/* receive data */
					comip_udc_rx_dma_handle(ep, dma_des, req);

					/* clear interrupt */
					val = USB_DIOEPINT_XFCOMPL;
					comip_udc_write_reg32(val, USB_DOEPINT(ep->ep_num));

					/* done request */
					done(ep, req, 0);

					/* start next req */
					comip_ep_out_start(ep, req_next);
				}
			} else { /*Last one! */
				if (ep->dir_in) {
					comip_udc_wait_dma_done(ep, dma_des);

					/* clear interrupt */
					val = USB_DIOEPINT_XFCOMPL; //add 1225
					comip_udc_write_reg32(val, USB_DIEPINT(ep->ep_num));

					if (req->req.zero) {    //handle zlp
						/* start zero package */
						if ((0 == req->req.length % ep->ep.maxpacket)
							&& req->req.length)
							comip_ep_in_start(ep, req, 1);
						else
							done(ep, req, 0);
						/* reset zero */
						req->req.zero = 0;
					} else {
						/* done request */
						done(ep, req, 0);
					}
				} else {

					/* receive data */
					comip_udc_rx_dma_handle(ep, dma_des, req);

					/* clear interrupt */
					val = USB_DIOEPINT_XFCOMPL; //add 1225
					comip_udc_write_reg32(val, USB_DOEPINT(ep->ep_num));

					/* dont request */
					done(ep, req, 0);
				}
			}
		} else { //no request pending, just clear interrupt
			USBINFO("%s no request pending , but we got dma int!!,dir:%d,req:0x%p\n",
					__func__, ep->dir_in,req);
			if (ep->dir_in) {
				val = USB_DIOEPINT_XFCOMPL;
				comip_udc_write_reg32(val, USB_DIEPINT(ep->ep_num));
			} else {
				val = USB_DIOEPINT_XFCOMPL;
				comip_udc_write_reg32(val, USB_DOEPINT(ep->ep_num));
			}
		}
	}
	ep->irqs++;
}

static void comip_udc_setaddress(u16 address)
{
	unsigned int val;

	val = comip_udc_read_reg32(USB_DCFG);
	val_set(val, 4, 7, address);    /* to set the usb device address added by sdx */
	comip_udc_write_reg32(val, USB_DCFG);
}

//mask: 1:mask and 0:unmask
static void usb_ep_int_mask(u8 ep_num, u8 mask)
{
	unsigned int val;

	val = comip_udc_read_reg32(USB_DAINTMSK);

	if (mask) { //mask
		if (ep_num) { //other eps
			if (ep_num <= (COMIP_EP_NUM / 2)) {
				val &= ~(1 << ep_num);
			} else {
				val &= ~(1 << (ep_num + 16));
			}
		} else { //ep0
			val &= ~((1 << 0) | (1 << 16));
		}
	} else {
		if (ep_num) {
			if (ep_num <= (COMIP_EP_NUM / 2)) {
				val |= (1 << ep_num);
			} else {
				val |= (1 << (ep_num + 16));
			}
		} else {
			val |= ((1 << 0) | (1 << 16));
		}
	}

	comip_udc_write_reg32(val, USB_DAINTMSK);
}

static void comip_udc_reset(void)
{
	unsigned int val;
	u32 i;

	/*1.setup the NAK bit of all the output endpoint */
	for (i = COMIP_EP_NUM / 2 + 1; i < COMIP_EP_NUM; i++) {
		val = comip_udc_read_reg32(USB_DOEPCTL(i));
		val |= USB_DIOEPCTL_SNAK;
		comip_udc_write_reg32(val, USB_DOEPCTL(i));
	}

	/*2.to disable interrupt */
	val = 0x0; /*disable input/output ep0 interrupt */
	comip_udc_write_reg32(val, USB_DAINTMSK);

	val = USB_DOEPMSK_SETUPMSK | USB_DOEPMSK_XFCOMPLMSK | USB_DOEPMSK_SPRMSK
		| USB_DOEPMSK_AHBERRMSK | USB_DOEPMSK_BNAMSK /*| USB_DOEPMSK_B2BSUPMSK*/;
	comip_udc_write_reg32(val, USB_DOEPMSK);

	val = USB_DIEPMSK_AHBERRMSK | USB_DIEPMSK_XFCOMPLMSK | USB_DIEPMSK_BNAMSK;
	comip_udc_write_reg32(val, USB_DIEPMSK);

	/*clear in ep tx  empty fifo intrrupt*/
	comip_udc_write_reg32(0, USB_DIEPEMPMSK);

	/*enable in & out endpoint interrupt */
	val = comip_udc_read_reg32(USB_GINTMSK);
	val &= ~(USB_GINTMSK_NPTXFEMP | USB_GINTMSK_PTXFEMP | USB_GINTMSK_RXFLVL);
	val |= USB_GINTMSK_IEPINT | USB_GINTMSK_OEPINT;
	comip_udc_write_reg32(val, USB_GINTMSK);

}

static int comip_ep_out_start(struct comip_ep *ep, struct comip_request *req)
{
	unsigned int val;

	DIE(req == NULL);

	if (req->req.length == 0) {
		USBINFO("%s, req->req.length = 0\n", __func__);
		return -EINVAL;
	}

	ep->pdma_desc = &req->dma_desc;
	/* status init */
	val_set(ep->pdma_desc->status, STAT_IO_BS_SHIFT,  2, STAT_IO_BS_H_BUSY);
	val_set(ep->pdma_desc->status, STAT_IO_L_SHIFT,   1, 1);
	val_set(ep->pdma_desc->status, STAT_IO_IOC_SHIFT, 1, 1);
	val_set(ep->pdma_desc->status, 0, 16, GET_DMA_BYTES(req->req.length, ep->ep.maxpacket));
	val_set(ep->pdma_desc->status, STAT_IO_BS_SHIFT,  2, STAT_IO_BS_H_READY);

	/* write the data address to DMA register */
	comip_udc_write_reg32((u32)(&(ep->pdma_desc->status)), USB_DOEPDMA(ep->ep_num));

	// Programming Model for Bulk OUT Endpoints With OUT NAK Set for Device
	// Descriptor DMA Mode
	// Program the DOEPTSIZn register for the transfe r size and the
	// corresponding packet count
	if (ep->ep_num) {
		val = comip_udc_read_reg32(USB_DOEPSIZ(ep->ep_num));
		val_set(val, 19, 10, GET_DMA_BYTES(req->req.length, ep->ep.maxpacket) / ep->ep.maxpacket);
		val_set(val, 0, 19, req->req.length);
		comip_udc_write_reg32(val, USB_DOEPSIZ(ep->ep_num));
	} else {
		val = comip_udc_read_reg32(USB_DOEPSIZ(ep->ep_num));
		val_set(val, 29, 2, 1);
		val_set(val, 19, 1, 1);
		val_set(val, 0, 7, ep->ep.maxpacket);
		comip_udc_write_reg32(val, USB_DOEPSIZ(ep->ep_num));
	}

	USBVDBG("OUT ep%d dma:%p,sts:%x buf:%x\n", ep->ep_num, ep->pdma_desc,
			ep->pdma_desc->status, ep->pdma_desc->buf_addr);
	/* start out dma */
	val = comip_udc_read_reg32(USB_DOEPCTL(ep->ep_num));
	val |= USB_DIOEPCTL_CNAK | USB_DIOEPCTL_EPEN;
	comip_udc_write_reg32(val, USB_DOEPCTL(ep->ep_num));

	return 0;
}

void comip_udc_irq(void *data)
{
	struct comip_udc *dev = &controller;
	unsigned int val;
	u32 usb_gintsts;
	u32 usb_gintmsk;

	/* get raw interrupt & mask */
	usb_gintsts = comip_udc_read_reg32(USB_GINTSTS);
	usb_gintmsk = comip_udc_read_reg32(USB_GINTMSK);

	USBINFO("I:%x M:%x\n", usb_gintsts, usb_gintmsk);

	/* clear interrupt */
	comip_udc_write_reg32(usb_gintsts, USB_GINTSTS);

	/* get unmasked interrupt */
	val = (usb_gintsts & usb_gintmsk);

	/* SUSpend Interrupt Request */
	if (usb_gintsts & USB_GINTSTS_USBSUSP) {
		USBINFO("%s Suspend!%x\n",__func__, usb_gintsts);
		/* clear interrupt */
		val = USB_GINTSTS_USBSUSP;
		comip_udc_write_reg32(val, USB_GINTSTS);
	}

	if (usb_gintsts & USB_GINTSTS_ERLYSUSP) {
		USBINFO("%s EarlySuspend!%x\n",__func__, usb_gintsts);
		/* clear interrupt */
		val = USB_GINTSTS_ERLYSUSP;
		comip_udc_write_reg32(val, USB_GINTSTS);
	}

	/* RESume Interrupt Request */
	if (usb_gintsts & USB_GINTSTS_WKUPINT) {
		USBINFO("%s Resume!%x\n",__func__, usb_gintsts);
		/* clear interrupt */
		val = USB_GINTSTS_WKUPINT;
		comip_udc_write_reg32(val, USB_GINTSTS);
	}

	/* ReSeT Interrupt Request - USB reset */
	if (usb_gintsts & USB_GINTSTS_USBRST) {
		USBINFO("%s Reset!%x\n",__func__, usb_gintsts);
		dev->gadget.speed = USB_SPEED_UNKNOWN;

		comip_udc_reinit(dev);

		/* set device address to zero */
		comip_udc_setaddress(0);

		/* reset controller */
		comip_udc_reset();

		/* clear interrupt */
		val = USB_GINTSTS_USBRST;
		comip_udc_write_reg32(val, USB_GINTSTS);
	}

	/*to get enum speed */
	if (usb_gintsts & USB_GINTSTS_ENUMDONE) {//speed enum completed
		USBINFO("%s Enumdone!%x\n",__func__, usb_gintsts);
		dev->ep0state = EP0_IDLE;

		comip_udc_enum_done();

		/* change the endpoint MPS */
		comip_udc_change_mps(dev->gadget.speed);
		/* clear interrupt */
		val = USB_GINTSTS_ENUMDONE;
		comip_udc_write_reg32(val, USB_GINTSTS);

	}

	/* SOF/uSOF Interrupt */
	if (usb_gintsts & USB_GINTSTS_SOF) {
		USBINFO("%s sof!%x\n",__func__, usb_gintsts);
		/* clear SOF/uSOF interrupt */
		val = USB_GINTSTS_SOF;
		comip_udc_write_reg32(val, USB_GINTSTS);
	}

	/* handle endpoint interrupt */
	if ((usb_gintsts & USB_GINTSTS_OEPINT) || (usb_gintsts & USB_GINTSTS_IEPINT)) {
		u32 usb_daint;
		u32 usb_diepint = 0;
		u32 usb_doepint = 0;
		u16 temp;
		s32 i;

		/* get the interrupted endpoint */
		usb_daint = comip_udc_read_reg32(USB_DAINT);    //USB_DAINT;
		USBVDBG("USB_DAINT:0x%x\n",usb_daint);

		/* ep0 interrupt */
		if (usb_daint & 0x10001) {
			dev->eps[0].irqs++;
			comip_handle_ep0(dev);
		}

		/* ep1--ep7 IN endpoints */
		temp = usb_daint & 0xfe;
		temp &= comip_udc_read_reg32(USB_DAINTMSK);

		if (temp) {
			for (i = 1; i <= COMIP_EP_NUM / 2; i++) {
				if (temp & (1 << i)) {
					/* get unmasked IN interrupt */
					usb_diepint = (comip_udc_read_reg32(USB_DIEPINT(i)) & \
									comip_udc_read_reg32(USB_DIEPMSK));
					comip_handle_ep(&dev->eps[i], usb_diepint);
				}
			}
		}

		/* ep8--ep14 OUT endpoints */
		temp = (usb_daint & 0x7f000000) >> 16;
		if (temp) {
			for (i = COMIP_EP_NUM / 2 + 1; i < COMIP_EP_NUM; i++) {
				if (temp & (1 << i)) {
					usb_doepint = (comip_udc_read_reg32(USB_DOEPINT(i)) & \
									comip_udc_read_reg32(USB_DOEPMSK));
					comip_handle_ep(&dev->eps[i], usb_doepint);
				}
			}
		}
	}
}

static void comip_udc_clk_set(int enable)
{
	if (enable) {
		comip_udc_write_reg32(USBOTG_12M_EN, AP_PWR_USBCLK_EN);
		udelay(100);
	} else {
		comip_udc_write_reg32(USBOTG_12M_DISABLE, AP_PWR_USBCLK_EN);
		udelay(10);
	}
}

static s32 comip_udc_soft_dis(int disable)
{
	unsigned int val;
	if (disable) {
		/*enable soft disconnect */
		val = comip_udc_read_reg32(USB_DCTL);
		val |= USB_DCTL_SFTDISC;
		comip_udc_write_reg32(val, USB_DCTL);
		USBINFO("USB soft disconnect!\n");
	} else {
		/*disable soft disconnect */
		val = comip_udc_read_reg32(USB_DCTL);
		val &= ~USB_DCTL_SFTDISC;
		comip_udc_write_reg32(val, USB_DCTL);
		USBINFO("USB soft connect!\n");
	}
	return 0;
}

static s32 comip_udc_eps_fifo_config(void)
{
	unsigned int val;
	u16 i, fifo_size, fifo_start;

	val = 0x01c0;       /*Default rx fifo depth */
	comip_udc_write_reg32(val, USB_GRXFSIZ);

	val = comip_udc_read_reg32(USB_GNPTXFSIZ);
	val_set(val, 16, 16, 0xC0);
	val_set(val, 0, 16, 0x01c0);    /*Tx fifo0 */
	comip_udc_write_reg32(val, USB_GNPTXFSIZ);

	fifo_start = 0x0280;
	for (i = 1; i <= COMIP_EP_NUM / 2; i++) {
		fifo_size = BULK_FIFO_SIZE / 4;
		val = comip_udc_read_reg32(USB_DIEPTXF(i));
		val_set(val, 16, 16, fifo_size);
		val_set(val, 0, 16, fifo_start);    /*Tx fifo0 */
		comip_udc_write_reg32(val, USB_DIEPTXF(i));
		fifo_start += fifo_size;
	}
	return 0;
}

static void comip_udc_ep_flush(int ep_num, int dir_in)
{
	unsigned int val;
	int i = 0;

	USBDBG("ep%d flush!\n", ep_num);

	if (dir_in) {
		val = comip_udc_read_reg32(USB_GRSTCTL);
		val_set(val, 6, 5, ep_num); /*Now NP Tx FIFO flush */
		val |= USB_GRSTCTL_TXFFLSH;
		comip_udc_write_reg32(val, USB_GRSTCTL);
		val = comip_udc_read_reg32(USB_GRSTCTL);
		while (val & USB_GRSTCTL_TXFFLSH) {
			i++;
			if (i > MAX_FLUSH_FIFO_WAIT) {
				USBINFO("%s: wait USB_GRSTCTL_TXFFLSH time out\n", __func__);
				return;
			}
			udelay(10);
			val = comip_udc_read_reg32(USB_GRSTCTL);
		}
	} else {
		val = comip_udc_read_reg32(USB_GRSTCTL);
		val |= USB_GRSTCTL_RXFFLSH;
		comip_udc_write_reg32(val, USB_GRSTCTL);

		i = 0;
		val = comip_udc_read_reg32(USB_GRSTCTL);
		while (val & USB_GRSTCTL_RXFFLSH) {
			i++;
			if (i > MAX_FLUSH_FIFO_WAIT) {
				USBINFO("%s: wait USB_GRSTCTL_RXFFLSH time out\n", __func__);
				return ;
			}
			udelay(1);
			val = comip_udc_read_reg32(USB_GRSTCTL);
		}
	}
}

static int comip_udc_wait_dma_done(struct comip_ep *ep,struct comip_dma_des *dma_des)
{
	int timeout = 0;
	int ret = 0;
	while ((dma_des->status >> STAT_IO_BS_SHIFT) == STAT_IO_BS_DMABUSY) {
		USBINFO("%s waiting for dma transfer compelete after int, des status 0x%x, AHB 0x%x\n",
			ep->name, dma_des->status, comip_udc_read_reg32(USB_GAHBCFG));
		if (timeout++ > 100) {
			if (timeout > 200) {
				USBINFO("time is too long!\n");
				ret = -ETIMEDOUT;
				break;
			}
			udelay(1);
		}
	}

	if ((dma_des->status >> STAT_IO_BS_SHIFT) != STAT_IO_BS_DMADONE) {
		USBINFO("check dma status!dma des status 0x%x, AHB 0x%x\n",
			dma_des->status, comip_udc_read_reg32(USB_GAHBCFG));
		ret = -EIO;
	}
	return ret;
}

static void comip_udc_rx_dma_handle(struct comip_ep *ep, struct comip_dma_des *dma_des, struct comip_request *req)
{
	int length;
	unsigned int val;

	comip_udc_wait_dma_done(ep, dma_des);

	if ((ep->pdma_desc->status >> STAT_IO_BS_SHIFT) == STAT_IO_BS_DMADONE) {
		/* update the received data size */
		length = (GET_DMA_BYTES(req->req.length, ep->ep.maxpacket) -
			(dma_des->status & STAT_IO_NON_ISO_RTB_MASK));

		/* should never happen */
		if (length > UDC_DMA_BUF_SIZE) {
			USBERR("%s:length is too large 0x%x,use maxpacket length:0x%lx\n",
				__func__, length, UDC_DMA_BUF_SIZE);
			req->req.actual += UDC_DMA_BUF_SIZE;
			return;
		}

		val = (ep->pdma_desc->status >> STAT_IO_RTS_SHIFT);
		if (val & 0x03) {
			USBERR("%s:check buffer status:0x%x\n",
				__func__, ep->pdma_desc->status);
		}

		USBVDBG("ep%d-rx:%d\n",ep->ep_num,length);
		req->req.actual += length;
	} else {
		USBINFO("%s DMA not Done!ep->dma_desc->status:0x%x\n",
			__func__, ep->pdma_desc->status);
		comip_ep_enable(&ep->ep, ep->desc);
	}
}

static s32 comip_ep_enable(struct usb_ep *_ep, const struct usb_endpoint_descriptor *desc)
{
	unsigned int val;
	struct comip_ep *ep;
	struct comip_udc *dev;

	ep = container_of(_ep, struct comip_ep, ep);
	dev = ep->dev;

	ep->desc = desc;
	if (!list_empty(&ep->queue))
		USBINFO("%s, %d enabling a non-empty endpoint!", __func__, ep->ep_num);

	ep->stopped = 0;
	ep->irqs = 0;
	ep->assigned=1;
	ep->dir_in = usb_endpoint_dir_in(desc) ? 1 : 0;
	ep->ep_num  = usb_endpoint_num(desc);
	ep->ep_type  = usb_endpoint_type(desc);
	ep->ep.maxpacket = get_mps(dev->gadget.speed, desc->bmAttributes);
	/* flush fifo */
	comip_udc_ep_flush(ep->ep_num, ep->dir_in); /*only for TX */

	/* The variable pdma_desc is NULL, now. memset is unreasonable.*/
	//memset(ep->pdma_desc, 0, sizeof(struct comip_dma_des));

	if (ep->dir_in) {
		/* IN */
		val = comip_udc_read_reg32(USB_DIEPCTL(ep->ep_num));
		val |= USB_DIOEPCTL_ACTEP;
		val_set(val, 0, 11, ep->ep.maxpacket);
		val_set(val, 18, 2, ep->ep_type);

		if (ep->ep_type & 0x2)  /*INT or BULK */
			val |= USB_DIOEPCTL_SETD0PID;
		val_set(val, 22, 4, ep->ep_num);
		comip_udc_write_reg32(val, USB_DIEPCTL(ep->ep_num));
	} else {
		/* OUT */
		val = comip_udc_read_reg32(USB_DOEPCTL(ep->ep_num));
		val |= USB_DIOEPCTL_ACTEP;
		val_set(val, 0, 11, ep->ep.maxpacket);
		val_set(val, 18, 2, ep->ep_type);

		if (ep->ep_type & 0x2)  /*INT or BULK */
			val |= USB_DIOEPCTL_SETD0PID;

		comip_udc_write_reg32(val, USB_DOEPCTL(ep->ep_num));
	}

	USBINFO("%s:: ep%x enabled!\n", __func__, ep->ep_num);
	return 0;
}

static int comip_udc_hw_init(void)
{
	unsigned int val;
	int i;

#ifdef CONFIG_CPU_LC1860
	//TODO: USB0VBUS
	if (!cpu_is_lc1860_eco2()) {
		printf("cpu_is_not_lc1860_eco2 CTL_OTG_CORE_CTRL->0x3\n");
		comip_udc_write_reg32(0x03,CTL_OTG_CORE_CTRL);
	} else {
		printf("cpu_is_lc1860_eco2 CTL_OTG_CORE_CTRL->0x0\n");
		comip_udc_write_reg32(0x0,CTL_OTG_CORE_CTRL);
	}

	comip_udc_write_reg32(0x01, CTL_POR_OTGPHY_CTRL);

	comip_udc_clk_set(0);

	if (cpu_is_lc1860_eco2()) {
		val = comip_udc_read_reg32(CTL_OTGPHY_PARAM_OVERRIDE);
		val &= ~(0x3<<22);
		val |= (0x1<<22);
		comip_udc_write_reg32(val,CTL_OTGPHY_PARAM_OVERRIDE);
		printf("cpu_is_lc1860_eco2 CTL_OTGPHY_PARAM_OVERRIDE->(0x1<<22)\n");
	} else {
		printf("cpu_is_not_lc1860_eco2 CTL_OTGPHY_PARAM_OVERRIDE=0x%x\n",
				comip_udc_read_reg32(CTL_OTGPHY_PARAM_OVERRIDE));
	}

	comip_udc_write_reg32(0x0, CTL_OTGPHY_SUSPENDM_CTRL);
	mdelay(10);
	comip_udc_write_reg32(0x0, CTL_POR_OTGPHY_CTRL);

	comip_udc_clk_set(1);
	mdelay(1);

	comip_udc_write_reg32(0x1, CTL_OTGPHY_SUSPENDM_CTRL);
	mdelay(10);

	if (!cpu_is_lc1860_eco2()) {
		/*set USB PHY reset */
		val = 0x01;
		comip_udc_write_reg32(val, CTL_POR_OTGPHY_CTRL);
		mdelay(10);
		/*set USB PHY no reset */
		val = 0x00;
		comip_udc_write_reg32(val, CTL_POR_OTGPHY_CTRL);
		mdelay(10);
		printf("cpu_is_not_lc1860_eco2 CTL_POR_OTGPHY_CTRL double reset \n");
	} else {
		printf("cpu_is_lc1860_eco2 CTL_POR_OTGPHY_CTRL no need reset\n");
	}

	comip_udc_write_reg32(0x1, CTL_OTG_CORE_RST_CTRL);
	mdelay(1);

#endif

#ifdef CONFIG_CPU_LC1813
	/* improve signal quality in eye diagram */
	if(cpu_is_lc1813s())
	{
		val = comip_udc_read_reg32(CTL_USB_OTG_PHY_CFG0);
		val &= ~((3<<28)|(3<<20)|(1<<18)|(3<<16));
		val |= ((3<<28)|(2<<20)|(1<<18)|(3<<16));
		comip_udc_write_reg32(val, CTL_USB_OTG_PHY_CFG0);
	}

	/*set USB PHY no suspend */
	if(cpu_is_lc1813s())
	{
		val = comip_udc_read_reg32(CTL_USB_OTG_PHY_SUSPEND);
		val |= 0x01;
	}
	else
	{
		val = 0x01;
	}
	comip_udc_write_reg32(val, CTL_USB_OTG_PHY_SUSPEND);
	mdelay(10);
#endif

#ifdef CONFIG_CPU_LC1813
	/*set USB PHY no reset */
	if( cpu_is_lc1813s())
	{
		val = comip_udc_read_reg32(CTL_USB_OTG_PHY_RST_CTRL);
		val |= 0x10;
	}
	else
	{
		val = 0x01;
	}
	comip_udc_write_reg32(val, CTL_USB_OTG_PHY_RST_CTRL);
	mdelay(10);
#endif

#if defined(CONFIG_COMIP_TARGETLOADER) || defined(CONFIG_COMIP_FASTBOOT)
	/*Soft reset*/
	val = USB_GRSTCTL_CSRST;
	comip_udc_write_reg32(val, USB_GRSTCTL);
	mdelay(10);

	//wait for soft reset to finish
	i = 0;
	val = comip_udc_read_reg32(USB_GRSTCTL);
	while (!(val & USB_GRSTCTL_AHBIDLE) || (val & USB_GRSTCTL_CSRST)) {
		i++;
		if (i > MAX_WAIT_COUNT) {
			USBERR("%s: wait USB_GRSTCTL_AHBIDLE  time out\n",  __func__);
			/*
			 * Fix me? when usb cable offline, these condition cann't be ture!
			 * targetloader: always online.
			 * fastboot: always offline
			 * So, remove "goto err"?
			 */
			goto err;
		}
		udelay(10);
		val = comip_udc_read_reg32(USB_GRSTCTL);
	}
#endif

	val = USB_GAHBCFG_DMAEN | USB_GAHBCFG_GLBINTMSK; /* DMA mode */
	comip_udc_write_reg32(val, USB_GAHBCFG);
	val = comip_udc_read_reg32(USB_GAHBCFG);

	/*force device mode */
	val = comip_udc_read_reg32(USB_GINTSTS);
	if ((val & USB_GINTSTS_CURMOD)) {
		val = comip_udc_read_reg32(USB_GUSBCFG);
		val |= USB_GUSBCFG_FRCDM;
		comip_udc_write_reg32(val, USB_GUSBCFG);
		//mdelay(50);
		mdelay(50);
	}

	/*disable HNPCap and SRPCap,USB Turnaround Time to be 5,
	 *16bits USB 2.0 HS UTMI+ PHY ,FS Timeout Calibration to be 5 */
	val = 0x1408;
	comip_udc_write_reg32(val, USB_GUSBCFG);
	mdelay(1);

	/* ensable des DMA, FS, set the NZStsOUTHShk */
#ifdef CONFIG_USB_HIGH
	val = 0x4;
#else
	val = 0x5;
#endif
	val |= USB_DCFG_DESCDMA | USB_DCFG_ENOUTNAK; /* enable desriptor DMA */
	comip_udc_write_reg32(val, USB_DCFG);

	val = comip_udc_read_reg32(USB_DCFG);
	mdelay(10);

	/* configure fifo */
	comip_udc_eps_fifo_config();
	/* flush fifo after configure fifo */
	comip_udc_ep_flush(0x10, 1);
	comip_udc_ep_flush(0, 0);

	/* Clear all pending Device Interrupts */
	comip_udc_write_reg32(0, USB_DIEPMSK);
	comip_udc_write_reg32(0, USB_DOEPMSK);
	comip_udc_write_reg32(0xffffffff, USB_DAINT);
	comip_udc_write_reg32(0, USB_DAINTMSK);

	/* endpoint init */
	/* IN */
	/* endpoint 0-7 */
	for (i=0; i <= (COMIP_EP_NUM / 2); i++) {
		val = comip_udc_read_reg32(USB_DIEPCTL(i));
		if (val & USB_DIOEPCTL_EPEN) { //endpoint is enabled
			val = USB_DIOEPCTL_EPDIS | USB_DIOEPCTL_SNAK;
			comip_udc_write_reg32(val, USB_DIEPCTL(i));
		} else {
			val = 0;
			comip_udc_write_reg32(val, USB_DIEPCTL(i));
		}
		comip_udc_write_reg32(0, USB_DIEPSIZ(i));
		comip_udc_write_reg32(0, USB_DIEPDMA(i));
		comip_udc_write_reg32(0xff, USB_DIEPINT(i));
	}

	/* IN ep0 */
	comip_udc_write_reg32((64 + (1 << 19)), USB_DIEPSIZ(i));

	/* OUT */
	/* endpoint 0 */
	i = 0;
	val = comip_udc_read_reg32(USB_DOEPCTL(i));
	if (val & USB_DIOEPCTL_EPEN) { //endpoint is enabled
		val = USB_DIOEPCTL_EPDIS | USB_DIOEPCTL_SNAK;
		comip_udc_write_reg32(val, USB_DOEPCTL(i));
	} else {
		val = 0;
		comip_udc_write_reg32(val, USB_DOEPCTL(i));
	}
	//comip_udc_write_reg32(0, USB_DOEPSIZ(i));
	comip_udc_write_reg32(64 + (1 << 19) + (1 << 29), USB_DOEPSIZ(i));
	comip_udc_write_reg32(0, USB_DOEPDMA(i));
	comip_udc_write_reg32(0xff, USB_DOEPINT(i));

	/* endpoint 8-14 */
	for (i = (COMIP_EP_NUM / 2) + 1; i < COMIP_EP_NUM; i++) {
		val = comip_udc_read_reg32(USB_DOEPCTL(i));
		if (val & USB_DIOEPCTL_EPEN) { //endpoint is enabled
			val = USB_DIOEPCTL_EPDIS | USB_DIOEPCTL_SNAK;
			comip_udc_write_reg32(val, USB_DOEPCTL(i));
		} else {
			val = 0;
			comip_udc_write_reg32(val, USB_DOEPCTL(i));
		}
		comip_udc_write_reg32(0, USB_DOEPSIZ(i));
		comip_udc_write_reg32(0, USB_DOEPDMA(i));
		comip_udc_write_reg32(0xff, USB_DOEPINT(i));
	}

	i = 0;
	/*wait for device mode, add by zw */
	val = comip_udc_read_reg32(USB_GINTSTS);
	while (val & USB_GINTSTS_CURMOD) {
		USBERR("%s: wait USB_GINTSTS_CURMOD\n",  __func__);
		i++;
		if (i > MAX_WAIT_COUNT) {
			USBERR("%s: wait USB_GINTSTS_CURMOD  time out\n",  __func__);
			goto err;
		}
		udelay(1);
		val = comip_udc_read_reg32(USB_GINTSTS);
	}

	USBINFO("udc hw init done\n");
	return 0;

err:
	/*disable suspend*/
	val = 0x00;
	comip_udc_write_reg32(val, CTL_USB_OTG_PHY_SUSPEND);
	mdelay(10);

	/*disable phy */
	val = 0x00;
	comip_udc_write_reg32(val, CTL_USB_OTG_PHY_RST_CTRL);
	mdelay(10);

	USBINFO("udc hw init error\n");
	return -1;
}

static void comip_udc_enable(struct comip_udc *dev)
{
	unsigned int val;
	dev->ep0state = EP0_IDLE;
	/* default speed, or should be unknown here? */
	dev->gadget.speed = USB_SPEED_UNKNOWN;

	val = comip_udc_read_reg32(USB_GINTSTS);
	val |= USB_GINTSTS_WKUPINT | USB_GINTSTS_SREQINT | USB_GINTSTS_DISCONNINT |
			USB_GINTSTS_CONIDSTSCHNG | USB_GINTSTS_FETSUSP |
			USB_GINTSTS_INCOMPISOOUT | USB_GINTSTS_INCOMPISOIN |
			USB_GINTSTS_EOPF | USB_GINTSTS_ISOOUTDROP | USB_GINTSTS_ENUMDONE |
			USB_GINTSTS_USBRST | USB_GINTSTS_USBSUSP | USB_GINTSTS_ERLYSUSP |
			USB_GINTSTS_SOF | USB_GINTSTS_MODEMIS;
	comip_udc_write_reg32(val, USB_GINTSTS);

	/* enable reset/suspend/early suspend/ENUM/SOF irqs */
	val = comip_udc_read_reg32(USB_GINTMSK);
	val &= ~ (USB_GINTMSK_NPTXFEMP | USB_GINTMSK_PTXFEMP | USB_GINTMSK_RXFLVL);
	val |= USB_GINTMSK_USBRST | USB_GINTMSK_ENUMDONE;
	comip_udc_write_reg32(val, USB_GINTMSK);
}

static int comip_pullup(struct usb_gadget *gadget, int is_on)
{
#ifdef CONFIG_CPU_LC1813
	unsigned int val;

	if(cpu_is_lc1813s())
	{
		printf("%s:%d cpu is lc1813s\n", __func__ , __LINE__);
		/*set USB PHY reset */
		val = 0x00;
		comip_udc_write_reg32(val, CTL_USB_OTG_PHY_RST_CTRL);
		mdelay(10);

		comip_udc_clk_set(0);

		/*set USB PHY sleep */
		val = 0x02;
		comip_udc_write_reg32(val, CTL_USB_OTG_PHY_SUSPEND);
		mdelay(10);

		/*set USB PHY no reset */
		val = comip_udc_read_reg32(CTL_USB_OTG_PHY_RST_CTRL);
		val |= 0x01;
		comip_udc_write_reg32(val, CTL_USB_OTG_PHY_RST_CTRL);
		mdelay(10);
	}
#endif
	unsigned int val;
	//USB0VBUS
	val = comip_udc_read_reg32(CTL_OTGPHY_CTRL);
	val |= (3<<11);
	comip_udc_write_reg32(val, CTL_OTGPHY_CTRL);
	mdelay(10);

	printf("set ctl_otgphy_ctrl (3<<11)\n");

	// enable usb clock
	comip_udc_clk_set(1);
	mdelay(1);

	// soft disconnect first
	comip_udc_soft_dis(1);
	mdelay(2);

	// register usb irq
	irq_install_handler(INT_USB_OTG, comip_udc_irq, NULL);
	enable_irq(INT_USB_OTG);

	if (comip_udc_hw_init())
		goto fail;

	comip_udc_enable(&controller);
	comip_udc_soft_dis(0);

	return 0;

fail:
	printf("%s failed!\n",__func__);
	return -1;
}

static int comip_udc_probe(void)
{
	controller.gadget.ops = &comip_udc_ops;
	comip_udc_eps_init();
	comip_udc_reqs_init();
	return 0;
}

int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	struct comip_udc *dev = &controller;
	int retval = 0;

	if (!driver
		|| (driver->speed != USB_SPEED_FULL&& driver->speed != USB_SPEED_HIGH)
		|| !driver->bind || !driver->disconnect || !driver->setup)
		return -EINVAL;
	if (!dev)
		return -ENODEV;
	if (dev->driver)
		return -EBUSY;

	dev->driver = driver;

	if (!comip_udc_probe()) {
		USBINFO("comip_udc_proble ok!\n");
	}

	retval = driver->bind(&dev->gadget);
	if (retval) {
		USBINFO("%s: bind to driver --> error %d\n",
				dev->gadget.name, retval);
		dev->driver = 0;
		return retval;
	}
	controller.driver = driver;

	return 0;
}
