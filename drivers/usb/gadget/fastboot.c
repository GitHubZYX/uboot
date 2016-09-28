/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <common.h>
#include <asm/errno.h>
#include <command.h>
#include "error.h"

DECLARE_GLOBAL_DATA_PTR;

//#define FASTBOOT_DEBUG
#ifdef FASTBOOT_DEBUG
#define fb_printf(fmt, args...) printf(fmt, ##args)
#else
#define fb_printf(fmt, args...) do {} while(0)
#endif

extern int usb_abi_init(void);
extern int usb_abi_recv(void *packet, int length);
extern int usb_abi_send(void *packet, int length);

/* todo: give lk strtoul and nuke this */
static unsigned hex2unsigned(const char *x)
{
    unsigned n = 0;

    while(*x) {
        switch(*x) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            n = (n << 4) | (*x - '0');
            break;
        case 'a': case 'b': case 'c':
        case 'd': case 'e': case 'f':
            n = (n << 4) | (*x - 'a' + 10);
            break;
        case 'A': case 'B': case 'C':
        case 'D': case 'E': case 'F':
            n = (n << 4) | (*x - 'A' + 10);
            break;
        default:
            return n;
        }
        x++;
    }

    return n;
}

struct fastboot_cmd {
	struct fastboot_cmd *next;
	const char *prefix;
	unsigned prefix_len;
	void (*handle)(const char *arg, void *data, unsigned sz);
};

struct fastboot_var {
	struct fastboot_var *next;
	const char *name;
	const char *value;
};

#define FASTBOOT_CMD_MAX	(20)
static unsigned char cmdnum = 0;
static struct fastboot_cmd cmdbuf[FASTBOOT_CMD_MAX];
static struct fastboot_cmd *cmdlist;

static void fastboot_register(const char *prefix,
		       void (*handle)(const char *arg, void *data, unsigned sz))
{
	struct fastboot_cmd *cmd;

	if (cmdnum >= FASTBOOT_CMD_MAX) {
		fb_printf("too many commands\n");
		return;
	}

	cmd = &cmdbuf[cmdnum++];
	if (cmd) {
		cmd->prefix = prefix;
		cmd->prefix_len = strlen(prefix);
		cmd->handle = handle;
		cmd->next = cmdlist;
		cmdlist = cmd;
	}
}

#define FASTBOOT_VAR_MAX	(5)
static unsigned char varnum = 0;
static struct fastboot_var varbuf[FASTBOOT_VAR_MAX];
static struct fastboot_var *varlist;

static void fastboot_publish(const char *name, const char *value)
{
	struct fastboot_var *var;

	if (varnum >= FASTBOOT_VAR_MAX) {
		fb_printf("too many var\n");
		return;
	}

	var = &varbuf[varnum++];
	if (var) {
		var->name = name;
		var->value = value;
		var->next = varlist;
		varlist = var;
	}
}

static unsigned char buffer[4096];

static void *download_base;
static unsigned download_max;
static unsigned download_size;

#define STATE_OFFLINE	0
#define STATE_COMMAND	1
#define STATE_COMPLETE	2
#define STATE_ERROR	3

static unsigned fastboot_state = STATE_OFFLINE;

#define USB_DMA_BUFFER_LENGTH   (32*1024ul)
static int usb_read(void *_buf, unsigned len)
{
	unsigned int read_cnt=0x0;
	unsigned int left_cnt=0x0;
	unsigned int current_cnt=0x0;
	unsigned int want_cnt=0x0;
	unsigned int read_times=0x0;

	if (fastboot_state == STATE_ERROR)
		return -1;
	fb_printf("usb app read start:\n");
	for(;len>read_cnt;){
		left_cnt=len-read_cnt;
		if(left_cnt>USB_DMA_BUFFER_LENGTH){
			want_cnt=USB_DMA_BUFFER_LENGTH;
		}else{
			want_cnt=left_cnt;
		}
		current_cnt = usb_abi_recv((_buf +read_cnt), want_cnt);

		read_cnt +=current_cnt;

		fb_printf("%d:current_cnt:%d left_cnt=%d  process:%d/%d\n", read_times, current_cnt,left_cnt,read_cnt,len);
		//fb_printf("=");
		read_times++;

		if(current_cnt<USB_DMA_BUFFER_LENGTH){
			break;
		}
	}
	fb_printf("\nusb app read end:\n");
	printf("buf=0x%08X, want=%d, real=%d\n", (unsigned int)_buf, len, read_cnt);

	return read_cnt;
}


