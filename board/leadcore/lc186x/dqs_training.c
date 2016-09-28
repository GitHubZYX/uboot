/*
 * dqs_test.c
 *
 *  Created on: 2014-3-6
 *      Author: bjld
 */
#include <common.h>
#include "memctl_inits.h"

DECLARE_GLOBAL_DATA_PTR;

//#define	DDR_TRAINING_DEBUG_LOG
DECLARE_GLOBAL_DATA_PTR;
//#define  DDR_TRAINING_WB_LOG 1

extern int memctl_refresh_dll_lock_value(unsigned int freq);
static unsigned int bist_test(int ddr_id, int slice, unsigned int offset);
static	void training_record_filter(unsigned int training_record[4][256], unsigned int *training_result);

unsigned int write_dqs[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x1};
unsigned int write_dq[5]  = {0x40, 0x40, 0x40, 0x40, 0x40};
unsigned int read_dqs[5]  = {0x40, 0x40, 0x40, 0x40, 0x40};

unsigned int write_dq_standard_value[5]  = {0x39, 0x3c, 0x3c, 0x42, 0x40};
unsigned int read_dqs_standard_value[5]  = {0x51, 0x4a, 0x4b, 0x4d, 0x40};
unsigned int f0_ddr0_gate_training_coarse_delay[5] = {0x0, 0x0, 0x0, 0x0, 0x1};
unsigned int f0_ddr1_gate_training_coarse_delay[5] = {0x0, 0x0, 0x1, 0x0, 0x0};
unsigned int f1_ddr0_gate_training_coarse_delay[5] = {0x1, 0x0, 0x0, 0x0, 0x0};
unsigned int f1_ddr1_gate_training_coarse_delay[5] = {0x1, 0x0, 0x0, 0x0, 0x0};
unsigned int f0_ddr0_gate_training_fine_delay[5] = {0x0, 0x0, 0x1, 0x0, 0x0};
unsigned int f0_ddr1_gate_training_fine_delay[5] = {0x0, 0x0, 0x1, 0x0, 0x0};
unsigned int f1_ddr0_gate_training_fine_delay[5] = {0x0, 0x1, 0x0, 0x0, 0x0};
unsigned int f1_ddr1_gate_training_fine_delay[5] = {0x0, 0x1, 0x0, 0x0, 0x0};

#if defined(CONFIG_GATE_TRAINING_SUPPORT)
#define CNT_G	(4)
/* Include gate_training. */
struct memctl_data memctl0_dqs_gate_training[] = {
	// freq 0
	{DDR_CFG_BASE_0 + DENALI_PHY_19, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_0 + DENALI_PHY_04, 0}, //( DDR_PHY_BASE + 0x10 + freq* 0x1c + i*32*4)
	{DDR_CFG_BASE_0 + DENALI_PHY_05, 0}, //( DDR_PHY_BASE + 0x14 + freq* 0x1c + 32*4*k )
	{DDR_CFG_BASE_0 + DENALI_PHY_02, 0}, //( DDR_PHY_BASE + 0x08 + freq* 0x1c + 32*4*k )

	{DDR_CFG_BASE_0 + DENALI_PHY_19 + 32*4*1, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 32*4*1, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 32*4*1, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_02 + 32*4*1, 0},

	{DDR_CFG_BASE_0 + DENALI_PHY_19 + 32*4*2, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 32*4*2, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 32*4*2, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_02 + 32*4*2, 0},

	{DDR_CFG_BASE_0 + DENALI_PHY_19 + 32*4*3, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 32*4*3, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 32*4*3, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_02 + 32*4*3, 0},

	{DDR_CFG_BASE_0 + DENALI_PHY_19 + 32*4*4, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 32*4*4, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 32*4*4, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_02 + 32*4*4, 0},

	// freq 1
	{DDR_CFG_BASE_0 + DENALI_PHY_20, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 0x1c, 0}, //( DDR_PHY_BASE + 0x10 + freq* 0x1c + i*32*4)
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 0x1c, 0}, //( DDR_PHY_BASE + 0x14 + freq* 0x1c + 32*4*k )
	{DDR_CFG_BASE_0 + DENALI_PHY_02 + 0x1c, 0}, //( DDR_PHY_BASE + 0x08 + freq* 0x1c + 32*4*k )

	{DDR_CFG_BASE_0 + DENALI_PHY_20        + 32*4*1, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 0x1c + 32*4*1, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 0x1c + 32*4*1, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_02 + 0x1c + 32*4*1, 0},

	{DDR_CFG_BASE_0 + DENALI_PHY_20        + 32*4*2, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 0x1c + 32*4*2, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 0x1c + 32*4*2, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_02 + 0x1c + 32*4*2, 0},

	{DDR_CFG_BASE_0 + DENALI_PHY_20        + 32*4*3, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 0x1c + 32*4*3, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 0x1c + 32*4*3, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_02 + 0x1c + 32*4*3, 0},

	{DDR_CFG_BASE_0 + DENALI_PHY_20        + 32*4*4, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 0x1c + 32*4*4, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 0x1c + 32*4*4, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_02 + 0x1c + 32*4*4, 0},

	{DDR_CFG_BASE_0 + DENALI_CTL_29, 0},

	/* ZQ Select. */
	{DDR_CFG_BASE_0 + DENALI_PHY_177, DENALI_PHY_177_DATA},	/* 0x2C4 */
};

/* Include gate_training. */
struct memctl_data memctl1_dqs_gate_training[] = {
	// freq 0
	{DDR_CFG_BASE_1 + DENALI_PHY_19, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_1 + DENALI_PHY_04, 0}, //( DDR_PHY_BASE + 0x10 + freq* 0x1c + i*32*4)
	{DDR_CFG_BASE_1 + DENALI_PHY_05, 0}, //( DDR_PHY_BASE + 0x14 + freq* 0x1c + 32*4*k )
	{DDR_CFG_BASE_1 + DENALI_PHY_02, 0}, //( DDR_PHY_BASE + 0x08 + freq* 0x1c + 32*4*k )

	{DDR_CFG_BASE_1 + DENALI_PHY_19 + 32*4*1, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 32*4*1, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 32*4*1, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_02 + 32*4*1, 0},

	{DDR_CFG_BASE_1 + DENALI_PHY_19 + 32*4*2, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 32*4*2, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 32*4*2, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_02 + 32*4*2, 0},

	{DDR_CFG_BASE_1 + DENALI_PHY_19 + 32*4*3, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 32*4*3, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 32*4*3, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_02 + 32*4*3, 0},

