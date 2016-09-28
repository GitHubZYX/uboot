/*
 * ddr_zq_select.c
 *
 *  Created on: 2014-5-19
 *	Author: qichaofang
 */

#include <common.h>
DECLARE_GLOBAL_DATA_PTR;

extern int memctl_refresh_dqsdatas(unsigned int id, unsigned int reg, unsigned int val, unsigned int mask);
static	void write_mr3_reg(unsigned int write_data, int ddr_id);
static unsigned int ddr_data_bist_test(int ddr_id, unsigned int offset);
static unsigned int ddr_address_bist_test(int ddr_id, unsigned int offset);

//#define DEBUG_LOG
//#define WRITE_BACK_LOG

void dq_dqs_dm_zq_select(int ddr_id)
{
	unsigned int i, j, k;
	unsigned int data, zq_wb, wb_value, error_flag;
	unsigned int flag[3][8];
	unsigned int zq_value[8] = {0, 1, 8, 9, 0xc, 0xd, 0xe, 0xf};
	unsigned int mr3_value[3] = {1, 2, 3};
	unsigned int data1[3] = {0, 0, 0};
	unsigned int data2[3] = {0, 0, 0};
	unsigned int DDR_PHY_BASE;

	DDR_PHY_BASE = (DDR0_REG_BASE + 0x400 + 0x1000 * ddr_id);

#ifdef WRITE_BACK_LOG
	printf("\n*********************DDR%d dq_dqs_dm_zq_select(PHY_170)************************\n", ddr_id);
#endif

	for(j=0; j<3; j++)
	{
		write_mr3_reg(mr3_value[j], ddr_id);

		for(i=0; i<8; i++)
		{
			data = (zq_value[i] << 1);
			zq_wb = (data<<25)|(data<<20)|(data<<15)|(data<<10)|(data<<5)|(data<<0);	//ODT disable
			*((volatile unsigned  int*)(DDR_PHY_BASE + 0x2A8)) = zq_wb;

			flag[j][i] = ddr_data_bist_test(ddr_id, 0x00100000*i);				//right = 0;
		}
	}

#ifdef DEBUG_LOG
	for(j=0; j<3; j++)
	{
		printf("\n");
		for(i=0; i<8; i++)
		{
			data = (zq_value[i] << 1);
			zq_wb = (data<<25)|(data<<20)|(data<<15)|(data<<10)|(data<<5)|(data<<0);	//ODT disable

			if(flag[j][i])
				printf("error!!!   MR3 = 0x%08x; PHY_170 = 0x%08x\n", mr3_value[j],zq_wb);
			else
				printf("right!!!   MR3 = 0x%08x; PHY_170 = 0x%08x\n", mr3_value[j],zq_wb);
		}
	}
#endif

	//select ddr_zq
	for(j=0; j<3; j++)
	{
		for(i=0; i<(8-1); i++)
		{
			if((flag[j][i] == 0) && (flag[j][i+1] == 0))
			{
				data1[j] = i;

				for(k=0; k<(8-data1[j]); k++)
				{
					if(flag[j][i] != 0)
					{
						data2[j] = i-1;
						break;
					}
					i++;

					if(i == 8)
					{
						data2[j] = i-1;
						break;
					}
				}
				break;
			}
		}
	}

	i = 0;
	k = 0;
	error_flag = 0;
	for(j=0; j<3; j++)
	{
		if((data1[j] == 0) && (data2[j] == 0))
		{
			error_flag += 1;
			i = j+1;
		}
		if((data1[j] != 0) || (data2[j] != 0))
		{
			k = j;
		}
	}
	if(error_flag == 3)
	{
		printf("ERROR!!!!  No correct parameter!  DDR%d dq_dqs_dm_zq_select(PHY_170)\n", ddr_id);
		return;
	}

	k += i;
	k = (unsigned int)(k/2);

	//write back
	if(k >= 2)
	{
		wb_value = mr3_value[k];
	}
	else
	{
		if((data1[k+1] != 0) || (data2[k+1] != 0))
		{
			wb_value = mr3_value[k+1];
		}
		else
		{
			wb_value = mr3_value[k];
		}
	}
	write_mr3_reg(wb_value, ddr_id);


	i = (unsigned int)((data2[k] + data1[k])/2);
	if(i<=0)
	{
		data = (zq_value[i] << 1);
	}
	else
	{
		if(flag[k][i-1] == 0)
			data = (zq_value[i-1] << 1);
		else
			data = (zq_value[i] << 1);
	}
	zq_wb = (data<<25)|(data<<20)|(data<<15)|(data<<10)|(data<<5)|(data<<0);		//ODT disable
	*((volatile unsigned  int*)(DDR_PHY_BASE + 0x2A8)) = zq_wb;

	if (memctl_refresh_dqsdatas(ddr_id, (DDR_PHY_BASE + 0x2A8), zq_wb, 0xffffffff))
		printf("memctl[%d] refresh PHY170 to 0x%x fail\n", ddr_id, zq_wb);

#ifdef WRITE_BACK_LOG
	printf("write_back: MR3 = 0x%08x; PHY_170 = 0x%08x\n", wb_value, zq_wb);
#endif
}