static int usb_write(void *buf, unsigned len)
{
	if (fastboot_state == STATE_ERROR)
		return -1;

	usb_abi_send(buf, len);

	return len;
}

static void fastboot_ack(const char *code, const char *reason)
{
	char response[64] = {0};

	if (fastboot_state != STATE_COMMAND)
		return;

	if (reason == 0)
		reason = "";

	if(strlen(code) + strlen(reason) >= 64) {
		printf("%s too long string\r\n", __func__);
	}
	sprintf(response, "%s%s", code, reason);
	fastboot_state = STATE_COMPLETE;

	usb_write(response, strlen(response));
}

static void fastboot_fail(const char *reason)
{
	fastboot_ack("FAIL", reason);
}

static void fastboot_okay(const char *info)
{
	fastboot_ack("OKAY", info);
}

static void cmd_getvar(const char *arg, void *data, unsigned sz)
{
	struct fastboot_var *var;
	fb_printf("fastboot: %s, arg'%s' data %p, sz 0x%x \n", __func__, arg, data,sz);

	for (var = varlist; var; var = var->next) {
		if (!strcmp(var->name, arg)) {
			fastboot_okay(var->value);
			return;
		}
	}
	fastboot_okay("");
}

static void cmd_download(const char *arg, void *data, unsigned sz)
{
	char response[64];
	unsigned len = hex2unsigned(arg);
	int r;

	fb_printf("fastboot: %s, arg'%s' data %p, sz 0x%x \n", __func__, arg, data,sz);
	
	download_size = 0;
	if (len > download_max) {
		put_error("10001");
		fastboot_fail(error_string);
		return;
	}

	sprintf(response,"DATA%08x", len);
	if (usb_write(response, strlen(response)) < 0)
		return;

	r = usb_read(download_base, len);
	if ((r < 0) || (r != len)) {
		fastboot_state = STATE_ERROR;
		return;
	}
	download_size = len;

	fastboot_okay("download completed");
}

static void cmd_flash(const char *arg, void *data, unsigned sz)
{
	int ret = 0;

	data = download_base; //previous downloaded date to download_base

	fb_printf("fastboot: %s, arg:%p|%s date: 0x%x, sz 0x%x\n", __func__, arg,arg, (unsigned int)data, sz);

	ret = flash_partition_erase_write(arg, (u8*)download_base, download_size);
	if (ret)
		printf("write flash failed!! \n" );

	if(!ret)
		fastboot_okay("");
	else
		fastboot_fail(error_string);
}

static void cmd_erase(const char *arg, void *data, unsigned sz)
{
	int ret = 0;
	fb_printf("%s, arg: %s, data: %p, sz: 0x%x\n", __func__, arg, data, sz);
	ret = flash_partition_erase(arg);
	if (ret)
		printf("erase flash failed!! \n" );

	if(!ret)
		fastboot_okay("");
	else
		fastboot_fail("erase error");
}

static void cmd_boot(const char *arg, void *data, unsigned sz)
{
	fb_printf("%s, arg: %s, data: %p, sz: 0x%x\n", __func__, arg, data, sz);
	fastboot_okay("");
}

static void cmd_continue(const char *arg, void *data, unsigned sz)
{
	fb_printf("%s, arg: %s, data: %p, sz: 0x%x\n", __func__, arg, data, sz);
	fastboot_okay("");
}