	{DDR_CFG_BASE_1 + DENALI_PHY_19 + 32*4*4, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 32*4*4, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 32*4*4, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_02 + 32*4*4, 0},

	// freq 1
	{DDR_CFG_BASE_1 + DENALI_PHY_20, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 0x1c, 0}, //( DDR_PHY_BASE + 0x10 + freq* 0x1c + i*32*4)
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 0x1c, 0}, //( DDR_PHY_BASE + 0x14 + freq* 0x1c + 32*4*k )
	{DDR_CFG_BASE_1 + DENALI_PHY_02 + 0x1c, 0}, //( DDR_PHY_BASE + 0x08 + freq* 0x1c + 32*4*k )

	{DDR_CFG_BASE_1 + DENALI_PHY_20        + 32*4*1, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 0x1c + 32*4*1, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 0x1c + 32*4*1, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_02 + 0x1c + 32*4*1, 0},

	{DDR_CFG_BASE_1 + DENALI_PHY_20        + 32*4*2, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 0x1c + 32*4*2, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 0x1c + 32*4*2, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_02 + 0x1c + 32*4*2, 0},

	{DDR_CFG_BASE_1 + DENALI_PHY_20        + 32*4*3, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 0x1c + 32*4*3, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 0x1c + 32*4*3, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_02 + 0x1c + 32*4*3, 0},

	{DDR_CFG_BASE_1 + DENALI_PHY_20        + 32*4*4, 0}, ///* updata gate_training. */
	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 0x1c + 32*4*4, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 0x1c + 32*4*4, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_02 + 0x1c + 32*4*4, 0},

	{DDR_CFG_BASE_1 + DENALI_CTL_29, 0},

	/* ZQ Select. */
	{DDR_CFG_BASE_1 + DENALI_PHY_177, DENALI_PHY_177_DATA},	/* 0x2C4 */
};
const unsigned int memctl_dqs_length_gate_training = sizeof(memctl0_dqs_gate_training);
#endif

struct memctl_data memctl0_dqs_no_gate_training[] = {
	{DDR_CFG_BASE_0 + DENALI_PHY_04, 0}, //( DDR_PHY_BASE + 0x10 + freq* 0x1c + i*32*4)
	{DDR_CFG_BASE_0 + DENALI_PHY_05, 0}, //( DDR_PHY_BASE + 0x14 + freq* 0x1c + 32*4*k )

	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 32*4*1, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 32*4*1, 0},

	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 32*4*2, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 32*4*2, 0},

	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 32*4*3, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 32*4*3, 0},

	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 32*4*4, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 32*4*4, 0},

	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 0x1c, 0}, //( DDR_PHY_BASE + 0x10 + freq* 0x1c + i*32*4)
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 0x1c, 0}, //( DDR_PHY_BASE + 0x14 + freq* 0x1c + 32*4*k )

	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 0x1c + 32*4*1, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 0x1c + 32*4*1, 0},

	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 0x1c + 32*4*2, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 0x1c + 32*4*2, 0},

	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 0x1c + 32*4*3, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 0x1c + 32*4*3, 0},

	{DDR_CFG_BASE_0 + DENALI_PHY_04 + 0x1c + 32*4*4, 0},
	{DDR_CFG_BASE_0 + DENALI_PHY_05 + 0x1c + 32*4*4, 0},

	{DDR_CFG_BASE_0 + DENALI_CTL_29, 0},

	/* ZQ Select. */
	{DDR_CFG_BASE_0 + DENALI_PHY_177, DENALI_PHY_177_DATA},	/* 0x2C4 */
};

struct memctl_data memctl1_dqs_no_gate_training[] = {
	{DDR_CFG_BASE_1 + DENALI_PHY_04, 0}, //( DDR_PHY_BASE + 0x10 + freq* 0x1c + i*32*4)
	{DDR_CFG_BASE_1 + DENALI_PHY_05, 0}, //( DDR_PHY_BASE + 0x14 + freq* 0x1c + 32*4*k )

	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 32*4*1, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 32*4*1, 0},

	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 32*4*2, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 32*4*2, 0},

	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 32*4*3, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 32*4*3, 0},

	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 32*4*4, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 32*4*4, 0},

	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 0x1c, 0}, //( DDR_PHY_BASE + 0x10 + freq* 0x1c + i*32*4)
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 0x1c, 0}, //( DDR_PHY_BASE + 0x14 + freq* 0x1c + 32*4*k )

	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 0x1c + 32*4*1, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 0x1c + 32*4*1, 0},

	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 0x1c + 32*4*2, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 0x1c + 32*4*2, 0},

	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 0x1c + 32*4*3, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 0x1c + 32*4*3, 0},

	{DDR_CFG_BASE_1 + DENALI_PHY_04 + 0x1c + 32*4*4, 0},
	{DDR_CFG_BASE_1 + DENALI_PHY_05 + 0x1c + 32*4*4, 0},

	{DDR_CFG_BASE_1 + DENALI_CTL_29, 0},

	/* ZQ Select. */
	{DDR_CFG_BASE_1 + DENALI_PHY_177, DENALI_PHY_177_DATA},	/* 0x2C4 */

};

const unsigned int memctl_dqs_length_no_gate_training = sizeof(memctl0_dqs_no_gate_training);


struct memctl_data *memctl0_dqs = (struct memctl_data *)memctl0_dqs_no_gate_training;
struct memctl_data *memctl1_dqs = (struct memctl_data *)memctl1_dqs_no_gate_training;
unsigned int memctl_dqs_length = sizeof(memctl0_dqs_no_gate_training);

int memctl_refresh_dqsdatas(unsigned int id, unsigned int reg, unsigned int val, unsigned int mask)
{
	int i;
	int ret = -1;
	struct memctl_data * dqsdata = id ? memctl1_dqs : memctl0_dqs;

	for (i = (memctl_dqs_length-1); i >= 0; i--){
		if (reg == dqsdata[i].reg) {
			dqsdata[i].val &= ~mask;
			dqsdata[i].val |= val;
			ret = 0;
		}
	}

	return ret;
}