void instruction_clk_zq_select(int ddr_id)
{
	unsigned int i, k;
	unsigned int data, data1, data2, zq_wb;
	unsigned int flag[8];
	unsigned int zq_value[8] = {0, 1, 8, 9, 0xc, 0xd, 0xe, 0xf};
	unsigned int DDR_PHY_BASE;

	DDR_PHY_BASE = (DDR0_REG_BASE + 0x400 + 0x1000 * ddr_id);

#ifdef WRITE_BACK_LOG
	printf("\n*********************DDR%d instruction_clk_zq_select(PHY_172)************************\n", ddr_id);
#endif

	for(i=0; i<8; i++)
	{
		data = zq_value[i];
		zq_wb = (data<<20)|(data<<16)|(data<<4)|(data<<0);
		*((volatile unsigned  int*)(DDR_PHY_BASE + 0x2B0)) = zq_wb;

		flag[i] = ddr_address_bist_test(ddr_id, 0x00100000*i);	//right = 0;

#ifdef DEBUG_LOG
		if(flag[i])
			printf("error!!!   PHY_172 = 0x%08x\n", zq_wb);
		else
			printf("right!!!   PHY_172 = 0x%08x\n", zq_wb);
#endif
	}

	//select ddr_zq
	data1 = 0;
	data2 = 0;

	for(i=0; i<(8-1); i++)
	{
		if((flag[i] == 0) && (flag[i+1] == 0))
		{
			data1 = i;

			for(k=0; k<(8-data1); k++)
			{
				if(flag[i] != 0)
				{
					data2 = i-1;
					break;
				}
				i++;

				if(i >= 8)
				{
					data2 = i-1;
					break;
				}
			}
			break;
		}
	}

	if((data1==0) && (data2==0))
	{
		printf("ERROR!!!!  No correct parameter!  DDR%d instruction_clk_zq_select(PHY_172)\n", ddr_id);
		return;
	}

	i = (unsigned int)((data2 + data1)/2);
	if(i<=0)
	{
		data = zq_value[i];
	}
	else
	{
		if(flag[i-1] == 0)
			data = zq_value[i-1];
		else
			data = zq_value[i];
	}
	zq_wb = (data<<20)|(data<<16)|(data<<4)|(data<<0);
	*((volatile unsigned  int*)(DDR_PHY_BASE + 0x2B0)) = zq_wb;

	if (memctl_refresh_dqsdatas(ddr_id, (DDR_PHY_BASE + 0x2B0), zq_wb, 0xffffffff))
		printf("memctl[%d] refresh PHY172 to 0x%x fail\n", ddr_id, zq_wb);


#ifdef WRITE_BACK_LOG
	printf("write_back: PHY_172 = 0x%08x\n", zq_wb);
#endif
}