extern void do_bootm_linux(void);
static void cmd_reboot(const char *arg, void *data, unsigned sz)
{
	fb_printf("%s, arg: %s, data: %p, sz: 0x%x\n", __func__, arg, data, sz);
	fastboot_okay("");
	pmic_reboot_reason_set(REBOOT_NORMAL);
	mdelay(100);
	pmic_reboot();

	while(1);
}

static void cmd_reboot_bootloader(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");
}

static void cmd_powerdown(const char *arg, void *data, unsigned sz)
{
	fb_printf("%s, arg: %s, data: %p, sz: 0x%x\n", __func__, arg, data, sz);
	fastboot_okay("");
}

static void fastboot_command_loop(void)
{
	struct fastboot_cmd *cmd;
	int r;
	fb_printf("fastboot: processing commands\n");

	while (fastboot_state != STATE_ERROR) {
		memset(buffer, 0 , 64);

		r = usb_read(buffer, 64);
		if (r < 0)
			break;
		buffer[r] = 0;
		fb_printf("fastboot: %s, r:%d\n", buffer, r);

		for (cmd = cmdlist; cmd; cmd = cmd->next) {
			fb_printf("cmd list :%s \n", cmd->prefix);
			if (memcmp(buffer, cmd->prefix, cmd->prefix_len))
				continue;
			else
				break;
		}

		if (cmd) {
			printf("enter sub command handler:\n");
			fastboot_state = STATE_COMMAND;
			cmd->handle((const char*) buffer + cmd->prefix_len,
				    (void*) download_base, download_size);

			if (fastboot_state == STATE_COMMAND) {
				fastboot_fail("unknown reason");
			}
		} else {
			printf("unknown command!\n");
			fastboot_fail("unknown command");
		}

	}

	fastboot_state = STATE_OFFLINE;
	fb_printf("fastboot: oops!\n");
}

static int fastboot_handler(void *arg)
{
	for (;;) {
		fastboot_command_loop();
	}
	return 0;
}

extern void comip_usb_power_on(void);
int fastboot_init(void)
{
	int ret=0;

	printf("fastboot_init\n");

	printf("gd->bb_id=0x%lx\n",gd->bb_id);
	printf("REV_ID_MASK=0x%x\n",REV_ID_MASK);
	printf("gd->bb_id&REV_ID_MASK=0x%lx\n",gd->bb_id &REV_ID_MASK);
	if (cpu_is_lc1860_eco2()){
		printf("cpu_is_lc1860_eco2\n");
	} else if (cpu_is_lc1860_eco1()) {
		printf("cpu_is_lc1860_eco1\n");
	} else if(cpu_is_lc1860()){
		printf("cpu_is_lc1860\n");
	} else {
		printf("cpu_is_not_lc1860\n");
	}

	printf("FASTBOOT_BUFFER_SIZE=%dMB\n",CONFIG_FASTBOOT_BUFFER_SIZE/(1024*1024));

	download_base = (void *)calloc(1, CONFIG_FASTBOOT_BUFFER_SIZE);
	download_max = CONFIG_FASTBOOT_BUFFER_SIZE;

	fastboot_register("getvar:", cmd_getvar);
	fastboot_register("download:", cmd_download);
	fastboot_publish("version", "1.0");
	#ifdef CONFIG_CURR_PRODUCT_NAME
	fastboot_publish("product", CONFIG_CURR_PRODUCT_NAME);
	#endif
	fastboot_publish("serial", (const char*)gd->fastboot_device_id);
	fastboot_register("flash:", cmd_flash);
	fastboot_register("erase:", cmd_erase);
	fastboot_register("boot", cmd_boot);
	fastboot_register("reboot", cmd_reboot);
	fastboot_register("powerdown", cmd_powerdown);
	fastboot_register("continue", cmd_continue);
	fastboot_register("reboot-bootloader", cmd_reboot_bootloader);

	comip_usb_power_on();
	mdelay(1000);

	ret = usb_abi_init();
	if (ret) {
		printf("usb_abi_init failed\n");
		return -1;
	} else {
		fastboot_handler(0);
	}

	return 0;
}
