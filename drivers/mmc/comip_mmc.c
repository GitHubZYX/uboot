/*
 * (C) Copyright Leadcore
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <mmc.h>
#include <asm/io.h>

struct mmc_dmac_des {
	u32 DES0;
	u32 DES1;
	u32 DES2;
	u32 DES3;
};

struct comip_mmc_host {
	struct mmc_host host_mmc;
	struct mmc mmc_dev;
	struct mmc_dmac_des *sg_cpu;
	int index;
	int clk_read_delay;
	int clk_write_delay;
	int div;
};

#if defined(CONFIG_COMIP_EMMC_ENHANCE)
DECLARE_GLOBAL_DATA_PTR;
#else
static struct comip_mmc_host comip_mmc_host_c[4];
#endif

#define IDMAC_DES_BS						(1 << 13)
#define IDMAC_DES_ADDR						0x10000	// idmac descriptor

#define comip_mmc_writel(value, offset)		\
	writel(value, host->base + offset)

#define comip_mmc_readl(offset)			\
	readl(host->base + offset)

#define MMC_TRANSFER_TIMEOUT					(8000000) /* us. */
#define MMC_RETRY_CNT						(60000)
#define MMC_SEND_CMD_TIMEOUT					(5000000) /* us. */
#define MMC_WAIT_IDLE_TIMEOUT					(5000000) /* us. */



/**
 * Get the host address and peripheral ID for a device. Devices are numbered
 * from 0 to 3.
 *
 * @param host		Structure to fill in (base, reg, mmc_id)
 * @param dev_index	Device index (0-3)
 */
static void comip_get_setup(struct mmc_host *host, int dev_index)
{
	switch (dev_index) {
	case 1:
		host->base = SDMMC1_BASE;
		break;
	case 2:
		host->base = SDMMC2_BASE;
		break;
#if defined(CONFIG_CPU_LC1810)
	case 3:
		host->base = SDMMC3_BASE;
		break;
#endif
	case 0:
	default:
		host->base = SDMMC0_BASE;
		break;
	}
}

static int comip_mmc_wait_for_bb(struct mmc_host *host)
{
	unsigned int retry = 0;

	while ((comip_mmc_readl(SDMMC_STATUS) & (1 << SDMMC_STATUS_DATA_BUSY))
			&& (retry++ < MMC_WAIT_IDLE_TIMEOUT))
		udelay(1);

	if (retry >= MMC_WAIT_IDLE_TIMEOUT) {
		printf("wait mmc idle timeout:0x%x\n", comip_mmc_readl(SDMMC_STATUS));
		return TIMEOUT;
	}

	return 0;
}

static int __comip_mmc_send_cmd(struct mmc_host* host, unsigned int cmdat)
{
	unsigned int retry = 0;

	comip_mmc_writel(cmdat, SDMMC_CMD);

	while ((comip_mmc_readl(SDMMC_CMD) & (1 << SDMMC_CMD_START_CMD))
			&& (retry++ < MMC_SEND_CMD_TIMEOUT))
		udelay(1);

	if (retry >= MMC_SEND_CMD_TIMEOUT) {
		printf("mmc send command timeout:0x%x\n", comip_mmc_readl(SDMMC_CMD));
		return TIMEOUT;
	}

	return 0;
}