#if 0
static void write_dqs_training(unsigned int ddr_id, unsigned int freq)
{
	unsigned int i, j, k, data1, data2, flag;
	unsigned int data_record[4][256];
	unsigned int clk_wr_dqs, clk_wr, clk_rd_dqs;
	unsigned int DDR_CTL_BASE, DDR_PHY_BASE;

	DDR_CTL_BASE = (DDR0_REG_BASE + 0x1000 * ddr_id);
	DDR_PHY_BASE = (DDR0_REG_BASE + 0x400 + 0x1000 * ddr_id);

	for(k = 0; k < 4; k++){
		for(j = 0; j <= 0xff; j++){
			clk_wr_dqs = (0 + j) % 0x100;
			clk_wr = (0x40 + j) % 0x100;
			clk_rd_dqs = 0x40;

			*((volatile unsigned  int*)(DDR_PHY_BASE + 0x14 + freq* 0x1c + 32*4*k)) &= 0xFF000000;
			*((volatile unsigned  int*)(DDR_PHY_BASE + 0x14 + freq* 0x1c + 32*4*k)) |= ((clk_wr_dqs<<16) | (clk_wr<<8) | (clk_rd_dqs<<0));
			//*((volatile unsigned  int*)(DDR_CTL_BASE + 0x74 + 32*4*k)) |= (1<<8);
			//*((volatile unsigned  int*)(DDR_CTL_BASE + 0x74 + 32*4*k)) |= (1<<8);
			*((volatile unsigned  int*)(DDR_CTL_BASE + 0x74)) |= (1<<8);

			flag = bist_test(ddr_id, k, j*0x00001000);
			data_record[k][j] = flag;
		}
	}

	for(k = 0; k < 4; k++){
		data1 = 0;
		data2 = 0;
		for(j=0; j<=(0xff-4); j++){
			if((data_record[k][j+0] == 0) 
				&& (data_record[k][j+1] == 0)
				&& (data_record[k][j+2] == 0)
				&& (data_record[k][j+3] == 0)
				&& (data_record[k][j+4] == 0)){
				data1 = j;
				for(i = 0; i <= (0xff - data1); i++){
					if(data_record[k][j] == 1){
						data2 = j-1;
						break;
					}
					j++;
				}
				break;
			}
		}
		write_dqs[k] = (unsigned int)((data2+data1)/2);
	}
}
#endif

static void write_dq_training(unsigned int ddr_id, unsigned int freq)
{
	unsigned int j, k, flag;
	unsigned int data_record[4][256];
	unsigned int clk_wr_dqs, clk_wr, clk_rd_dqs;
	unsigned int DDR_CTL_BASE, DDR_PHY_BASE;

	DDR_CTL_BASE = (DDR0_REG_BASE + 0x1000 * ddr_id);
	DDR_PHY_BASE = (DDR0_REG_BASE + 0x400 + 0x1000 * ddr_id);

	for(k = 0; k < 4; k++){
#ifdef DDR_TRAINING_DEBUG_LOG
		printf("*******************DDR%d freq_%d slice%d write_dq_training************************\n",ddr_id, freq, k);
#endif
		for(j = 0; j <= 0xff; j++){
			clk_wr_dqs = write_dqs[k];
			clk_wr = j;
			clk_rd_dqs = 0x40;

			*((volatile unsigned  int*)(DDR_PHY_BASE + 0x14 + freq* 0x1c + 32*4*k)) &= 0xFF000000;
			*((volatile unsigned  int*)(DDR_PHY_BASE + 0x14 + freq* 0x1c + 32*4*k)) |= ((clk_wr_dqs<<16) | (clk_wr<<8) | (clk_rd_dqs<<0));
			//*((volatile unsigned  int*)(DDR_CTL_BASE + 0x74 + 32*4*k)) |= (1<<8);
			//*((volatile unsigned  int*)(DDR_CTL_BASE + 0x74 + 32*4*k)) |= (1<<8);
			*((volatile unsigned  int*)(DDR_CTL_BASE + 0x74)) |= (1<<8);

			flag = bist_test(ddr_id, k, j*0x00001000);
			data_record[k][j] = flag;
#ifdef DDR_TRAINING_DEBUG_LOG
			if(flag)
				printf("error!!!    clk_wr_dq=0x%08x\n", clk_wr);
			else
				printf("right!!!    clk_wr_dq=0x%08x\n", clk_wr);
#endif
		}
	}

	training_record_filter(data_record, write_dq);

	for(k=0; k<4; k++)
	{
		if((write_dq[k] < 0x30) || (write_dq[k] > 0x60)) {
			write_dq[k] = write_dq_standard_value[k];
#ifdef DDR_TRAINING_DEBUG_LOG
			printf("lte2007 write back: clk_wr_dq = 0x%08x\n",write_dq[k]);
#endif
		}
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*k )) &= 0xFF000000;
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*k )) |= ((write_dqs[k]<<16) | (write_dq[k]<<8) | (read_dqs[k]<<0));
		* ((volatile unsigned  int*)( DDR_CTL_BASE + 0x74 )) |= (1<<8);
	}
}

static	void read_dqs_training(unsigned int ddr_id, unsigned int freq)
{
	unsigned int j, k, flag;
	unsigned int data_record[4][256];
	unsigned int clk_wr_dqs, clk_wr, clk_rd_dqs;
	unsigned int DDR_CTL_BASE, DDR_PHY_BASE;

	DDR_CTL_BASE = (DDR0_REG_BASE + 0x1000 * ddr_id);
	DDR_PHY_BASE = (DDR0_REG_BASE + 0x400 + 0x1000 * ddr_id);

	for(k = 0; k < 4; k++){
#ifdef DDR_TRAINING_DEBUG_LOG
		printf("*******************DDR%d freq_%d slice%d read_dqs_training************************\n",ddr_id, freq, k);
#endif
		for(j = 0; j <= 0xff; j++){
			clk_wr_dqs = write_dqs[k];
			clk_wr = write_dq[k];
			clk_rd_dqs = j;
			*((volatile unsigned  int*)(DDR_PHY_BASE + 0x14 + freq* 0x1c + 32*4*k)) &= 0xFF000000;
			*((volatile unsigned  int*)(DDR_PHY_BASE + 0x14 + freq* 0x1c + 32*4*k)) |= ((clk_wr_dqs<<16) | (clk_wr<<8) | (clk_rd_dqs<<0));
			//*((volatile unsigned  int*)(DDR_CTL_BASE + 0x74 + 32*4*k)) |= (1<<8);
			//*((volatile unsigned  int*)(DDR_CTL_BASE + 0x74 + 32*4*k)) |= (1<<8);
			*((volatile unsigned  int*)(DDR_CTL_BASE + 0x74)) |= (1<<8);
			flag = bist_test(ddr_id, k, j*0x00001000);
			data_record[k][j] = flag;
#ifdef DDR_TRAINING_DEBUG_LOG
			if(flag)
				printf("error!!!    clk_rd_dqs=0x%08x\n", clk_rd_dqs);
			else
				printf("right!!!    clk_rd_dqs=0x%08x\n", clk_rd_dqs);
#endif
		}
	}

	training_record_filter(data_record, read_dqs);


	for(k=0; k<4; k++)
	{
		if((read_dqs[k] < 0x30 ) || (read_dqs[k] > 0x60)) {
			read_dqs[k] = read_dqs_standard_value[k];
#ifdef DDR_TRAINING_DEBUG_LOG
			printf("lte2007 write back: clk_rd_dqs = 0x%08x\n",read_dqs[k]);
#endif
		}
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*k )) &= 0xFF000000;
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*k )) |= ((write_dqs[k]<<16) | (write_dq[k]<<8) | (read_dqs[k]<<0));
		* ((volatile unsigned  int*)( DDR_CTL_BASE + 0x74 )) |= (1<<8);
	}
}