void feedback_zq_select(int ddr_id)
{
	unsigned int i, k;
	unsigned int data, data1, data2, zq_wb;
	unsigned int flag[8];
	unsigned int zq_value[8] = {0, 1, 8, 9, 0xc, 0xd, 0xe, 0xf};
	unsigned int DDR_PHY_BASE;

	DDR_PHY_BASE = (DDR0_REG_BASE + 0x400 + 0x1000 * ddr_id);

#ifdef WRITE_BACK_LOG
	printf("\n*********************DDR%d feedback_zq_select(PHY_173)************************\n", ddr_id);
#endif

	for(i=0; i<8; i++)
	{
		data = zq_value[i];
		zq_wb = (data<<20)|(data<<16)|(data<<4)|(data<<0);
		*((volatile unsigned  int*)(DDR_PHY_BASE + 0x2B4)) = zq_wb;

		flag[i] = ddr_data_bist_test(ddr_id, 0x00100000*i);	//right = 0;

#ifdef DEBUG_LOG
		if(flag[i])
			printf("error!!!   PHY_173 = 0x%08x\n", zq_wb);
		else
			printf("right!!!   PHY_173 = 0x%08x\n", zq_wb);
#endif
	}

	//select ddr_zq
	data1=0;
	data2=0;

	for(i=0; i<(8-1); i++)
	{
		if((flag[i] == 0) && (flag[i+1] == 0))
		{
			data1 = i;

			for(k=0; k<(8-data1); k++)
			{
				if(flag[i] != 0)
				{
					data2 = i-1;
					break;
				}
				i++;

				if(i >= 8)
				{
					data2 = i-1;
					break;
				}
			}
			break;
		}
	}

	if((data1==0) && (data2==0))
	{
		printf("ERROR!!!!  No correct parameter!  DDR%d feedback_zq_select(PHY_173)\n", ddr_id);
		return;
	}

	i = (unsigned int)((data2 + data1)/2);
	if(i<=0)
	{
		data = zq_value[i];
	}
	else
	{
		if(flag[i-1] == 0)
			data = zq_value[i-1];
		else
			data = zq_value[i];
	}
	zq_wb = (data<<20)|(data<<16)|(data<<4)|(data<<0);
	*((volatile unsigned  int*)(DDR_PHY_BASE + 0x2B4)) = zq_wb;

	if (memctl_refresh_dqsdatas(ddr_id, (DDR_PHY_BASE + 0x2B4), zq_wb, 0xffffffff))
		printf("memctl[%d] refresh PHY173 to 0x%x fail\n", ddr_id, zq_wb);

#ifdef WRITE_BACK_LOG
	printf("write_back: PHY_173 = 0x%08x\n", zq_wb);
#endif
}


static	void write_mr3_reg(unsigned int write_data, int ddr_id)
{
	unsigned int data, wb_value;
	unsigned int DDR_CTL_BASE;
	unsigned long timeout = 1000000;
	int retry = 100;

	DDR_CTL_BASE = (DDR0_REG_BASE + 0x1000 * ddr_id);

	*((volatile unsigned  int*)(DDR_CTL_BASE + 0x170)) = (1<<22);

	loop:
	//配置MR3
	data = *((volatile unsigned  int*)(DDR_CTL_BASE + 0xF4));
	data &= 0xffff0000;
	wb_value = (data | write_data<<0);
	*((volatile unsigned  int*)(DDR_CTL_BASE + 0xF4)) = wb_value;


	data = *((volatile unsigned  int*)(DDR_CTL_BASE + 0x108));
	data &= 0x0000ffff;
	wb_value = (data | write_data<<16);
	*((volatile unsigned  int*)(DDR_CTL_BASE + 0x108)) = wb_value;


	data = *((volatile unsigned  int*)(DDR_CTL_BASE + 0xD4));
	data &= 0xfc000000;
	wb_value = (0x7<<23)|0x3;
	*((volatile unsigned  int*)(DDR_CTL_BASE + 0xD4)) = wb_value;

	//check CTL_91的中断[22]为1，表示MRW指令完成
	data = *((volatile unsigned  int*)(DDR_CTL_BASE + 0x16c));
	while(!(data && (1<<22)) && --timeout)
	{
		data = *((volatile unsigned  int*)(DDR_CTL_BASE + 0x16c));
	}

	if (!timeout)
		printf("write_mr3_reg id:%d, check ctl91:0x%x timeout\n", ddr_id,
			*((volatile unsigned  int*)(DDR_CTL_BASE + 0x16c)));

	*((volatile unsigned  int*)(DDR_CTL_BASE + 0x170)) = (1<<22);


	//check CTL_54[7:0]为0，表示MRW指令没有错误
	data = *((volatile unsigned  int*)(DDR_CTL_BASE + 0xD8));
	if(data & 0xff)
	{
		printf("write MR_register error, goto loop!!!");
		if (-- retry)
			goto loop;
	}
}



static unsigned int ddr_data_bist_test(int ddr_id, unsigned int offset);
static unsigned int ddr_address_bist_test(int ddr_id, unsigned int offset);
static unsigned int ddr_data_bist_test(int ddr_id, unsigned int offset)
{
	unsigned int data;
	unsigned int timeout = 10000000;

	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x114 )) &= ~(0x3f<<24);
//	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x114 )) |= (0xa<<24);		//addr_space	1kB
	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x114 )) |= (0xc<<24);		//addr_space	4kB
