#include <common.h>
#include <asm/io.h>
#include <asm/armv7.h>

#include "pubh.h"

#define		HEADINFOLEN        (512 * 3)
#define		RSASIGNEDLEN         512
#define		RSAPUBKEYLEN         768

#define         VERIFY_KERNEL         1
#define         VERIFY_RAMDISK        2

unsigned char *image_data_all;


void dumpint(char *s, unsigned int *pdata, unsigned int len)
{
        int i;
        printf("\nstart dump %s(%p-%d):\n", s, pdata, len);
        for (i=0; i<len; i++) {
                if (!(i%4)) {
                        printf("\n[%2d]:", i);
                }
                printf("%u ", pdata[i]);
        }
        printf("\ndump end\n");
}

void dumphex(char *s, unsigned char *pdata, unsigned int len)
{
        int i;
        printf("\nstart dump %s(%p-%d):\n", s, pdata, len);
        for (i=0; i<len; i++) {
                if (!(i%4)) {
                        printf("\n[%2d]:", i);
                }
                printf("%02x ", pdata[i]);
        }
        printf("\ndump end\n");
}

void getverifyimage(int whichimage)
{
        int i;
        unsigned int *cfgInfor = image_data_all;
        int ret;

        if(whichimage == VERIFY_KERNEL) {
                image_data_all = CONFIG_KERNEL_LOADADDR - HEADINFOLEN;
        } else if(whichimage == VERIFY_RAMDISK) {
                image_data_all = CONFIG_RAMDISK_LOADADDR - HEADINFOLEN;
        }
        cfgInfor = (unsigned int *)image_data_all;
        ORIGIN_IMAGE_LEN =  cfgInfor[0];

        for(i=0; i<(256/4); i++)
        {
                RSASIGNATURE[i] = cfgInfor[i + (RSASIGNEDLEN / 4)];
        }

        for(i=0; i<(524/4); i++)
        {
                RSAPUBKEYSTRU[i] = cfgInfor[i + (RSAPUBKEYLEN / 4)];
	}

        ORIGIN_IMAGE_BASEADDR = &image_data_all[HEADINFOLEN];

#if defined(CONFIG_ENABLE_SECURE_VERIFY_DEBUG)
        printf("image len:0x%x(%d)\n", ORIGIN_IMAGE_LEN, ORIGIN_IMAGE_LEN);

        dumphex("rsa pub key", RSAPUBKEYSTRU, 524/4);
        dumpint("rsa pub key", RSAPUBKEYSTRU, 524/4);
#endif
}

int secure_verify(void)
{
        getverifyimage(VERIFY_KERNEL);
        if (image_rsa_verify()) {
                printf("kernel verify failed!\n");
                return 1;
        } else {
                printf("kernel verify ok!\n");
                getverifyimage(VERIFY_RAMDISK);
                if(image_rsa_verify()) {
                        printf("ramdisk verify failed!\n");
                        return 1;
                } else {
                        printf("ramdisk verify ok!\n");
                }
        }
        return 0;
}