#if 1

#if defined(CONFIG_GATE_TRAINING_SUPPORT)
static	void write_back_gate_training(unsigned int ddr_id, int freq)
{
	unsigned int k;
	struct memctl_data * memctl_dqs = ddr_id ? memctl1_dqs: memctl0_dqs;
#if 0
	unsigned int training_result;
	unsigned int phy_2;
#endif

#if defined(DDR_TRAINING_WB_LOG)
	unsigned int DDR_CTL_BASE, DDR_PHY_BASE;
	DDR_CTL_BASE = (DDR0_REG_BASE         + 0x1000 * ddr_id);
	DDR_PHY_BASE = (DDR0_REG_BASE + 0x400 + 0x1000 * ddr_id);
#endif

#if 0
	unsigned int *gate_coarse_delay;
	unsigned int *gate_fine_delay;

	if (ddr_id) {
		gate_coarse_delay = freq ? f1_ddr1_gate_training_coarse_delay : f0_ddr1_gate_training_coarse_delay;
		gate_fine_delay = freq ? f1_ddr1_gate_training_fine_delay : f0_ddr1_gate_training_fine_delay;
	} else {
		gate_coarse_delay = freq ? f1_ddr0_gate_training_coarse_delay : f0_ddr0_gate_training_coarse_delay;
		gate_fine_delay = freq ? f1_ddr0_gate_training_fine_delay : f0_ddr0_gate_training_fine_delay;
	}
#endif
	for(k = 0; k < 5; k++){

		// set phy_19[12]=1
		memctl_dqs[k*CNT_G + CNT_G*5*freq].val = (1 << 12)
			| (readl(memctl_dqs[k*CNT_G + CNT_G*5*freq].reg));

		memctl_dqs[k*CNT_G+1   + CNT_G*5*freq].val = readl(memctl_dqs[k*CNT_G+1   + CNT_G*5*freq].reg);

		//fine delay
		// phy_21[15:8]->phy_5[31:24]
		memctl_dqs[k*CNT_G+2 + CNT_G*5*freq].val = readl(memctl_dqs[k*CNT_G+2 + CNT_G*5*freq].reg);


		//coarse delay
		// phy_21[7:5]->phy_2[2:0],  phy_21[4]->phy_2[29]
		memctl_dqs[k*CNT_G+3 + CNT_G*5*freq].val = readl(memctl_dqs[k*CNT_G+3 + CNT_G*5*freq].reg);

/*

		// set phy_19[12]=1
		memctl_dqs[k*CNT_G + CNT_G*5*freq].val = (1 << 12)
			| (*((volatile unsigned  int*)(memctl_dqs[k*CNT_G + CNT_G*5*freq].reg)));

		memctl_dqs[k*CNT_G+1   + CNT_G*5*freq].val = *((volatile unsigned  int*)( DDR_PHY_BASE + 0x10 + freq * 0x1c + 32*4*k));

		//fine delay
		// phy_21[15:8]->phy_5[31:24]
		memctl_dqs[k*CNT_G+2 + CNT_G*5*freq].val = (gate_fine_delay[k]<<24)
			| ((write_dqs[k]<<16) | (write_dq[k]<<8) | (read_dqs[k]<<0));


		//coarse delay
		training_result = (((gate_coarse_delay[k] & 0x1) << 29) | (gate_coarse_delay[k] >> 1));
		// phy_21[7:5]->phy_2[2:0],  phy_21[4]->phy_2[29]
		phy_2 = *((volatile unsigned  int*)(DDR_PHY_BASE + 0x08 + freq * 0x1c + 32*4*k));
		memctl_dqs[k*CNT_G+3 + CNT_G*5*freq].val = training_result
			| (phy_2 & (~((1<<29) | 0x07)));
*/

#ifdef	DDR_TRAINING_WB_LOG
		printf("function_%d DDR%d write back: address=0x%08x, value =0x%08x \n",
			freq, ddr_id, ( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*k ),
			* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*k )));
#endif
	}
	// *((volatile unsigned  int*)(DDR_CTL_BASE + 0x74)) |= (1<<8);
	memctl_dqs[k*CNT_G + CNT_G*5].val = readl(memctl_dqs[k*CNT_G + CNT_G*5].reg) | (1<<8);
	//memctl_dqs[k*CNT_G + CNT_G*5].val = *((volatile unsigned  int*)( DDR_CTL_BASE + 0x74 )) | (1<<8);


#ifdef DDR_TRAINING_WB_LOG
	{
		printf("id:%d, frq:%d\n",ddr_id, freq);
		for(k=0; k<(memctl_dqs_length/sizeof(struct memctl_data)); k++)
		{
			printf("[%02d][%s-%d]0x%08x=0x%08x\n",
				k,
				(memctl_dqs[k].reg>DDR_PHY_BASE) ? "phy" : "ctl",
				(memctl_dqs[k].reg>DDR_PHY_BASE) ? ((memctl_dqs[k].reg-DDR_PHY_BASE)/4):((memctl_dqs[k].reg-DDR_CTL_BASE)/4),
				memctl_dqs[k].reg,
				memctl_dqs[k].val);
		}
	}
#endif

}
#endif