static void comip_mmc_set_clock(struct mmc_host* host, u32 clock_rate)
{
	struct comip_mmc_host *comip_host = container_of(host, struct comip_mmc_host, host_mmc);
	unsigned int cmdat = (1 << SDMMC_CMD_START_CMD)
				| (1 << SDMMC_CMD_UPDATE_CLOCK_REGISTERS_ONLY);
	unsigned int v;
	int div;
	int ret;

	if (clock_rate == 0)
		div = 0;
	else if (clock_rate == 400000)
		div = 130;
	else {
		if (comip_host->index == 1) {
			if (gd->g_mmc_usedma) {
				if (EMMC_SAMSUNG == get_emmc_vendor())
					div = 0;
				else
					div = 1;
			} else {
				if (EMMC_SAMSUNG == get_emmc_vendor())
					div = 2;
				else
					div = 1;
			}
		} else
			div = 2;
	}

	if (div == comip_host->div)
		return;

	ret = comip_mmc_wait_for_bb(host);
	if (ret)
		goto out;

	v = comip_mmc_readl(SDMMC_CLKENA);
	v &= ~(1 << SDMMC_CLKENA_CCLK_EN);
	comip_mmc_writel(v, SDMMC_CLKENA);


		/* Update clk setting to controller. */
	ret = __comip_mmc_send_cmd(host, cmdat);
	if (ret)
		goto out;

	comip_mmc_writel(div, SDMMC_CLKDIV);

	/* Update clk setting to controller. */
	ret = __comip_mmc_send_cmd(host, cmdat);
	if (ret)
		goto out;

	v |= (1 << SDMMC_CLKENA_CCLK_EN);
	comip_mmc_writel(v, SDMMC_CLKENA);

	/* Update clk setting to controller. */
	ret = __comip_mmc_send_cmd(host, cmdat);
	if (ret)
		goto out;

	comip_host->div = div;

out:
	if (ret) {
		printf("set mmc clock(%d) fail\n", clock_rate);
		pmic_reboot();
	}
}

static void comip_reset_fifo(struct mmc_host *host)
{
	unsigned int v;
	unsigned int i = 0;

	/* Reset SDMMC FIFO.  */
	v = comip_mmc_readl(SDMMC_CTRL);
	v |= (1 << SDMMC_CTRL_FIFO_RESET);
	comip_mmc_writel(v, SDMMC_CTRL);

	while ((comip_mmc_readl(SDMMC_CTRL) & (1 << SDMMC_CTRL_FIFO_RESET))
			&& (i++ < MMC_RETRY_CNT));

	if (i >= MMC_RETRY_CNT)
		printf("DMA_RESET reset timeout:0x%x\n" ,comip_mmc_readl(SDMMC_CTRL));
}

static void comip_reset_idmac(struct mmc_host *host)
{
	unsigned int v;

	/* dma disable. reset idma internal logic. */
	v = comip_mmc_readl(SDMMC_BMOD);
	v &= ~(1 << SDMMC_BMOD_DE);
	comip_mmc_writel(v, SDMMC_BMOD);
	v |= (1 << SDMMC_BMOD_SWR);
	comip_mmc_writel(v, SDMMC_BMOD);

	/* Select idmac interface, reset idma interface. */
	v = comip_mmc_readl(SDMMC_CTRL);
	v &= ~(1 << SDMMC_CTRL_USE_INTERNAL_DMAC);
	comip_mmc_writel(v, SDMMC_CTRL);
	v |=(1 << SDMMC_CTRL_DMA_RESET);
	comip_mmc_writel(v, SDMMC_CTRL);

	v = 0;
	while ((comip_mmc_readl(SDMMC_CTRL) & (1 << SDMMC_CTRL_DMA_RESET))
			 && (v++ < MMC_RETRY_CNT));
	if (v >= MMC_RETRY_CNT)
		printf("DMA_RESET reset timeout:0x%x\n" ,comip_mmc_readl(SDMMC_CTRL));
}


