#include <common.h>
#include <asm/io.h>
#include <asm/armv7.h>
//#include "firmware.h"

#include "pubh.h"

#include "rsa.h"
#include "sha256.h"

unsigned  int  ORIGIN_IMAGE_LEN;
unsigned  int  RSASIGNATURE[256/4];
unsigned  int  RSAPUBKEYSTRU[524/4];
unsigned  char ORIGIN_IMAGE_SHA[32];
unsigned  int  updateNum;
unsigned  char *ORIGIN_IMAGE_BASEADDR;

#define SECURITY_BASEADDR	0xA0820000
#define	RSA_SIGNATURE0	((volatile unsigned  int*)( SECURITY_BASEADDR + 0x234 ))
#define	RSA_SIGNATURE1	((volatile unsigned  int*)( SECURITY_BASEADDR + 0x238 ))
#define	RSA_SIGNATURE2	((volatile unsigned  int*)( SECURITY_BASEADDR + 0x23C ))
#define	RSA_SIGNATURE3	((volatile unsigned  int*)( SECURITY_BASEADDR + 0x240 ))
#define	RSA_SIGNATURE4	((volatile unsigned  int*)( SECURITY_BASEADDR + 0x244 ))
#define	RSA_SIGNATURE5	((volatile unsigned  int*)( SECURITY_BASEADDR + 0x248 ))
#define	RSA_SIGNATURE6	((volatile unsigned  int*)( SECURITY_BASEADDR + 0x24C ))
#define	RSA_SIGNATURE7	((volatile unsigned  int*)( SECURITY_BASEADDR + 0x250 ))


unsigned int rsahash[]={
	0x02cd0b91,
	0x031c5ed1,
	0x46774c03,
	0xa61b5fc5,
	0x5fda323b,
	0x2025e2ab,
	0xb2eabfd8,
	0x7f302dd2,

};

extern void enable_icache(void);

/****************************************************
                Use SHA256 to generate digest of RSA pub-key, 
                which is 524 BYTES. Then compare the digest 
                with the original digest which is store in the
                EFUSE. If the new digest equals original digest,
                it means RSA pub-key is right. Otherwise, means
                the RSA pub-key is wrong. 
*****************************************************/
int rsaPubKey_sha256_verify(void)
{
		unsigned int *digest, origDigest[8], i, result;
		unsigned char *newDigest;
		SHA256_CTX ctx;
		digest = (unsigned int *)SHA256_hash(RSAPUBKEYSTRU, 524, newDigest, &ctx);
#if 1
		origDigest[0] = *RSA_SIGNATURE0;
		origDigest[1] = *RSA_SIGNATURE1;
		origDigest[2] = *RSA_SIGNATURE2;
		origDigest[3] = *RSA_SIGNATURE3;
		origDigest[4] = *RSA_SIGNATURE4;
		origDigest[5] = *RSA_SIGNATURE5;
		origDigest[6] = *RSA_SIGNATURE6;
		origDigest[7] = *RSA_SIGNATURE7;
#else
		origDigest[0] = rsahash[0];
		origDigest[1] = rsahash[1];
		origDigest[2] = rsahash[2];
		origDigest[3] = rsahash[3];
		origDigest[4] = rsahash[4];
		origDigest[5] = rsahash[5];
		origDigest[6] = rsahash[6];
		origDigest[7] = rsahash[7];
#endif
#if defined(CONFIG_ENABLE_SECURE_VERIFY_DEBUG)
		dumphex("pubkey hash", digest, 32);
		dumphex("read pubkey hash", origDigest, 32);
#endif
		//new key and old key xor
#if defined(CONFIG_ENABLE_SECURE_VERIFY_DEBUG)
		printf("read pubkey hash:\n");
#endif
		for(i=0; i<8; i++)
		{
#if defined(CONFIG_ENABLE_SECURE_VERIFY_DEBUG)
			printf("%08x ", origDigest[i]);
#endif
			result = ((origDigest[i]) ^ (digest[i]));
			if(result != 0) {
#if defined(CONFIG_ENABLE_SECURE_VERIFY_DEBUG)
				printf("ERROR! %s %d\n", __func__,__LINE__);
#endif
				return 1;
			}
		}

#if defined(CONFIG_ENABLE_SECURE_VERIFY_DEBUG)
                dumphex("pubkey hash", digest, 32);
#endif
                return 0;
}


/******************************************************
                Let image to do RSA verify. If verify OK,
                return 0. Otherwise, return 1.
*******************************************************/
int image_rsa_verify(void)
{
                unsigned int value, i;
                unsigned char *image_sha256;
                unsigned char *signature;
                RSAPublicKey *public_key;
                SHA256_CTX ctx;

                updateNum = 0;
                value = rsaPubKey_sha256_verify();
                if(value == 1)
                        return 1;

                updateNum = 0;
                image_sha256 = (unsigned char*)SHA256_hash(ORIGIN_IMAGE_BASEADDR, ORIGIN_IMAGE_LEN, image_sha256, &ctx);

#if defined(CONFIG_ENABLE_SECURE_VERIFY_DEBUG)
                dumphex("current image hash", uboot_sha256, 32);
#endif

                for(i=0; i<32; i++)
                        ORIGIN_IMAGE_SHA[i] = image_sha256[i];

                updateNum = 0;
                signature = (unsigned char*)RSASIGNATURE;
                public_key = (RSAPublicKey *)RSAPUBKEYSTRU;
                value = RSA_verify(public_key, signature, 256, ORIGIN_IMAGE_SHA, 32);

                if(value == 0)
                        return 1;
                return 0;
}