static	void write_back_no_gate_training(unsigned int ddr_id, int freq)
{
	unsigned int k;
	unsigned int DDR_CTL_BASE, DDR_PHY_BASE;
	struct memctl_data * memctl_dqs = ddr_id ? memctl1_dqs: memctl0_dqs;

	DDR_CTL_BASE = (DDR0_REG_BASE + 0x1000 * ddr_id);
	DDR_PHY_BASE = (DDR0_REG_BASE + 0x400 + 0x1000 * ddr_id);

	for(k = 0; k < 5; k++){
		*((volatile unsigned  int*)(DDR_PHY_BASE + 0x14 + freq * 0x1c + 32*4*k)) &= 0xFF000000;
		*((volatile unsigned  int*)(DDR_PHY_BASE + 0x14 + freq * 0x1c + 32*4*k)) |= ((write_dqs[k]<<16) | (write_dq[k]<<8) | (read_dqs[k]<<0));

		memctl_dqs[k*2   +10*freq].val = *((volatile unsigned  int*)( DDR_PHY_BASE + 0x10 + freq * 0x1c + 32*4*k));
		memctl_dqs[k*2+1 +10*freq].val = *((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq * 0x1c + 32*4*k))
			| ((write_dqs[k]<<16) | (write_dq[k]<<8) | (read_dqs[k]<<0));

#ifdef	DDR_TRAINING_WB_LOG
		printf("function_%d DDR%d write back: address=0x%08x, value =0x%08x \n",
			freq, ddr_id, ( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*k ),
			* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*k )));
#endif
	}
	*((volatile unsigned  int*)(DDR_CTL_BASE + 0x74)) |= (1<<8);
	memctl_dqs[k*2+10].val = *((volatile unsigned  int*)( DDR_CTL_BASE + 0x74 )) | (1<<8);

#if 0
	{
		printf("id:%d, frq:%d\n",ddr_id, freq);
		for(k=0; k<(memctl_dqs_length/sizeof(struct memctl_data)); k++)
		{
			printf("[%02d]0x%08x=0x%08x\n", k,
				memctl_dqs[k].reg, memctl_dqs[k].val);
		}
	}
#endif

}
#else
#error need update gate_training
static	void write_back(unsigned int ddr_id, unsigned int freq)
{
	unsigned int i, k;
	unsigned int write_dqs_wb[5], write_dq_wb[5], read_dqs_wb[5];
	unsigned int dll_lock_mode[5], dll_lock_value[5];
	struct memctl_data * memctl_dqs = ddr_id ? memctl1_dqs: memctl0_dqs;
	unsigned int DDR_CTL_BASE, DDR_PHY_BASE;

	DDR_CTL_BASE = (0xE1001000 + 0x1000 * ddr_id);
	DDR_PHY_BASE = (0xE1001400 + 0x1000 * ddr_id);


	for(i=0;i<5;i++)
	{
		dll_lock_value[i] = *((volatile unsigned  int*)( DDR_PHY_BASE + 0x3C + i*32*4));
		dll_lock_mode[i] = (dll_lock_value[i] & 0x6);
		dll_lock_value[i] = ((dll_lock_value[i]>>8) & 0xff);

		if(dll_lock_mode[i] == (0x0<<1))		//full clk mode
		{
			write_dqs_wb[i] = dll_lock_value[i]*write_dqs[i]/256 ;
			write_dq_wb[i] = dll_lock_value[i]*write_dq[i]/256;
			read_dqs_wb[i] = dll_lock_value[i]*read_dqs[i]/256;
		}

		if(dll_lock_mode[i] == (0x2<<1))		//half clk mode
		{
			write_dqs_wb[i] = dll_lock_value[i]*write_dqs[i]/128 ;
			write_dq_wb[i] = dll_lock_value[i]*write_dq[i]/128;
			read_dqs_wb[i] = dll_lock_value[i]*read_dqs[i]/128;
		}
	}


	for(k=0;k<5;k++)
	{
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x10 + freq * 0x1c + 32*4*k)) |= (1<<23);	//disable master dll
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq * 0x1c + 32*4*k )) &= 0xFF000000;
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq * 0x1c + 32*4*k )) |= ((write_dqs_wb[k]<<16) | (write_dq_wb[k]<<8) | (read_dqs_wb[k]<<0));

		memctl_dqs[k*2   +10*freq].val = *((volatile unsigned  int*)( DDR_PHY_BASE + 0x10 + freq * 0x1c + 32*4*k)) | (1<<23);
		memctl_dqs[k*2+1 +10*freq].val = *((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq * 0x1c + 32*4*k))
			| ((write_dqs_wb[k]<<16) | (write_dq_wb[k]<<8) | (read_dqs_wb[k]<<0));

	}
	* ((volatile unsigned  int*)( DDR_CTL_BASE + 0x74 )) |= (1<<8);

	memctl_dqs[k*2+10].val = *((volatile unsigned  int*)( DDR_CTL_BASE + 0x74 )) | (1<<8);
}
#endif

static unsigned int bist_test(int ddr_id, int slice, unsigned int offset)
{
	unsigned int data;
	unsigned int DDR_CTL_BASE;
	int timeout = 10000000;

	DDR_CTL_BASE = (DDR0_REG_BASE + 0x1000 * ddr_id);

	*((volatile unsigned  int*)(DDR_CTL_BASE + 0x114)) &= ~(0x3f<<24);
	*((volatile unsigned  int*)(DDR_CTL_BASE + 0x114)) |= (0xc<<24);		//addr_space	4kB
	*((volatile unsigned  int*)(DDR_CTL_BASE + 0x118)) = 0x001;	//data_bist
	*((volatile unsigned  int*)(DDR_CTL_BASE + 0x11C)) = (0x100000*slice + offset);	//bist_start_addr
	*((volatile unsigned  int*)(DDR_CTL_BASE + 0x124)) = ~(0xff<<(slice*8));	//data_mask
	*((volatile unsigned  int*)(DDR_CTL_BASE + 0x128)) = ~(0xff<<(slice*8));	//data_mask
	*((volatile unsigned  int*)(DDR_CTL_BASE + 0x114)) |= (0x1<<8);				//bist_go
	//wait for ddr init complete inter
	while(--timeout){
		if((*((volatile unsigned  int*)( DDR_CTL_BASE + 0x16C ))>>6)&0x1){
			*((volatile unsigned  int*)( DDR_CTL_BASE + 0x170 )) |= (0x1<<6);
			break;
		}
	}

	if (!timeout)
		printf("bist_test id:%d,slice:%d, offset:%d timeout!\n",
			ddr_id, slice, offset);

	//check bist status
	data = *((volatile unsigned  int*)(DDR_CTL_BASE + 0x114));
	*((volatile unsigned  int*)(DDR_CTL_BASE + 0x114)) &= ~(0x1<<8);

	if((data >>16)&0x1)
		return(0); // ok
	else
		return(1);  // fail
}
#if 1