//	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x114 )) |= (0x14<<24);		//addr_space	1MB
//	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x114 )) |= (0x16<<24);		//addr_space	4MB
	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x118 )) = 0x001;			//data_bist
	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x11C )) = (0x100000 + offset);	//bist_start_addr
	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x124 )) = 0x0;			//data_mask
	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x128 )) = 0x0;			//data_mask

	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x114 )) |= (0x1<<8);			//bist_go

	//wait for ddr init complete inter
	while(--timeout){
		if((*((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x16C ))>>6)&0x1){
			*((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x170 )) |= (0x1<<6);
			break;
		}
	}
	//check bist status
	data = * ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id  + 0x114 ));
	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id  + 0x114 )) &= ~(0x1<<8);		//clear bist_go
	if((data >>16)&0x1)
		return(0);	//OK
	else
		return(1);	//fail
}

static unsigned int ddr_address_bist_test(int ddr_id, unsigned int offset)
{
	unsigned int data;
	unsigned int timeout = 10000000;

	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x114 )) &= ~(0x3f<<24);
//	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x114 )) |= (0xa<<24);		//addr_space	1kB
//	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x114 )) |= (0xc<<24);		//addr_space	4kB
//	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x114 )) |= (0x14<<24);		//addr_space	1MB
	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x114 )) |= (0x16<<24);		//addr_space	4MB
	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x118 )) = 0x100;			//address_bist
	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x11C )) = (0x100000 + offset);	//bist_start_addr
	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x124 )) = 0x0;			//data_mask
	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x128 )) = 0x0;			//data_mask

	* ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x114 )) |= (0x1<<8);		//bist_go

	//wait for ddr init complete inter
	while(--timeout){
		if((*((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x16C ))>>6)&0x1){
			*((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id + 0x170 )) |= (0x1<<6);
			break;
		}
	}
	//check bist status
	data = * ((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id  + 0x114 ));
	*((volatile unsigned  int*)( 0xE1001000 + 0x1000*ddr_id  + 0x114 )) &= ~(0x1<<8);		//clear bist_go

	if(((data >>16)&0x2) == 0x2)
		return(0);	//OK
	else
		return(1);	//fail
}


#if defined(CONFIG_GATE_TRAINING_SUPPORT)
int ddr_vref_select_gate_training(int ddr_id)
{
	unsigned int i, j;
	unsigned int data, data1, data2, vref_value;
	unsigned int flag[64];
	unsigned int max_len, len, data_flag;
	unsigned int DDR_PHY_BASE = (DDR0_REG_BASE + 0x400 + 0x1000 * ddr_id);
#ifdef WRITE_BACK_LOG
	printf("\n*********************DDR%d ddr_vref_select(PHY_177)************************\n", ddr_id);
#endif

	for(i=0; i<0x20; i++)
	{
		if(i<0x20)
			data = ((1<<8)|i);
		else
			data = ((3<<8)|(i-0x20+9));

		*((volatile unsigned  int*)(0xE1001400 + 0x1000*ddr_id + 0x2C4)) = data;

		flag[i] = ddr_data_bist_test(ddr_id, 0x00100000*i);	//right = 0;

#ifdef DEBUG_LOG
		if(flag[i])
			printf("error!!!  PHY_177 = 0x%08x\n", data);
		else
			printf("right!!!  PHY_177 = 0x%08x\n", data);
#endif
	}


	/*select ddr_vref*/
	max_len = 0;
	data_flag = 0;
	len = 0;

	data1 = 0;
	data2 = 0;

	for(i=0; i<(0x20-2); i++)
	{
		if((flag[i+0] == 0) && (flag[i+1] == 0) && (flag[i+2] == 0))
		{
			data1 = i;

			for(j=0;j<(0x20-data1);j++)
			{
				i++;
				if(i >= 0x20)
				{
					data2 = i-1;
					data = (unsigned int)((data2+data1)/2);
					break;
				}

				if(flag[i] == 1)
				//if((flag[i] == 1) && (flag[i+1] == 1))
				{
					data2 = i-1;

					if(data_flag == 0)
					{
						max_len = data2 - data1 + 1;
						data = (unsigned int)((data2+data1)/2);
						data_flag = 1;
					}
					else
					{
						len = data2 - data1 + 1;
						if(len > max_len)
						{
							max_len = len;
							data = (unsigned int)((data2+data1)/2);
						}
					}
					break;
				}
			}
		}
	}


	if((data1==0) && (data2==0))
	{
		printf("ERROR!!!!  No correct parameter! DDR%d write_back(default value): PHY_177 = 0x10f\n", ddr_id);
		*((volatile unsigned  int*)(0xE1001400 + 0x1000*ddr_id + 0x2C4)) = 0x10f;
		return(1);
	}

	if(data<0x20)
		vref_value = ((1<<8)|data);
	else
		vref_value = ((3<<8)|(data-0x20+9));
	*((volatile unsigned  int*)(0xE1001400 + 0x1000*ddr_id + 0x2C4)) = vref_value;
	if (memctl_refresh_dqsdatas(ddr_id, (DDR_PHY_BASE + 0x2C4), vref_value, 0xffffffff))
		printf("memctl[%d] refresh PHY177 to 0x%x fail\n", ddr_id, vref_value);
#ifdef WRITE_BACK_LOG
	printf("write_back: PHY_177 = 0x%08x\n\n", vref_value);
#endif
	return 0;
}
#endif