#if defined(CONFIG_COMIP_MMC_DMA) || defined(CONFIG_COMIP_EMMC_ENHANCE)
static void comip_mmc_prepare_data_dma(struct mmc_host *host, struct mmc_data *data)
{
	struct comip_mmc_host *comip_host = container_of(host, struct comip_mmc_host, host_mmc);
	struct mmc_dmac_des *sg_desc_ptr = comip_host->sg_cpu;
	unsigned int sg_blk_num = 0;
	unsigned int sg_blk_len = 0;
	unsigned int i = 0;
	unsigned int v;
	char *ptr;

	if (MMC_DATA_WRITE == data->flags)
		ptr = (char*)data->src;
	else
		ptr = data->dest;

	comip_reset_idmac(host);
	comip_reset_fifo(host);

	comip_mmc_writel(0xffffffff, SDMMC_RINTSTS);
	comip_mmc_writel(data->blocks*data->blocksize, SDMMC_BYTCNT);
	comip_mmc_writel(data->blocksize, SDMMC_BLKSIZ);

	sg_blk_num = 1 + (data->blocks * data->blocksize) / IDMAC_DES_BS;
	sg_blk_len = (data->blocks * data->blocksize) % IDMAC_DES_BS;

	for (i = 0; i < sg_blk_num; i++) {
		sg_desc_ptr[i].DES1 = IDMAC_DES_BS;
		sg_desc_ptr[i].DES2 = (u32)ptr;
		//bit 31:own,bit 4:sg mode
		sg_desc_ptr[i].DES0 = (1 << 31) | (1 << 4);
		if (0 == i) //fisrt des
			sg_desc_ptr[i].DES0 |= 1 << 3;
		if ((sg_blk_num - 1) == i) { //last des
			sg_desc_ptr[i].DES0 |= 1 << 2; 
			if (sg_blk_len) {
				sg_desc_ptr[i].DES1 = sg_blk_len;
			}
		}

		ptr += IDMAC_DES_BS;
		sg_desc_ptr[i].DES3 = (u32)(sg_desc_ptr + (i + 1));
	}

	comip_mmc_writel((u32)sg_desc_ptr, SDMMC_DBADDR);

	/* Start idmac. */
	v = comip_mmc_readl(SDMMC_CTRL);
	v |= 1 << SDMMC_CTRL_USE_INTERNAL_DMAC;
	comip_mmc_writel( v, SDMMC_CTRL);

	v = comip_mmc_readl(SDMMC_BMOD);
	v |= (1 << SDMMC_BMOD_DE | 1 << SDMMC_BMOD_FB);
	comip_mmc_writel( v, SDMMC_BMOD);

	/* exit idmac powerdown. */
	comip_mmc_writel(1, SDMMC_PLDMND);
}

static int comip_mmc_transfer_dma(struct mmc_host* host, struct mmc_data *data)
{
	unsigned int retry = 0;
	unsigned int status_word;
	int result = 0;

	/* Wait until data has been written. */
	do {
		status_word = comip_mmc_readl(SDMMC_RINTSTS);
		udelay(1);
	} while (((status_word & MMC_TRANSFER_MASK) == 0)
			&&(retry++ < MMC_TRANSFER_TIMEOUT));

	if (MMC_DATA_WRITE == data->flags) {
		status_word &= ~(1<<SDMMC_DCRC_INTR);
		debug("write status_word:0x%x\n", status_word);
	}

	if (status_word & (MMC_TRANSFER_ERROR_MASK)) {
		result = comip_mmc_readl(SDMMC_RINTSTS);
		debug("status_word:0x%x result:0x%x\n", status_word, result);
	}

	if (retry >= MMC_TRANSFER_TIMEOUT) {
		printf("time out :%d\n", retry);
		result = TIMEOUT;
	}

	comip_mmc_writel(0xffffffff, SDMMC_RINTSTS);
	comip_reset_idmac(host);

	return result;
}

#endif


#if !defined(CONFIG_COMIP_MMC_DMA) || defined(CONFIG_COMIP_EMMC_ENHANCE)

static void comip_mmc_prepare_data(struct mmc_host *host, struct mmc_data *data)
{
	unsigned int v;

	/* Enable cpu mode in MMC_CTL register. */
	v = comip_mmc_readl(SDMMC_CTRL);
	v &=~(1 << SDMMC_CTRL_DMA_ENABLE);
	comip_mmc_writel(v, SDMMC_CTRL);
	comip_reset_fifo(host);

	comip_mmc_writel(data->blocksize, SDMMC_BLKSIZ);
	comip_mmc_writel(data->blocks*data->blocksize, SDMMC_BYTCNT);
}