#if 0
static unsigned int gate_hardware_training(int ddr_id, int freq )
{
	int i;
	unsigned int data, gate_coarse_delay[5], gate_fine_delay[5];

	unsigned int DDR_CTL_BASE, DDR_PHY_BASE;

	DDR_CTL_BASE = (0xE1001000 + 0x1000 * ddr_id);
	DDR_PHY_BASE = (0xE1001400 + 0x1000 * ddr_id);

#if 0
	for(i=0;i<5;i++)
	{
		j = 5;
		data = (((j&0x1)<<29) | (j >> 1));
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x8 + freq*0x1c + 32*4*i )) &= ~((1<<29)|(0x7));
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x8 + freq*0x1c + 32*4*i )) |= data;
		* ((volatile unsigned  int*)( DDR_CTL_BASE + 0x74 )) |= (1<<8);
		printf("address=0x%08x, value =0x%08x \n", ( DDR_PHY_BASE + 0x8 + freq*0x1c + 32*4*i ), * ((volatile unsigned  int*)( DDR_PHY_BASE + 0x8 + freq*0x1c + 32*4*i )));
	}
#endif


	printf("Start a read leveling gate training operation. \n" );
	*((volatile unsigned  int*)( DDR_CTL_BASE + 0x278 )) |= (1<<16);
	*((volatile unsigned  int*)( DDR_CTL_BASE + 0x1F4 )) |= (1<<0);

	*((volatile unsigned  int*)( DDR_CTL_BASE + 0x1EC )) |= (1<<24);


	while((*((volatile unsigned  int*)( DDR_CTL_BASE + 0x16C )) & (1<<14)) == 0);
	printf("A read leveling gate training operation has been requested. \n" );

	while((*((volatile unsigned  int*)( DDR_CTL_BASE + 0x16C )) & (1<<16)) == 0);
	printf("The leveling operation has completed. \n" );

	if((*((volatile unsigned  int*)( DDR_CTL_BASE + 0x16C )) & (1<<8)) == 1)
	{
		printf("A read leveling gate training error has occurred. \n" );
	}


	*((volatile unsigned  int*)( DDR_CTL_BASE + 0x278 )) &= ~(1<<16);
	*((volatile unsigned  int*)( DDR_CTL_BASE + 0x170 )) = (1<<16)|(1<<14)|(1<<8);		//clear intr


	//save gate_training result
	for(i=0;i<5;i++)
	{
		data = *((volatile unsigned  int*)( DDR_PHY_BASE + 0x54 + i*32*4));
		printf("gate_training_result: address=0x%08x, value =0x%08x \n", ( DDR_PHY_BASE + 0x54 + i*32*4 ), *((volatile unsigned  int*)( DDR_PHY_BASE + 0x54 + i*32*4)));
		gate_coarse_delay[i] = ((data >> 4) & 0xf);
		gate_fine_delay[i] = ((data >> 8) & 0xff);

		if(freq == 0)
		{
			if(ddr_id == 0)
			{
				f0_ddr0_gate_training_coarse_delay[i] = gate_coarse_delay[i];
				f0_ddr0_gate_training_fine_delay[i] = gate_fine_delay[i];
			}
			if(ddr_id == 1)
			{
				f0_ddr1_gate_training_coarse_delay[i] = gate_coarse_delay[i];
				f0_ddr1_gate_training_fine_delay[i] = gate_fine_delay[i];
			}
		}


		if(freq == 1)
		{
			if(ddr_id == 0)
			{
				f1_ddr0_gate_training_coarse_delay[i] = gate_coarse_delay[i];
				f1_ddr0_gate_training_fine_delay[i] = gate_fine_delay[i];
			}
			if(ddr_id == 1)
			{
				f1_ddr1_gate_training_coarse_delay[i] = gate_coarse_delay[i];
				f1_ddr1_gate_training_fine_delay[i] = gate_fine_delay[i];
			}
		}

	}

	return(0);
}
#endif
static unsigned int	gate_software_training(int ddr_id, int freq)
{
	int i, j, k;
	unsigned int data, data1, data2, flag, initial_coarse_delay, initial_fine_delay;
	unsigned int max_len, len;
	unsigned int gate_coarse_delay[5], gate_fine_delay[5], data_record[5][96];

	unsigned int DDR_CTL_BASE, DDR_PHY_BASE;

	DDR_CTL_BASE = (0xE1001000 + 0x1000 * ddr_id);
	DDR_PHY_BASE = (0xE1001400 + 0x1000 * ddr_id);

	for(i=0;i<4;i++)
	{
#ifdef DDR_TRAINING_DEBUG_LOG
		printf("*******************function_%d DDR%d slice%d gate_software_training************************\n",freq, ddr_id,i);
#endif
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x4c + freq*0x04 + 32*4*i )) |= (1<<12);	//set lvl_sw_en=1

		for(j=0;j<6;j++)		//coarse delay
		{
			initial_coarse_delay = (((j&0x1)<<29) | (j >> 1));
			* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x8 + freq*0x1c + 32*4*i )) &= ~((1<<29)|(0x7));
			* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x8 + freq*0x1c + 32*4*i )) |= initial_coarse_delay;

			for(k=0; k<16; k++)		//fine delay
			{
				initial_fine_delay = 0x10*k;
				* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*i )) &= 0x00FFFFFF;
				* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*i )) |= (initial_fine_delay<<24);
				* ((volatile unsigned  int*)( DDR_CTL_BASE + 0x74 )) |= (1<<8);
				* ((volatile unsigned  int*)( DDR_CTL_BASE + 0x74 )) |= (1<<8);


				flag = bist_test(ddr_id, i, k*0x00001000);
				data_record[i][j*16 + k] = flag;
#ifdef DDR_TRAINING_DEBUG_LOG
				if(flag)	printf("error!!!    initial_coarse_delay=0x%08x, initial_fine_delay=0x%08x\n", j, initial_fine_delay);
				else		printf("	right!!!    initial_coarse_delay=0x%08x, initial_fine_delay=0x%08x\n", j, initial_fine_delay);
