#ifndef __PUBH_H__
#define __PUBH_H__

#include <common.h>
#include <asm/io.h>
#include <asm/armv7.h>

#include "hash-internal.h"
#include "rsa.h"
#include "sha256.h"

#define		SHA_BASEADDR								 			0xA0610800
#define 	SHA_CTL			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x00 ))
#define 	SHA_IN_TYPE		        				       			        ((volatile unsigned  int*)( SHA_BASEADDR + 0x04 ))
#define 	SHA_MOD			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x08 ))
#define 	SHA_INTR		        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x0C ))
#define 	SHA_INTE		        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x10 ))
#define 	SHA_INTS		        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x14 ))
#define 	SHA_SADDR		        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x18 ))
#define 	SHA_CUR_SAR		        				       			        ((volatile unsigned  int*)( SHA_BASEADDR + 0x1C ))
#define 	SHA_LINE_NUM	        				       			                ((volatile unsigned  int*)( SHA_BASEADDR + 0x20 ))
#define 	SHA_W0			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x24 ))
#define 	SHA_W1			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x28 ))
#define 	SHA_W2			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x2C ))
#define 	SHA_W3			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x30 ))
#define 	SHA_W4			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x34 ))
#define 	SHA_W5			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x38 ))
#define 	SHA_W6			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x3C ))
#define 	SHA_W7			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x40 ))
#define 	SHA_W8			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x44 ))
#define 	SHA_W9			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x48 ))
#define 	SHA_W10			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x4C ))
#define 	SHA_W11			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x50 ))
#define 	SHA_W12			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x54 ))
#define 	SHA_W13			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x58 ))
#define 	SHA_W14			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x5C ))
#define 	SHA_W15			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x60 ))
#define 	SHA_H0			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x64 ))
#define 	SHA_H1			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x68 ))
#define 	SHA_H2			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x6C ))
#define 	SHA_H3			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x70 ))
#define 	SHA_H4			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x74 ))
#define 	SHA_H5			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x78 ))
#define 	SHA_H6			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x7C ))
#define 	SHA_H7			        					       			((volatile unsigned  int*)( SHA_BASEADDR + 0x80 ))


extern unsigned	 int  ORIGIN_IMAGE_LEN;
extern unsigned  int  RSASIGNATURE[256/4];
extern unsigned  int  RSAPUBKEYSTRU[524/4];
extern unsigned  char  *ORIGIN_IMAGE_BASEADDR;

extern void dumphex(char *s, unsigned char *pdata, unsigned int len);
extern void dumpint(char *s, unsigned int *pdata, unsigned int len);

#endif