static int comip_mmc_transfer(struct mmc_host* host, struct mmc_data *data)
{
	int result = 0;
	unsigned long retry = 0;
	volatile unsigned int mmci_status;
	const char *data_buff_src = data->src;
	char *data_buff_dest = data->dest;
	unsigned int data_len = data->blocks*data->blocksize;

	if (MMC_DATA_WRITE == data->flags ) {
		while (data_len) {
			u32 fifo_data;
			while (0==(comip_mmc_readl(SDMMC_STATUS) & (1<<1)));
			fifo_data=0;
			if(data_len){
				fifo_data |= ((u32)data_buff_src[0]) <<0;
				data_buff_src++;
				data_len--;
			}
			if(data_len){
				fifo_data |= ((u32)data_buff_src[0]) <<8;
				data_buff_src++;
				data_len--;
			}
			if(data_len){
				fifo_data |= ((u32)data_buff_src[0]) <<16;
				data_buff_src++;
				data_len--;
			}
			if(data_len){
				fifo_data |= ((u32)data_buff_src[0]) <<24;
				data_buff_src++;
				data_len--;
			}
			
			comip_mmc_writel(fifo_data, SDMMC_FIFO);
		}
	} else {
		if (data_len) {
			u32 remain = data_len & 0x03;
			u32 fifo_data;
			if ((u32)data_buff_dest & 0x03) {
				debug("transferR:data_len:0x%x,data_buff:0x%x\n",data_len,(u32)data_buff_dest);
				goto copy_one_by_one;
			}

			while(data_len >= 4) {
				while (0==(comip_mmc_readl(SDMMC_STATUS) & (1<<0)));
				fifo_data = comip_mmc_readl(SDMMC_FIFO);
				*((u32 *)data_buff_dest) = fifo_data;
				data_buff_dest+=4;
				data_len -= 4;
			}
			if (remain) {
				while (0==(comip_mmc_readl(SDMMC_STATUS) & (1<<0)));
				fifo_data = comip_mmc_readl(SDMMC_FIFO);

				if(remain == 3) {
					*((u16 *)data_buff_dest) = (fifo_data & 0xffff);
					data_buff_dest +=2;
					*(u8 *)data_buff_dest =( fifo_data>>16) & 0xff;
				} else if (remain == 2)	{
					*((u16 *)data_buff_dest) = (fifo_data & 0xffff);
					data_buff_dest+=2;
				} else {
					*(u8 *)data_buff_dest = (fifo_data & 0xff);
				}
			}
			goto check_status;
		}
copy_one_by_one:
		while (data_len) {
			u32 fifo_data;
			while (0==(comip_mmc_readl(SDMMC_STATUS) & (1<<0)));
		
			fifo_data=comip_mmc_readl(SDMMC_FIFO);
			if(data_len){
				*data_buff_dest= (fifo_data >>0) &0xff;
				data_buff_dest++;
				data_len--;
			}
			if(data_len){
				*data_buff_dest= (fifo_data >>8) &0xff;
				data_buff_dest++;
				data_len--;
			}
			if(data_len){
				*data_buff_dest= (fifo_data >>16) &0xff;
				data_buff_dest++;
				data_len--;
			}
			if(data_len){
				*data_buff_dest= (fifo_data >>24) &0xff;
				data_buff_dest++;
				data_len--;
			}
		}
	}

check_status:

	mmci_status = comip_mmc_readl(SDMMC_RINTSTS);

	/* wait until data has been written */
	retry = 0;
	do {
		mmci_status = comip_mmc_readl(SDMMC_RINTSTS);
	} while ( ( (mmci_status & MMC_TRANSFER_MASK) == 0) &&
		  (retry++ < MMC_TRANSFER_TIMEOUT) );


	if(MMC_DATA_WRITE == data->flags)
		mmci_status &= ~(1<<SDMMC_DCRC_INTR);
	if (mmci_status & (MMC_TRANSFER_ERROR_MASK) ) {
		result = comip_mmc_readl(SDMMC_RINTSTS);
	}

	if (retry>=MMC_TRANSFER_TIMEOUT)
		result=TIMEOUT;

	comip_mmc_writel(0xffffffff, SDMMC_RINTSTS);

	return result;
}
#endif

