#include <common.h>
#include <asm/io.h>

extern int ddr_frequency_adjust(int select_low_fren);

//#define CONFIG_DDR_MEMTEST_NEW		1

#define WATCH_GPIO_INIT()	//gpio192_init()
#define WATCH_GPIO_SET(level)	//gpio192_out(level)
#define WATCH_GPIO_NOTIFY()	//gpio192_notify()
#define WATCH_GPIO_DIE()	//gpio192_out(1)

void system_die(void)
{
	WATCH_GPIO_DIE();
	printf("system die, waiting...\n");
	while(1);
}
//#define BREAKWHENERROR
#ifdef BREAKWHENERROR
#define CHECKRET (ret == 0)
#else
#define CHECKRET (1)
#endif

static void copy64(unsigned long long *src, unsigned long long *dest)
{
	*dest = *src;
}

/*
 * This is 64 bit wide test patterns.  Note that they reside in ROM
 * (which presumably works) and the tests write them to RAM which may
 * not work.
 *
 * The "otherpattern" is written to drive the data bus to values other
 * than the test pattern.  This is for detecting floating bus lines.
 *
 */

const static unsigned long long testdata[] = {
	0xaaaaaaaaaaaaaaaaULL,
	0xccccccccccccccccULL,
	0xf0f0f0f0f0f0f0f0ULL,
	0xff00ff00ff00ff00ULL,
	0xffff0000ffff0000ULL,
	0xffffffff00000000ULL,
	0x00000000ffffffffULL,
	0x0000ffff0000ffffULL,
	0x00ff00ff00ff00ffULL,
	0x0f0f0f0f0f0f0f0fULL,
	0x3333333333333333ULL,
	0x5555555555555555ULL
};

const unsigned long long othertestdata = 0x0123456789abcdefULL;


/* 数据线检测 */
static int memory_test_dataline(unsigned long long * pmem)
{

	unsigned long long temp64 = 0;
	int num_testdatas = sizeof(testdata)/ sizeof(testdata[0]);
	int i;
	unsigned int hi, lo, pathi, patlo;
	int ret = 0;

	for ( i = 0; i < num_testdatas; i++) 
	{
		copy64((unsigned long long *)&(testdata[i]), pmem++);
		/*
		* Put a different pattern on the data lines: otherwise they
		* may float long enough to read back what we wrote.
		*/
		/* 预防floating buses错误 */

		copy64((unsigned long long *)&othertestdata, pmem--);
		copy64(pmem, &temp64);


#ifdef INJECT_DATA_ERRORS
		temp64 ^= 0x00008000;
#endif
	 

		if (temp64 != testdata[i])

		{

			pathi = (testdata[i]>>32) & 0xffffffff;

			patlo = testdata[i] & 0xffffffff;

			hi = (temp64>>32) & 0xffffffff;

			lo = temp64 & 0xffffffff;

			printf ("Memory (date line) error at %08x, "

			 "wrote %08x%08x, read %08x%08x !\n",

			 (unsigned int)pmem, pathi, patlo, hi, lo);

			ret = -1;

		}

	}

	return ret;

}

 

/* 地址线检测 */

static int memory_test_addrline(ulong *testaddr, ulong *base, ulong size)

{
	ulong *target;
	ulong *end;
	ulong readback;
	ulong xor;
	int   ret = 0;
	 
	end = (ulong *)((ulong)base + size);/* pointer arith! */
	xor = 0;

	for(xor = sizeof(ulong); xor > 0; xor <<= 1) 
	{
		/* 对测试的地址的某一根地址线的值翻转 */
		target = (ulong *)((ulong)testaddr ^ xor);

		if((target >= base) && (target < end)) 
		{

			/* 由于target是testaddr某一根地址线的值翻转得来

			   故testaddr != target,下面赋值操作后

			   应有*testaddr != *target */

			*testaddr = ~*target;

			readback  = *target;

			 

#ifdef INJECT_ADDRESS_ERRORS

			if(xor == 0x00008000) 

			{

				readback = *testaddr;

			}

#endif

			                     /* 出现此种情况只有testaddr == target,即某根地址线翻转无效 */

			if(readback == *testaddr) 

			{

				printf ("Memory (address line) error at %08x<->%08x, "

				 "XOR value %08lx !\n",

				(unsigned int)testaddr, (unsigned int)target, xor);

				ret = -1;

			}

		}

	}

	return ret;

}

 

