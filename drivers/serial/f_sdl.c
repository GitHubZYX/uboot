/*
 * drivers/serial/f_sdl.c
 *
 * Use of source code is subject to the terms of the LeadCore license agreement
 * under which you licensed source code. If you did not accept the terms of the
 * license agreement, you are not authorized to use source code. For the terms
 * of the license, please see the license agreement between you and LeadCore.
 *
 * Copyright (c) 2010-2019  LeadCoreTech Corp.
 *
 * PURPOSE:
 *    This file is sdl(serial download) driver.
 *
 * CHANGE HISTORY:
 *    Version  Date           Author         Description
 *    1.0.0    2014-02-16     pengyimin      created
 */


#include <common.h>
#include <linux/compiler.h>

#include <ns16550.h>

static NS16550_t serial_port = (NS16550_t)CONFIG_SYS_NS16550_COM1;

int uart_sdl_recv(void *p, int length)
{
	char *c = (char *)p;

	while (length--)
	{
		*c++ = NS16550_getc(serial_port);
	}

	return 0;
}

int uart_sdl_send(void *p, int length)
{
	char *c = (char *)p;

	while (length--)
	{
		NS16550_putc(serial_port, *c++);
	}

	return 0;
}
