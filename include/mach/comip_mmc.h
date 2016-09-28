
#ifndef __COMIP_MMC_H_
#define __COMIP_MMC_H_

struct mmc_host {
	unsigned int version;	/* SDHCI spec. version */
	unsigned int clock;	/* Current clock (MHz) */
	unsigned int base;	/* Base address, SDMMC1/2/3/4 */
	int pwr_gpio;		/* Power GPIO */
	int cd_gpio;		/* Change Detect GPIO */
};

extern int comip_mmc_init(int dev_index, int bus_width, int clk_write_delay, int clk_read_delay);

#endif	/* __COMIP_MMC_H_ */
