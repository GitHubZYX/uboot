/* sha256.c
**
** Copyright 2013, The Android Open Source Project
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of Google Inc. nor the names of its contributors may
**       be used to endorse or promote products derived from this software
**       without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY Google Inc. ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
** EVENT SHALL Google Inc. BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
** OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
** OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
** ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Optimized for minimal code size.

#include "pubh.h"

#include "sha256.h"

//#include <stdio.h>
//#include <string.h>
//#include <stdint.h>

//#include "lc1860.h"

#define ror(value, bits) (((value) >> (bits)) | ((value) << (32 - (bits))))
#define shr(value, bits) ((value) >> (bits))

extern unsigned int  updateNum;
extern unsigned int  IMAGE_SHA_START;

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };

static void SHA256_Transform(SHA256_CTX* ctx) {
        uint32_t W[16];
        uint8_t* p = ctx->buf;
        int t;

        for(t = 0; t < 16; ++t) {
                uint32_t tmp =  *p++ << 24;
                tmp |= *p++ << 16;
                tmp |= *p++ << 8;
                tmp |= *p++;
                W[t] = tmp;
        }


            *SHA_W0 = W[0];
            *SHA_W1 = W[1];
            *SHA_W2 = W[2];
            *SHA_W3 = W[3];
            *SHA_W4 = W[4];
            *SHA_W5 = W[5];
            *SHA_W6 = W[6];
            *SHA_W7 = W[7];
            *SHA_W8 = W[8];
            *SHA_W9 = W[9];
            *SHA_W10 = W[10];
            *SHA_W11 = W[11];
            *SHA_W12 = W[12];
            *SHA_W13 = W[13];
            *SHA_W14 = W[14];
            *SHA_W15 = W[15];

        if(updateNum == 0)
                *SHA_CTL = 1; //SHA start
        else
                *SHA_CTL = 2; //SHA restart

        while(*SHA_INTR == 0);//wait for caculate end

        *SHA_INTS = 1; //clr interrupt

        updateNum ++;
}

static const HASH_VTAB SHA256_VTAB = {
    SHA256_init,
    SHA256_update,
    SHA256_final,
    SHA256_hash,
    SHA256_DIGEST_SIZE
};

void SHA256_init(SHA256_CTX* ctx) {
    ctx->f = &SHA256_VTAB;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->count = 0;
}


void SHA256_update(SHA256_CTX* ctx, const void* data, int len) {
    int i = (int) (ctx->count & 63);
    const uint8_t* p = (const uint8_t*)data;

    ctx->count += len;

    *SHA_MOD = 1; 	// set SHA256 mode
    *SHA_IN_TYPE = 0;   //enable CPU input mode

    while (len--) {
        ctx->buf[i++] = *p++;
        if (i == 64) {
            SHA256_Transform(ctx);
            i = 0;
        }
    }
}

const uint8_t* SHA256_final(SHA256_CTX* ctx) {
    uint8_t *p = ctx->buf;
    uint64_t cnt = ctx->count * 8;
    int i;
    unsigned int sha256_result[8];

    SHA256_update(ctx, (uint8_t*)"\x80", 1);
    while ((ctx->count & 63) != 56) {
        SHA256_update(ctx, (uint8_t*)"\0", 1);
    }
    for (i = 0; i < 8; ++i) {
        uint8_t tmp = (uint8_t) (cnt >> ((7 - i) * 8));
        SHA256_update(ctx, &tmp, 1);
    }

    sha256_result[0] = *SHA_H0;
    sha256_result[1] = *SHA_H1;
    sha256_result[2] = *SHA_H2;
    sha256_result[3] = *SHA_H3;
    sha256_result[4] = *SHA_H4;
    sha256_result[5] = *SHA_H5;
    sha256_result[6] = *SHA_H6;
    sha256_result[7] = *SHA_H7;

    for (i = 0; i < 8; i++) {
    uint32_t tmp = sha256_result[i];
    *p++ = tmp >> 24;
    *p++ = tmp >> 16;
    *p++ = tmp >> 8;
    *p++ = tmp >> 0;
    }

    return ctx->buf;
}


/* Convenience function */
const uint8_t* SHA256_hash(void* data, int len, const uint8_t* digest, SHA256_CTX *ctx) {
    SHA256_init(ctx);
    SHA256_update(ctx, data, len);
    digest = SHA256_final(ctx);
    return digest;
}