static int memory_test_test1 (unsigned long start,

     unsigned long size,

     unsigned long val)

{

	unsigned long i;

	ulong *mem = (ulong *) start;

	ulong readback;

	int ret = 0;

	 

	for (i = 0; i < size / sizeof (ulong); i++) {

		mem[i] = val;

	}

 

	for (i = 0; i < size / sizeof (ulong) && CHECKRET; i++) {

		readback = mem[i];

		if (readback != val) {

			printf ("Memory error at %p, "

			 "wrote %08lx, read %08lx !\n",

			 mem + i, val, readback);

			 

			ret = -1;

#ifdef BREAKWHENERROR
			break;
#endif

		}


	}

	return ret;

}

 

static int memory_test_test2 (unsigned long start, unsigned long size)

{

	unsigned long i;

	ulong *mem = (ulong *) start;

	ulong readback;

	int ret = 0;


	for (i = 0; i < size / sizeof (ulong); i++) {

		mem[i] = 1 << (i % 32);

	}


	for (i = 0; i < size / sizeof (ulong) && CHECKRET; i++) {

		readback = mem[i];

		if (readback != (1 << (i % 32))) {

			printf ("Memory error at %p, "

			 "wrote %08x, read %08lx !\n",

			 mem + i, 1 << (i % 32), readback);

			ret = -1;

#ifdef BREAKWHENERROR
			break;
#endif

		}

	}


	return ret;

}

 

static int memory_test_test3 (unsigned long start, unsigned long size)

{

	unsigned long i;

	ulong *mem = (ulong *) start;

	ulong readback;

	int ret = 0;

	 

	for (i = 0; i < size / sizeof (ulong); i++) {

		mem[i] = i;

	}

	 

	for (i = 0; i < size / sizeof (ulong) && CHECKRET; i++) {

		readback = mem[i];

		if (readback != i) {

			printf ("Memory error at %p, "

			 "wrote %08lx, read %08lx !\n",

			 mem + i, i, readback);


			ret = -1;

#ifdef BREAKWHENERROR
			break;
#endif

		}


	}

	return ret;

}

 

static int memory_test_test4 (unsigned long start, unsigned long size)

{

	unsigned long i;

	ulong *mem = (ulong *) start;

	ulong readback;

	int ret = 0;


	for (i = 0; i < size / sizeof (ulong); i++) {

		mem[i] = ~i;

	}


	for (i = 0; i < size / sizeof (ulong) && CHECKRET; i++) {

		readback = mem[i];

		if (readback != ~i) {

			printf ("Memory error at %p, "

			 "wrote %08lx, read %08lx !\n",

			 mem + i, ~i, readback);

			ret = -1;

#ifdef BREAKWHENERROR
			break;
#endif

		}

	}



	return ret;

}

 

static int memory_test_lines (unsigned long start, unsigned long size)

{

	int ret = 0;

	ret = memory_test_dataline ((unsigned long long *)start);

	ret += memory_test_addrline ((ulong *)start, (ulong *)start, size);

	ret += memory_test_addrline ((ulong *)(start + size - 8),

		   (ulong *)start, size);


	return ret;

}

static int memory_test_patterns(unsigned long start, unsigned long size)
{
	int ret = 0;

	printf("0x%lx, 0x%lx ,test:0x00000000\n",start, size);
	ret = memory_test_test1 (start, size, 0x00000000);
	WATCH_GPIO_NOTIFY();

	printf("0x%lx, 0x%lx ,test:0xffffffff\n",start, size);
	ret += memory_test_test1 (start, size, 0xffffffff);
	WATCH_GPIO_NOTIFY();

	printf("0x%lx, 0x%lx ,test:0x55555555\n",start, size);
	ret += memory_test_test1 (start, size, 0x55555555);
	WATCH_GPIO_NOTIFY();


	printf("0x%lx, 0x%lx ,test:0xaaaaaaaa\n",start, size);
	ret += memory_test_test1 (start, size, 0xaaaaaaaa);
	WATCH_GPIO_NOTIFY();


	printf("memory_test_test2: 0x%lx, 0x%lx\n",start, size);
	ret += memory_test_test2 (start, size);
	WATCH_GPIO_NOTIFY();


	printf("memory_test_test3: 0x%lx, 0x%lx\n",start, size);
	ret += memory_test_test3 (start, size);
	WATCH_GPIO_NOTIFY();


	printf("memory_test_test4: 0x%lx, 0x%lx\n",start, size);
	ret += memory_test_test4 (start, size);
	WATCH_GPIO_NOTIFY();

	return ret;
}