#endif
			}
		}
	}

	//find the last working value
	for(k = 0; k < 4; k++)
	{
		max_len = 0;
		flag = 0;
		len = 0;

		data1 = 0;
		data2 = 0;

		for(j = 0; j < (96 - 4); j++)
		{
			if((data_record[k][j+0] == 0) && (data_record[k][j+1] == 0)	&& (data_record[k][j+2] == 0)	&& (data_record[k][j+3] == 0)	&& (data_record[k][j+4] == 0))
			{
				data1 = j;
				for(i = 0; i < (96 - data1); i++)
				{
					j++;
					if(j > 95)
					{
						data2 = j-1;
						gate_coarse_delay[k] = (data2/16);
						gate_fine_delay[k] = (data2%16) * 0x10;
						break;
					}

					if(data_record[k][j] == 1)
					{
						data2 = j-1;

						if(flag == 0)
						{
							max_len = data2 - data1 + 1;
							gate_coarse_delay[k] = (data2/16);
							gate_fine_delay[k] = (data2%16) * 0x10;
							flag = 1;
						}
						else
						{
							len = data2 - data1 + 1;
							if(len > max_len)
							{
								max_len = len;
								gate_coarse_delay[k] = (data2/16);
								gate_fine_delay[k] = (data2%16) * 0x10;
							}
						}
						break;
					}
				}
			}
		}
		if((data1 == 0) && (data2 == 0))
		{
			printf("function_%d DDR%d slice%d gate_software_training error!\n",freq, ddr_id,k);
		}
#ifdef DDR_TRAINING_DEBUG_LOG
		printf("the last working value: gate_coarse_delay[%d] = 0x%08x, gate_fine_delay[%d] = 0x%08x\n",k, gate_coarse_delay[k], k, gate_fine_delay[k]);
#endif
	}


	//Find the last working value and then reduce by 3/4clk
	for(k=0;k<4;k++)
	{
		if(gate_fine_delay[k] >= 0x40)
		{
			gate_coarse_delay[k] = gate_coarse_delay[k]-1;
			gate_fine_delay[k] = gate_fine_delay[k]-0x40;
		}
		else
		{
			gate_coarse_delay[k] = gate_coarse_delay[k]-2;
			gate_fine_delay[k] = gate_fine_delay[k]+0x40;
		}
	}


	//write back
	for(k=0;k<4;k++)
	{
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x4c + freq*0x04 + 32*4*k )) |= (1<<12);	//set lvl_sw_en=1

		//coarse delay
		data = (((gate_coarse_delay[k] & 0x1) << 29) | (gate_coarse_delay[k] >> 1));
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x8 + freq*0x1c + 32*4*k )) &= ~((1<<29)|(0x7));
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x8 + freq*0x1c + 32*4*k )) |= data;
#ifdef	DDR_TRAINING_WB_LOG
		printf("function_%d DDR%d write back: address=0x%08x, value =0x%08x \n", freq, ddr_id, ( DDR_PHY_BASE + 0x8 + freq*0x1c + 32*4*k ), * ((volatile unsigned  int*)( DDR_PHY_BASE + 0x8 + freq*0x1c + 32*4*k )));
#endif

		//fine delay
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*k )) &= 0x00FFFFFF;
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*k )) |= (gate_fine_delay[k]<<24);
		* ((volatile unsigned  int*)( DDR_CTL_BASE + 0x74 )) |= (1<<8);
		* ((volatile unsigned  int*)( DDR_CTL_BASE + 0x74 )) |= (1<<8);
#ifdef	DDR_TRAINING_WB_LOG
		printf("function_%d DDR%d write back: address=0x%08x, value =0x%08x \n", freq, ddr_id, ( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*k ), * ((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*k )));
#endif
	}




	//save gate_training result
	for(k=0;k<4;k++)
	{
		if(freq == 0)
		{
			if(ddr_id == 0)
			{
				f0_ddr0_gate_training_coarse_delay[k] = gate_coarse_delay[k];
				f0_ddr0_gate_training_fine_delay[k] = gate_fine_delay[k];
			}
			if(ddr_id == 1)
			{
				f0_ddr1_gate_training_coarse_delay[k] = gate_coarse_delay[k];
				f0_ddr1_gate_training_fine_delay[k] = gate_fine_delay[k];
			}
		}


		if(freq == 1)
		{
			if(ddr_id == 0)
			{
				f1_ddr0_gate_training_coarse_delay[k] = gate_coarse_delay[k];
				f1_ddr0_gate_training_fine_delay[k] = gate_fine_delay[k];
			}
			if(ddr_id == 1)
			{
				f1_ddr1_gate_training_coarse_delay[k] = gate_coarse_delay[k];
				f1_ddr1_gate_training_fine_delay[k] = gate_fine_delay[k];
			}
		}

	}
	return(0);
}
int	gate_training_write_back(int freq, int ddr_id)
{
	int k;
	unsigned int data, gate_coarse_delay[5], gate_fine_delay[5];

	unsigned int DDR_CTL_BASE, DDR_PHY_BASE;

	DDR_CTL_BASE = (0xE1001000 + 0x1000 * ddr_id);
	DDR_PHY_BASE = (0xE1001400 + 0x1000 * ddr_id);



	for(k=0; k<4; k++)
	{
		if(freq == 0)
		{
			if(ddr_id == 0)
			{
				gate_coarse_delay[k] = f0_ddr0_gate_training_coarse_delay[k];
				gate_fine_delay[k] = f0_ddr0_gate_training_fine_delay[k];
			}
			if(ddr_id == 1)
			{
				gate_coarse_delay[k] = f0_ddr1_gate_training_coarse_delay[k];
				gate_fine_delay[k] = f0_ddr1_gate_training_fine_delay[k];
			}
		}

		if(freq == 1)
		{
			if(ddr_id == 0)
			{
				gate_coarse_delay[k] = f1_ddr0_gate_training_coarse_delay[k];
				gate_fine_delay[k] = f1_ddr0_gate_training_fine_delay[k];
			}
			if(ddr_id == 1)
			{
				gate_coarse_delay[k] = f1_ddr1_gate_training_coarse_delay[k];
				gate_fine_delay[k] = f1_ddr1_gate_training_fine_delay[k];
			}
		}
	}


	for(k=0;k<4;k++)
	{
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x4c + freq*0x04 + 32*4*k )) |= (1<<12);	//set lvl_sw_en=1

		//coarse delay
		data = (((gate_coarse_delay[k] & 0x1) << 29) | (gate_coarse_delay[k] >> 1));
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x8 + freq*0x1c + 32*4*k )) &= ~((1<<29)|(0x7));
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x8 + freq*0x1c + 32*4*k )) |= data;
#ifdef	DDR_TRAINING_WB_LOG
		printf("function_%d DDR%d write back: address=0x%08x, value =0x%08x \n", freq, ddr_id, ( DDR_PHY_BASE + 0x8 + freq*0x1c + 32*4*k ), * ((volatile unsigned  int*)( DDR_PHY_BASE + 0x8 + freq*0x1c + 32*4*k )));
#endif

		//fine delay
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*k )) &= 0x00FFFFFF;
		* ((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*k )) |= (gate_fine_delay[k]<<24);
		* ((volatile unsigned  int*)( DDR_CTL_BASE + 0x74 )) |= (1<<8);
		* ((volatile unsigned  int*)( DDR_CTL_BASE + 0x74 )) |= (1<<8);