extern void put_error(char * err_str);

static void  comip_mmc_dump_register(struct mmc *mmc,
					        unsigned int i ,
					        unsigned int status_word,
					        unsigned int flag)
{
	struct mmc_host *host = (struct mmc_host *)mmc->priv;
	char str_status_word[100];
	sprintf(str_status_word, "%x->%x/%x/%x/%x/%x/%x/%x/%x/%x/%x/%x/%x/%x",
		flag,
		status_word,
		i,
		comip_mmc_readl(SDMMC_RINTSTS),
		comip_mmc_readl(SDMMC_CMD),
		comip_mmc_readl(SDMMC_CMDARG),
		comip_mmc_readl(SDMMC_STATUS),
		comip_mmc_readl(SDMMC_MINTSTS),
		comip_mmc_readl(SDMMC_BMOD),
		comip_mmc_readl(SDMMC_PLDMND),
		comip_mmc_readl(SDMMC_IDSTS),
		comip_mmc_readl(SDMMC_IDINTEN),
		comip_mmc_readl(SDMMC_DSCADDR),
		comip_mmc_readl(SDMMC_BUFADDR)
		);
		put_error(str_status_word);

}

static int comip_mmc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd,
						struct mmc_data *data)
{
	struct mmc_host *host = (struct mmc_host *)mmc->priv;
	unsigned int flags = 0;
	unsigned int i = 0;
	unsigned int status_word;
	int result=0;

	flags |= (1 << SDMMC_CMD_START_CMD);
	flags |= (1 << SDMMC_CMD_USE_HOLD_REG);
	flags |= (1 << SDMMC_CMD_WAIT_PRVDATA_COMPLETE);

	if (data) {
		#if defined(CONFIG_COMIP_EMMC_ENHANCE)
		if (gd->g_mmc_usedma)
			comip_mmc_prepare_data_dma(host, data);
		else
			comip_mmc_prepare_data(host, data);
		#else
			#ifdef CONFIG_COMIP_MMC_DMA
			comip_mmc_prepare_data_dma(host, data);
			#else
			comip_mmc_prepare_data(host, data);
			#endif
		#endif
	}else{
		comip_reset_idmac(host);
		comip_reset_fifo(host);
	}

	comip_mmc_writel(0xffffffff, SDMMC_RINTSTS);
	comip_mmc_writel(cmd->cmdarg, SDMMC_CMDARG);

	if (cmd->resp_type & MMC_RSP_PRESENT)
		flags |= (1 << SDMMC_CMD_RESPONSE_EXPECT);
	if (cmd->resp_type & MMC_RSP_136)
		flags |= (1 << SDMMC_CMD_RESPONSE_LENGTH);

	if (data) {
		flags |= (1 << SDMMC_CMD_DATA_TRANSFER_EXPECTED);

		if (! ( (cmd->cmdidx == MMC_CMD_WRITE_MULTIPLE_BLOCK)
			|| (cmd->cmdidx == MMC_CMD_WRITE_SINGLE_BLOCK)) ) {
			flags |= (1 << SDMMC_CMD_CHECK_RESPONSE_CRC);
		}

		if (MMC_DATA_WRITE == data->flags)
			flags |= (1 << SDMMC_CMD_READ_WRITE);
	}

	comip_mmc_writel((cmd->cmdidx | flags), SDMMC_CMD);

	do {
		status_word = comip_mmc_readl(SDMMC_RINTSTS);
	} while ( ((status_word & ((1<<SDMMC_CRCERR_INTR) | (1<<SDMMC_RCRC_INTR)
				|(1<<SDMMC_HLE_INTR)| (1<<SDMMC_RTO_INTR) | (1 << SDMMC_CD_INTR) )) == 0)
			 && (i++ < MMC_RETRY_CNT) );