#if 0
static int memory_test_regions(unsigned long start, unsigned long size)
{
	unsigned long i;
	int ret = 0;

	for (i = 0; i < (size >> 20); i++) {

		ret += memory_test_patterns(start + (i << 20),
			0x800);
		ret += memory_test_patterns(start + (i << 20) +
			0xff800, 0x800);
	}

	return ret;
}
#endif

typedef unsigned int ul;
typedef unsigned int  ulv;
typedef unsigned char  volatile u8v;
typedef unsigned short  volatile u16v;
#define UL_LEN 32
#define UL_ONEBITS 0xffffffff
#define CHECKERBOARD1 0x55555555
#define CHECKERBOARD2 0xaaaaaaaa
#define UL_BYTE(x) ((x | x << 8 | x << 16 | x << 24))
#define ONE 0x00000001L
struct test {
    char *name;
    int (*fp)();
};

union {
    unsigned char bytes[UL_LEN/8];
    ul val;
} mword8;

union {
    unsigned short u16s[UL_LEN/16];
    ul val;
} mword16;

unsigned int rand( void );           /* returns a random 32-bit integer */

/* return a random float >= 0 and < 1 */
#define rand_float          ((double)rand() / 4294967296.0)

static unsigned int SEED_X = 521288629;
static unsigned int SEED_Y = 362436069;


unsigned int rand ()
{
	static unsigned int a = 18000, b = 30903;
	unsigned int seed1, seed2;
	unsigned int seed_h;
	unsigned int seed_l;
	unsigned int ret = 0;
	int i, j;
	for(i = 0; i < 4; i++){
		seed_h = *(volatile u32*)(TPZCTL_BASE + 0x50);
		for(j = 0; j < 1; j++);
		seed1 |= (seed_h << (i * 8));
	}
	
	for(i = 0; i < 4; i++){
		seed_l = *(volatile u32*)(TPZCTL_BASE + 0x50);
		for(j = 0; j < 1; j++);
		seed2 |= (seed_l << (i * 8));
	}
	
	
	if (seed1) SEED_X = seed1;   /* use default seeds if parameter is 0 */
	if (seed2) SEED_Y = seed2;

	SEED_X = a*(SEED_X&65535) + (SEED_X>>16);
	SEED_Y = b*(SEED_Y&65535) + (SEED_Y>>16);
	ret = ((SEED_X<<16) + (SEED_Y&65535));
	return ret;
}

#define rand32() ((unsigned int) rand() | ( (unsigned int) rand() << 16))
#define rand_ul() rand32()

int compare_regions(ulv *bufa, ulv *bufb, size_t count) {
    int r = 0;
    size_t i;
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    for (i = 0; i < count; i++, p1++, p2++) {
        if (*p1 != *p2) {

                printf("FAILURE: 0x%08lx != 0x%08lx at physical address add1:0x%x, add2:0x%x\n", 
                        (ul) *p1, (ul) *p2, p1, p2);

            /* printf("Skipping to next test..."); */
            r = -1;
        }
    }
    return r;
}

int test_stuck_address(ulv *bufa, size_t count) {
    ulv *p1 = bufa;
    unsigned int j;
    size_t i;
	
    printf("start address is 0%x\n", bufa);
    printf("RAM size is 0%x\n", count);
    printf("test_stuck_address\n");

    for (j = 0; j < 4; j++) {
        p1 = (ulv *) bufa;
        for (i = 0; i < count; i++) {
            *p1 = ((j + i) % 2) == 0 ? (ul) p1 : ~((ul) p1);
            *p1++;
        }
        
	p1 = (ulv *) bufa;
		
        for (i = 0; i < count; i++, p1++) {
            if (*p1 != (((j + i) % 2) == 0 ? (ul) p1 : ~((ul) p1))) {
                    printf( 
                            "FAILURE: possible bad address line at physical "
                            "address 0x%x.\n", 
                            p1);
			printf("read p1 is 0x%x, calc data is 0x%x calc data ~ is 0x%x\n", *p1, 
				p1 ,~((ul) p1));

                printf("Skipping to next test...\n");
                return -1;
            }
        }
    }
    return 0;
}
int test_random_value(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    ul j = 0;
    size_t i;
    
    printf("test_random_value\n");
    for (i = 0; i < count; i++) {
	    *p1++ = *p2++ = rand_ul(); 
    }
    return compare_regions(bufa, bufb, count);
}