#ifdef	DDR_TRAINING_WB_LOG
		printf("function_%d DDR%d write back: address=0x%08x, value =0x%08x \n", freq, ddr_id, ( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*k ), * ((volatile unsigned  int*)( DDR_PHY_BASE + 0x14 + freq*0x1c + 32*4*k )));
#endif
	}
	return(0);
}
#endif

#if defined(CONFIG_GATE_TRAINING_SUPPORT)
int memctl_training_refresh_info(void)
{
	if (gd->need_gate_training) {
		memctl0_dqs = (struct memctl_data *)memctl0_dqs_gate_training;
		memctl1_dqs = (struct memctl_data *)memctl1_dqs_gate_training;
		memctl_dqs_length = memctl_dqs_length_gate_training;
	} else {
		memctl0_dqs = (struct memctl_data *)memctl0_dqs_no_gate_training;
		memctl1_dqs = (struct memctl_data *)memctl1_dqs_no_gate_training;
		memctl_dqs_length = memctl_dqs_length_no_gate_training;
	}

	return 0;
}

#endif

static int dqs_training(unsigned int ddr_id, unsigned int freq)
{
	//unsigned int k,data1;
	//write_dqs_training(ddr_id, freq);
	write_dq_training(ddr_id, freq);
#if defined(CONFIG_GATE_TRAINING_SUPPORT)
	if (gd->need_gate_training)
		gate_software_training(ddr_id, freq);
#endif
	read_dqs_training(ddr_id, freq);
#if defined(CONFIG_GATE_TRAINING_SUPPORT)
	if (gd->need_gate_training)
		write_back_gate_training(ddr_id, freq);
	else
#endif
		write_back_no_gate_training(ddr_id, freq);

#if DDR_TRAINING_WB_LOG
	printf("phy21:0x%x\n", readl(DDR_CFG_BASE_0 + DENALI_PHY_21));
	printf("phy53:0x%x\n", readl(DDR_CFG_BASE_0 + DENALI_PHY_53));
	printf("phy85:0x%x\n", readl(DDR_CFG_BASE_0 + DENALI_PHY_85));
	printf("phy117:0x%x\n", readl(DDR_CFG_BASE_0 + DENALI_PHY_117));
#endif
	return 0;
}

/* f0=high_fren, f1=low_fren */
int ddr_frequency_adjust(int select_low_fren)
{
	int timeout = 10000000;

	if (select_low_fren !=
		(readl(DDR_PWR_AP_DDR_FREQ)|readl(DDR_PWR_CP_DDR_FREQ))) {
		//printf("ddr freq is %s\n",
		//	(readl(DDR_PWR_AP_DDR_FREQ)|readl(DDR_PWR_CP_DDR_FREQ))?"high":"low");
		return 0;
	}


	/* Clear intr. */
	writel(((1 << 21) | (1 << 10)), DDR_PWR_INTR_APA7);

	/* Select hardware adjust. */
	writel(1, DDR_PWR_CMDFIFOCTL);

	if(select_low_fren)
	{
		/* Low freq. */
		writel(0, DDR_PWR_AP_DDR_FREQ);
		writel(0, DDR_PWR_CP_DDR_FREQ);
	}
	else
	{
		/* High freq. */
		writel(1, DDR_PWR_AP_DDR_FREQ);
		writel(1, DDR_PWR_CP_DDR_FREQ);
	}

	do {
		if (readl(DDR_PWR_INTR_RAW) & (1<<10)) {
			/* Clear interrupt. */
			writel((1<<10), DDR_PWR_INTR_APA7);
			break;
		}
	} while (--timeout);

	//printf("ddr freq changed to ddr0 %s, ddr1 %s\n",
	//	readl(DDR_CFG_BASE_0 + DENALI_CTL_48)&(1<<16)?"low":"high",
	//	readl(DDR_CFG_BASE_1 + DENALI_CTL_48)&(1<<16)?"low":"high");

	if (timeout <= 0) {
		printf("ddr_frequency_adjust %s timeout!\n",
			select_low_fren?"low":"high");
		return -1;
	}

	 return(0);
}

int ddr_training(unsigned int use0, unsigned int use1)
{
	printf("training :%d %d\n", use0, use1);

	ddr_frequency_adjust(0);

	/* Must call before dqs_training. */
	memctl_refresh_dll_lock_value(0);

	if (use0)
		dqs_training(0, 0);
	if (use1)
		dqs_training(1, 0);

	ddr_frequency_adjust(1);

	/* Must call before dqs_training. */
	memctl_refresh_dll_lock_value(1);

	if (use0)
		dqs_training(0, 1);
	if (use1)
		dqs_training(1, 1);

	ddr_frequency_adjust(0);

	return 0;
}

int memctl_update_training_date(struct memctl_data *data, unsigned int len)
{
	unsigned int i;
	/* Config timing and other registers. */
	for (i = 0; i < len; i++)
		writel(data[i].val, data[i].reg);

	return 0;
}

static	void training_record_filter(unsigned int training_record[4][256], unsigned int *training_result)
{
	unsigned int i, j, k, data1, data2;
	unsigned int max_len, len, flag;


	for(k = 0; k < 4; k++)
	{
		max_len = 0;
		flag = 0;
		len = 0;

		data1 = 0;
		data2 = 0;

		for(j = 0; j <= (0xff - 4); j++)
		{
			if((training_record[k][j+0] == 0) && (training_record[k][j+1] == 0)	&& (training_record[k][j+2] == 0)	&& (training_record[k][j+3] == 0)	&& (training_record[k][j+4] == 0))
			{
				data1 = j;
				for(i = 0; i <=(0xff - data1); i++)
				{
					j++;
					if(j > 0xff)
					{
						data2 = j-1;
						training_result[k] = (unsigned int)((data2+data1)/2);
						break;
					}

					if(training_record[k][j] == 1)
					{
						data2 = j-1;

						if(flag == 0)
						{
							max_len = data2 - data1 + 1;
							training_result[k] = (unsigned int)((data2+data1)/2);
							flag = 1;
						}
						else
						{
							len = data2 - data1 + 1;
							if(len > max_len)
							{
								max_len = len;
								training_result[k] = (unsigned int)((data2+data1)/2);
							}
						}
						break;
					}
				}
			}
		}
		if((data1 == 0) && (data2 == 0))
		{
			training_result[k] = 0x0;
		}
#ifdef DDR_TRAINING_DEBUG_LOG
		printf("write back: training_result[%d] = 0x%08x\n",k, training_result[k]);
#endif
	}
}