	if((status_word & ((1<<SDMMC_HLE_INTR)|(1<<SDMMC_CRCERR_INTR) | (1<<SDMMC_RCRC_INTR) | (1<<SDMMC_RTO_INTR) ))) {
		debug("error :0x%x\n", status_word);
		comip_mmc_dump_register(mmc,i,status_word,1);
		return TIMEOUT;
	}

	if (i >= MMC_RETRY_CNT) {
		debug("time out %d, status_word :0x%x\n", i, status_word);
		comip_mmc_dump_register(mmc,i,status_word,2);
		return TIMEOUT;
	}

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			cmd->response[0] = comip_mmc_readl(SDMMC_RESP3);
			cmd->response[1] = comip_mmc_readl(SDMMC_RESP2);
			cmd->response[2] = comip_mmc_readl(SDMMC_RESP1);
			cmd->response[3] = comip_mmc_readl(SDMMC_RESP0);
		} else {
			cmd->response[0] = comip_mmc_readl(SDMMC_RESP0);
		}
	}

	comip_mmc_writel((1 << SDMMC_CD_INTR), SDMMC_RINTSTS);

	if (data) {
		#if defined(CONFIG_COMIP_EMMC_ENHANCE)
		if (gd->g_mmc_usedma)
			result = comip_mmc_transfer_dma(host, data);
		else
			result = comip_mmc_transfer(host, data);
		#else
			#ifdef CONFIG_COMIP_MMC_DMA
			result = comip_mmc_transfer_dma(host, data);
			#else
			result = comip_mmc_transfer(host, data);
			#endif
		#endif
	}

	comip_mmc_writel(0xffffffff, SDMMC_RINTSTS);

	if (result) {
		comip_mmc_dump_register(mmc,i,status_word,3);
	}

	return result;
}
static void comip_mmc_set_ios(struct mmc *mmc)
{
	struct mmc_host *host = mmc->priv;

	if (8 == mmc->bus_width)
		comip_mmc_writel(0x10000, SDMMC_CTYPE);
	else if (4 == mmc->bus_width)
		comip_mmc_writel(1, SDMMC_CTYPE);
	else if (1 == mmc->bus_width)
		comip_mmc_writel(0, SDMMC_CTYPE);

	comip_mmc_set_clock(host, mmc->clock);
}