int test_xor_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    
    printf("test_xor_comparison\n");
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ ^= q;
        *p2++ ^= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_sub_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();
	
    printf("test_sub_comparison\n");
    for (i = 0; i < count; i++) {
        *p1++ -= q;
        *p2++ -= q;
    }
    return compare_regions(bufa, bufb, count);
}
int test_mul_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();
    
    printf("test_mul_comparison\n");
    for (i = 0; i < count; i++) {
        *p1++ *= q;
        *p2++ *= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_div_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();
    
    printf("test_div_comparison\n");
    for (i = 0; i < count; i++) {
        if (!q) {
            q++;
        }
        *p1++ /= q;
        *p2++ /= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_or_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();
    
    printf("test_or_comparison\n");
    for (i = 0; i < count; i++) {
        *p1++ |= q;
        *p2++ |= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_and_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();
   
    printf("test_and_comparison\n");

    for (i = 0; i < count; i++) {
        *p1++ &= q;
        *p2++ &= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_seqinc_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();
    
    printf("test_seqinc_comparison\n");
    for (i = 0; i < count; i++) {
        *p1++ = *p2++ = (i + q);
    }
    return compare_regions(bufa, bufb, count);
}

int test_solidbits_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    ul q;
    size_t i;

    printf("test_solidbits_comparison\n");
    for (j = 0; j < 2; j++) {
        q = (j % 2) == 0 ? UL_ONEBITS : 0;
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
        }
        
	if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    return 0;
}
int test_checkerboard_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    ul q;
    size_t i;

    printf("test_checkerboard_comparison\n");
    for (j = 0; j < 2; j++) {
        q = (j % 2) == 0 ? CHECKERBOARD1 : CHECKERBOARD2;
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
        }
        
	if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    return 0;
}
int test_blockseq_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    printf("test_blockseq_comparison\n");
    for (j = 0; j < 2; j++) {
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (ul) UL_BYTE(j);
        }
        
	if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    return 0;
}
int test_walkbits0_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    printf("test_walkbits0_comparison\n");
    for (j = 0; j < 2; j++) {
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = ONE << j;
            } else { /* Walk it back down. */
                *p1++ = *p2++ = ONE << (UL_LEN * 2 - j - 1);
            }
        }
        
	if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    return 0;
}
int test_walkbits1_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    printf("test_walkbits1_comparison\n");
    for (j = 0; j < 2; j++) {
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = UL_ONEBITS ^ (ONE << j);
            } else { /* Walk it back down. */
                *p1++ = *p2++ = UL_ONEBITS ^ (ONE << (UL_LEN * 2 - j - 1));
            }
        }
        
	if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    return 0;
}
int test_bitspread_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    printf("test_bitspread_comparison\n");
    for (j = 0; j < 2; j++) {
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = (i % 2 == 0)
                    ? (ONE << j) | (ONE << (j + 2))
                    : UL_ONEBITS ^ ((ONE << j)
                                    | (ONE << (j + 2)));
            } else { /* Walk it back down. */
                *p1++ = *p2++ = (i % 2 == 0)
                    ? (ONE << (UL_LEN * 2 - 1 - j)) | (ONE << (UL_LEN * 2 + 1 - j))
                    : UL_ONEBITS ^ (ONE << (UL_LEN * 2 - 1 - j)
                                    | (ONE << (UL_LEN * 2 + 1 - j)));
            }
        }
        
	if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    return 0;
}
int test_bitflip_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j, k;
    ul q;
    size_t i;

    printf("test_bitflip_comparison\n");
    for (k = 0; k < 2; k++) {
        q = ONE << k;
        for (j = 0; j < 8; j++) {
            q = ~q;
            p1 = (ulv *) bufa;
            p2 = (ulv *) bufb;
            for (i = 0; i < count; i++) {
                *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
            }
            
	    if (compare_regions(bufa, bufb, count)) {
                return -1;
            }
        }
    }
    return 0;
}
int test_8bit_wide_random(ulv* bufa, ulv* bufb, size_t count) {
    u8v *p1, *t;
    ulv *p2;
    int attempt;
    unsigned int b, j = 0;
    size_t i;
    
    printf("test_8bit_wide_random\n");
    for (attempt = 0; attempt < 2;  attempt++) {
        if (attempt & 1) {
            p1 = (u8v *) bufa;
            p2 = bufb;
        } else {
            p1 = (u8v *) bufb;
            p2 = bufa;
        }
        for (i = 0; i < count; i++) {
            t = mword8.bytes;
            *p2++ = mword8.val = rand_ul();
            for (b=0; b < UL_LEN/8; b++) {
                *p1++ = *t++;
            }
        }
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    return 0;
}

int test_16bit_wide_random(ulv* bufa, ulv* bufb, size_t count) {
    u16v *p1, *t;
    ulv *p2;
    int attempt;
    unsigned int b, j = 0;
    size_t i;
    
    printf("test_16bit_wide_random\n");
    for (attempt = 0; attempt < 2; attempt++) {
        if (attempt & 1) {
            p1 = (u16v *) bufa;
            p2 = bufb;
        } else {
            p1 = (u16v *) bufb;
            p2 = bufa;
        }
        for (i = 0; i < count; i++) {
            t = mword16.u16s;
            *p2++ = mword16.val = rand_ul();
            for (b = 0; b < UL_LEN/16; b++) {
                *p1++ = *t++;
            }
        }
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    return 0;
}

void memory_test_bank(u32 vstart, u32 memsize)
{
	int ret = 0;
	int seed1, seed2;

	printf("start memory test...\n");
	u32 memsize_b;
	ulv* vstart_2;

	vstart_2 = (ulv*)(vstart + memsize / 2);
	memsize_b = memsize /(2 * sizeof(u32));

#ifdef CONFIG_DDR_MEMTEST_NEW
	*(volatile u32*)(AP_PWR_SECPCLK_EN) |= (0x00100010);
	if(test_stuck_address((ulv*)vstart, (memsize_b * 2)))
		goto  test_failed;

	if(test_random_value((ulv*)vstart, vstart_2, memsize_b))
		goto  test_failed;

	if(test_xor_comparison((ulv*)vstart, vstart_2, memsize_b))
		goto  test_failed;

	if(test_sub_comparison((ulv*)vstart, vstart_2, memsize_b))
		goto  test_failed;

	if(test_mul_comparison((ulv*)vstart, vstart_2, memsize_b))
		goto  test_failed;

	if(test_div_comparison((ulv*)vstart, vstart_2, memsize_b))
		goto  test_failed;

	if(test_or_comparison((ulv*)vstart, vstart_2, memsize_b))
		goto  test_failed;

	if(test_and_comparison((ulv*)vstart, vstart_2, memsize_b))
		goto  test_failed;

	if(test_seqinc_comparison((ulv*)vstart, vstart_2, memsize_b))
		goto  test_failed;

	if(test_solidbits_comparison((ulv*)vstart, vstart_2, memsize_b))
		goto  test_failed;

	if(test_checkerboard_comparison((ulv*)vstart, vstart_2, memsize_b))
		goto  test_failed;

	if(test_blockseq_comparison((ulv*)vstart, vstart_2, memsize_b))
		goto  test_failed;

	if(test_walkbits0_comparison((ulv*)vstart, vstart_2, memsize_b))
		goto  test_failed;

	if(test_walkbits1_comparison((ulv*)vstart, vstart_2, memsize_b))
		goto  test_failed;

	if(test_bitspread_comparison((ulv*)vstart, vstart_2, memsize_b))
		goto  test_failed;

	if(test_bitflip_comparison((ulv*)vstart, vstart_2, memsize_b))
		goto  test_failed;

	if(test_8bit_wide_random((ulv*)vstart, vstart_2, memsize_b))
		goto  test_failed;

	if(test_16bit_wide_random((ulv*)vstart, vstart_2, memsize_b))
		goto  test_failed;
#endif

	if(memory_test_lines(vstart, memsize))
		goto  test_failed;

	if(memory_test_patterns(vstart, memsize))
		goto  test_failed;

	printf("test finish:OK!\n");
	//pmic_power_off();
	return;
test_failed:
	printf("test failed\n");
	while(1);
}

void memory_test(void)
{
	int i=0;

	while(++i < 10) {
		printf("memory test %d\n",i);
		ddr_frequency_adjust(i%2);

		if (gd->dram_size > 0) {
			printf("memory test ddr0\n");
			memory_test_bank(gd->dram_base, gd->dram_size);
		}

		if (gd->dram2_size > 0) {
			printf("memory test ddr1\n");
			memory_test_bank(gd->dram2_base, gd->dram2_size);
		}

	};
	pmic_reboot();
}