static int ddr_vref_select(int ddr_id)
{
	unsigned int i, j;
	unsigned int data, data1, data2, vref_value;
	unsigned int flag[64];
	unsigned int max_len, len, data_flag;
	unsigned int DDR_PHY_BASE = (DDR0_REG_BASE + 0x400 + 0x1000 * ddr_id);
#ifdef WRITE_BACK_LOG
	printf("\n*********************DDR%d ddr_vref_select(PHY_177)************************\n", ddr_id);
#endif

	for(i=0; i<(0x40-9); i++)
	{
		if(i<0x20)
			data = ((1<<8)|i);
		else
			data = ((3<<8)|(i-0x20+9));

		*((volatile unsigned  int*)(0xE1001400 + 0x1000*ddr_id + 0x2C4)) = data;

		flag[i] = ddr_data_bist_test(ddr_id, 0x00100000*i);	//right = 0;

#ifdef DEBUG_LOG
		if(flag[i])
			printf("error!!!  PHY_177 = 0x%08x\n", data);
		else
			printf("right!!!  PHY_177 = 0x%08x\n", data);
#endif
	}


	/*select ddr_vref*/
	max_len = 0;
	data_flag = 0;
	len = 0;

	data1 = 0;
	data2 = 0;

	for(i=0; i<((0x40-9)-2); i++)
	{
		if((flag[i+0] == 0) && (flag[i+1] == 0) && (flag[i+2] == 0))
		{
			data1 = i;

			for(j=0;j<((0x40-9)-data1);j++)
			{
				i++;
				if(i >= (0x40-9))
				{
					data2 = i-1;
					data = (unsigned int)((data2+data1)/2);
					break;
				}

				if(flag[i] == 1)
				//if((flag[i] == 1) && (flag[i+1] == 1))
				{
					data2 = i-1;

					if(data_flag == 0)
					{
						max_len = data2 - data1 + 1;
						data = (unsigned int)((data2+data1)/2);
						data_flag = 1;
					}
					else
					{
						len = data2 - data1 + 1;
						if(len > max_len)
						{
							max_len = len;
							data = (unsigned int)((data2+data1)/2);
						}
					}
					break;
				}
			}
		}
	}


	if((data1==0) && (data2==0))
	{
		printf("ERROR!!!!  No correct parameter! DDR%d write_back(default value): PHY_177 = 0x10f\n", ddr_id);
		*((volatile unsigned  int*)(0xE1001400 + 0x1000*ddr_id + 0x2C4)) = 0x10f;
		return(1);
	}

	if(data<0x20)
		vref_value = ((1<<8)|data);
	else
		vref_value = ((3<<8)|(data-0x20+9));
	*((volatile unsigned  int*)(0xE1001400 + 0x1000*ddr_id + 0x2C4)) = vref_value;
	if (memctl_refresh_dqsdatas(ddr_id, (DDR_PHY_BASE + 0x2C4), vref_value, 0xffffffff))
		printf("memctl[%d] refresh PHY177 to 0x%x fail\n", ddr_id, vref_value);
#ifdef WRITE_BACK_LOG
	printf("write_back: PHY_177 = 0x%08x\n\n", vref_value);
#endif
	return 0;
}


void ddr_zq_select(int ddr_id)
{
	printf("zq_select %d\n", ddr_id);
	//dq_dqs_dm_zq_select(ddr_id);
	//instruction_clk_zq_select(ddr_id);
	//feedback_zq_select(ddr_id);
#if defined(CONFIG_GATE_TRAINING_SUPPORT)
	if (gd->need_gate_training)
		ddr_vref_select_gate_training(ddr_id);
	else
#endif
		ddr_vref_select(ddr_id);
}