static int comip_mmc_core_init(struct mmc *mmc)
{
	struct mmc_host *host = (struct mmc_host *)mmc->priv;
	struct comip_mmc_host *comip_host = container_of(host, struct comip_mmc_host, host_mmc);
	unsigned int status;
	unsigned int i;

#ifdef CONFIG_COMIP_MMC_PULLUP
	#if defined(CONFIG_CPU_LC1810) || defined(CONFIG_CPU_LC1813)
		/* Enable pull. */
		if (comip_host->index == 1)
			writel(0xffff, MUX_PIN_SDMMC0_PAD_CTRL + comip_host->index * 4);

	#elif defined(CONFIG_CPU_LC1860)
		if (comip_host->index == 1) {
			int i;
			unsigned char mmc1_pins[] =
				{111, 106, 103, 110, 112, 113, 119, 118, 122}; /*MMCD 0~7, cmd */

			for (i = 0; i < ARRAY_SIZE(mmc1_pins); i++) {
				comip_mfp_config_pull(mmc1_pins[i], MFP_PULL_UP);
			}

		} else
			printf("error ! nothing to be done for pull sdmmc%d pins\n", comip_host->index);

	#else
		#error nothing to mach
	#endif
#endif

#if defined(CONFIG_CPU_LC1810) || defined(CONFIG_CPU_LC1813)
	/* Enable clock. */
	status = ((1 << (21 + comip_host->index)) | (1 << (5 + comip_host->index)));
	writel(status, (AP_PWR_SECPCLK_EN));

	/* Set clock rate. */
	/* Clock rate = (pll1_out / (2 * (div + 1))) * (gr / 8) =  (1248M / 6) * (1 / 2) = 104M. */
	status = 2 << 8 | (comip_host->clk_read_delay << 4) | (comip_host->clk_write_delay << 0);
	writel(status, (AP_PWR_SDMMC0CLKCTL + comip_host->index * 4));
	status = (1 << (16 + comip_host->index)) | (4 << (4 * comip_host->index));
	writel(status, (AP_PWR_SDMMCCLKGR_CTL));

	/* Reset SDMMC. */
	status = (1 << (6 + comip_host->index));
	writel(status, (AP_PWR_MOD_RSTCTL4));
	mdelay(5);

#elif defined(CONFIG_CPU_LC1860)
	/* Enable clock. */
	status = ((1 << (12 + comip_host->index)) | (1 << (28 + comip_host->index)));
	writel(status, (AP_PWR_SDMMCCLK_CTL0));

	/* Set clock rate. */
	/* Clock rate = parent_rate / (div0 + 1) / (2 * (div1 + 1)) =  1248M  / (1 + 1) / (2 * (2 + 1))= 104M. */
	if(gd->g_mmc_usedma) {
		if (EMMC_SAMSUNG == get_emmc_vendor())
			status = 5 << 8 | (comip_host->clk_read_delay << 4) | (comip_host->clk_write_delay << 0);
		else
			status = 2 << 8 | (comip_host->clk_read_delay << 4) | (comip_host->clk_write_delay << 0);
	} else
		status = 0x200;

	writel(status, (AP_PWR_SDMMC0CLK_CTL1 + comip_host->index * 4));
	status = (7 << (16 + comip_host->index * 4)) | (1 << (4 * comip_host->index));
	writel(status, (AP_PWR_SDMMCCLK_CTL0));

	/* Reset SDMMC. */
	if (comip_host->index == 1)
		writel(0x08, AP_PWR_HBLK0_RSTCTL);
	else if (comip_host->index == 0) {
		status = 1 << 5;
		writel(status, (AP_PWR_MOD_RSTCTL0));
	} else if (comip_host->index == 2) {
		status = 1 << 6;
		writel(status, (AP_PWR_MOD_RSTCTL0));
	}
	mdelay(5);

#else
	#error nothing to mach
#endif

	/* Reset. */
	status = comip_mmc_readl(SDMMC_CTRL);
	status |= (1 << SDMMC_CTRL_CONTROLLER_RESET);
	status |= (1 << SDMMC_CTRL_FIFO_RESET);
#ifdef CONFIG_COMIP_MMC_DMA
	#if defined(CONFIG_COMIP_EMMC_ENHANCE)
	if (gd->g_mmc_usedma)
	#endif
	{
		status |= (1 << SDMMC_CTRL_DMA_RESET);
		status |= (1 << SDMMC_CTRL_DMA_ENABLE);
	}
#endif
	status |= (1 << SDMMC_CTRL_INT_ENABLE);
	comip_mmc_writel(status, SDMMC_CTRL);

	/* Wait controller reset done. */
	i = 0;
	do {
		status = comip_mmc_readl(SDMMC_CTRL);
	}while((status & (1 << SDMMC_CTRL_CONTROLLER_RESET))
		&& (i++ < MMC_RETRY_CNT));
	if (i >= MMC_RETRY_CNT)
		printf("CONTROLLER_RESET reset timeout:0x%x\n", status);

	//Wait no dma_req
	i = 0;
	do {
		status = comip_mmc_readl(SDMMC_STATUS);
	}while((status & (1 << SDMMC_STATUS_DMA_REQ))
		&& (i++ < MMC_RETRY_CNT));
	if ( i >= MMC_RETRY_CNT)
		printf("CONTROLLER_RESET reset timeout:0x%x\n" ,comip_mmc_readl(SDMMC_CTRL));

	/* Clear all interrupt. */
	comip_mmc_writel(0xFFFFFFFF, SDMMC_RINTSTS);

	/* Set debounce time 25ms. */
	comip_mmc_writel(0x94C5F, SDMMC_DEBNCE);
	comip_mmc_writel(0xFFFFFFFF, SDMMC_TMOUT);

	/* Set fifo threshold. */
#if defined(CONFIG_COMIP_EMMC_ENHANCE)
	if (gd->g_mmc_usedma) {
		if (EMMC_SAMSUNG == get_emmc_vendor()) {
			comip_mmc_writel(0x307f0080, SDMMC_FIFOTH);
			comip_mmc_writel(0x02000001, SDMMC_CARDTHRCTL);
		} else
			comip_mmc_writel(0x10030004, SDMMC_FIFOTH);
	}else
		comip_mmc_writel(0x00000001, SDMMC_FIFOTH);
#else
#ifdef CONFIG_COMIP_MMC_DMA
	if (EMMC_SAMSUNG == get_emmc_vendor()) {
		comip_mmc_writel(0x307f0080, SDMMC_FIFOTH);
		comip_mmc_writel(0x02000001, SDMMC_CARDTHRCTL);
	} else
		comip_mmc_writel(0x10030004, SDMMC_FIFOTH);
#else
	comip_mmc_writel(0x00000001, SDMMC_FIFOTH);
#endif
#endif

	/* 1 bit card. */
	comip_mmc_writel(0, SDMMC_CTYPE);
	comip_mmc_writel(0, SDMMC_BYTCNT);

	/* 400K. */
	comip_mmc_set_clock(host, 400000);

	status = comip_mmc_readl(SDMMC_CMD);
	status |= 1 << SDMMC_CMD_SEND_INITIALIZATION;
	comip_mmc_writel(status, SDMMC_CMD);

	udelay(1);

	status = comip_mmc_readl(SDMMC_CMD);
	status &= ~(1 << SDMMC_CMD_SEND_INITIALIZATION);
	comip_mmc_writel(status, SDMMC_CMD);

	return 0;
}

