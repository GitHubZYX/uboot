#ifndef __EFUSE_H__
#define __EFUSE_H__

#define SE	0 	//security boot, 0:normal, 1:security boot
#define ST	1	//jtag enable,  0 enable, 1 use with security_ctrl.sec_sr/sec_st
#define SMC	2	//security recorder/camera 0 disable
#define SET	3	//efuse prog enable: 0 enable,1 disable
#define SSD	4	//4-5 ??
#define FL	6	//6-7,FL[0]ha7 freq lock 0:no limit, 1: <=1.5GHz;
				//FL[1]:GPU freq lock, 0:no limit, 1:<=1GHz
#define CM	8	//8-11,CM[0]HSUPA enable, 0:enable, 1:disable; CM[3:1]no use

enum EFUSE_OPSTS {
	EFUSE_OK = 0,
	EFUSE_TMOUT,
	EFUSE_AGAIN
};

#endif