int comip_mmc_init(int dev_index, int bus_width, int clk_write_delay, int clk_read_delay)
{
	struct comip_mmc_host *comip_host;
	struct mmc_host *host;
	struct mmc *mmc;

#if defined(CONFIG_COMIP_EMMC_ENHANCE)
	gd->g_comip_mmc_host[gd->g_cur_dev_num] = (void *)&gd->g_mmc_info[gd->g_mmc_info_used];
	gd->g_mmc_info_used += (sizeof(struct comip_mmc_host) + 3) & ~3;
	if (gd->g_mmc_info_used >= CONFIG_MMC_STRUCT_IN_GD)
		printf("Must set CONFIG_MMC_STRUCT_IN_GD larger than %d\n",
			sizeof(struct list_head) + sizeof(struct comip_mmc_host));
	comip_host = gd->g_comip_mmc_host[gd->g_cur_dev_num];
	debug("curdev:%d, mmc_info_used:%ld\n", gd->g_cur_dev_num, gd->g_mmc_info_used);
#else
	memset(&comip_mmc_host_c, 0, sizeof(comip_mmc_host_c));
	comip_host = &comip_mmc_host_c[dev_index];
#endif
	comip_host->index = dev_index;
	comip_host->clk_read_delay = clk_read_delay;
	comip_host->clk_write_delay = clk_write_delay;
	comip_host->div = -1;
	comip_host->sg_cpu = (struct mmc_dmac_des *)IDMAC_DES_ADDR;

	host = &comip_host->host_mmc;
	host->clock = 0;
	comip_get_setup(host, dev_index);

	mmc = &comip_host->mmc_dev;
	sprintf(mmc->name, "SD/MMC%d", dev_index);
	mmc->priv = host;
	mmc->send_cmd = comip_mmc_send_cmd;
	mmc->set_ios = comip_mmc_set_ios;
	mmc->init = comip_mmc_core_init;

	mmc->voltages = MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31
			| MMC_VDD_31_32 | MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;
	if (bus_width == 8)
		mmc->host_caps = MMC_MODE_8BIT;
	else
		mmc->host_caps = MMC_MODE_4BIT;
	mmc->host_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS | MMC_MODE_HC;

	mmc->f_min = 400000;
	mmc->f_max = 13000000;

	mmc_register(mmc);

	return 0;
}

