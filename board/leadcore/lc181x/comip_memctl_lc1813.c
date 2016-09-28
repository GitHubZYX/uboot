
#include <common.h>

DECLARE_GLOBAL_DATA_PTR;

void lpddr2_400MHz_config(void)
{
	unsigned int loop;
	unsigned int data;
	unsigned int data2;
	printf("ddr init\n");

	// set ddr_axi_ckl to 416M
	data = (1<<AP_PWR_DDRAXICLK_CTL_DDR_AXI_CLK_DR_WE)|(0<<AP_PWR_DDRAXICLK_CTL_DDR_AXI_CLK_DR)|(1<<AP_PWR_DDRAXICLK_CTL_DDR_AXI_CLK_GR_WK_WE)|8;
	*((volatile unsigned  int*)AP_PWR_DDRAXICLK_CTL) = data;

	//Start LPDDR2 Configuration
#if (CONFIG_DDR_BUS_CLK==195000000)
	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x14 )) = 0x03; //clk_freq	//195mhz
#else
	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x14 )) = 0x01; //clk_freq
#endif
	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x04 )) = 0x14; // pll stablization time
	
	//close dto ato pad
	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x094 )) |= (0x7<<9) | (0x7<<1);

	//dll reset
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x14 )) &= 0xbfffffff; 	//acdllcr
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x1cc )) &= 0xbfffffff;  	//dx0dllcr
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x20c )) &= 0xbfffffff;  	//dx1dllcr
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x24c )) &= 0xbfffffff;  	//dx2dllcr
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x28c )) &= 0xbfffffff;  	//dx3dllcr

	for(loop = 0; loop < 1000; loop++);

	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x38 )) = 0x02000200;

	for(loop = 0; loop < 1000; loop++);

	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x34 )) = 0x10000;
	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x150 )) = 0x40000;
	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x38 )) = 0x02000000;
	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x34 )) = 0x10001;
	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x150 )) = 0x40100;

	for(loop = 0; loop < 10000; loop++);

	//disable dll reset
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x14 )) |= (1<<30);
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x1cc )) |= (1<<30);
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x20c )) |= (1<<30);
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x24c )) |= (1<<30);
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x28c )) |= (1<<30);

	for(loop = 0; loop < 1000; loop++);

#if CONFIG_DDR_LP_ENABLE
	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x98 )) = 0x0000400;
#else
	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x98 )) = 0x1010401;
#endif

	//enable ddr_pclk
	//config ddr_pwr
	data = *((volatile unsigned  int*)( DDR_PWR_BASE + 0x30 )) | 0x30003;
	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x30 )) = data;

	//modify IO para
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x24 )) &= 0xdfffffff; //ACIOCR
	//swg_120422 *((volatile unsigned  int*)( PHY_DDR_BASE + 0x2c )) = 0x9900025f;	//NL2PD=1
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x2c )) = 0x95f0025f;	//NL2PD=1
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x1C0 )) &= 0xfffff9ff;  //DX0GCR
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x200 )) &= 0xfffff9ff; //DX1GCR
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x240 )) &= 0xfffff9ff; //DX2GCR
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x280 )) &= 0xfffff9ff; //DX3GCR
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x10 )) |=(0x3<<2) ; //DLLGCR
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x14 )) &=~(0x1<<30) ; //ACDLLCR
#if defined(CONFIG_COMIP_UST802_V1_0) || defined(CONFIG_COMIP_UST802_V1_1)
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x184 )) = 0x7d; //ZQ0CR1
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x194 )) = 0x7d; //ZQ1CR1
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x1a4 )) = 0x7d; //ZQ2CR1
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x1b4 )) = 0x7d; //ZQ3CR1
#else
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x184 )) = 0x79; //ZQ0CR1
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x194 )) = 0x79; //ZQ1CR1
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x1a4 )) = 0x79; //ZQ2CR1
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x1b4 )) = 0x79; //ZQ3CR1
#endif

	for(loop=0;loop<1000;loop++);
	//clk out enable
	data = *((volatile unsigned  int*)( DDR_PWR_BASE + 0x34 )) | 0xc030c03;
	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x34 )) = data;

#if defined(CONFIG_16GBIT_DDR)
	//cs0 address map; 10bit col, 15bit row;8bank; adr_map={cs,bank,row,col}
	data = (*((volatile unsigned	int*)( DDR_PWR_BASE + 0x040 )) | 0xa030f) & 0xfffaffff;
	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x40 )) = data;

	//cs1 address map
	data = (*((volatile unsigned	int*)( DDR_PWR_BASE + 0x044 )) | 0xa030f) & 0xfffaffff;
	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x44 )) = data;
#else
	//cs0 address map; 10bit col, 14bit row;8bank; adr_map={cs,bank,row,col}
	//data = (*((volatile unsigned	int*)( DDR_PWR_BASE + 0x040 )) | 0xa030f) & 0xfffaffff;
	data = (*((volatile unsigned  int*)( DDR_PWR_BASE + 0x040 )) | 0xa030e) & 0xfffaffff;
	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x40 )) = data;

	//cs1address map
	//data = (*((volatile unsigned	int*)( DDR_PWR_BASE + 0x044 )) | 0xa030f) & 0xfffaffff;
	data = (*((volatile unsigned  int*)( DDR_PWR_BASE + 0x044 )) | 0xa030e) & 0xfffaffff;
	*((volatile unsigned  int*)( DDR_PWR_BASE + 0x44 )) = data;
#endif

	//STEP 1:  Assert and release reset
	//read ctrl STAT
	data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x08 ));
	//write ctrl TOGCNT1U
	//   *((volatile unsigned	int*)( CTRL_DDR_BASE + 0xc0 )) = 0x190;    //400
#if (CONFIG_DDR_BUS_CLK == 195000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc0 )) = 0xc3;	//195mhz
#elif (CONFIG_DDR_BUS_CLK == 312000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc0 )) = 0x138;	//312mhz
#elif (CONFIG_DDR_BUS_CLK == 351000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc0 )) = 0x15f;	//351mhz
#else
	/* Default 390MHz. */
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc0 )) = 0x186;	//390mhz
#endif
	//write ctrl TINIT
	// *((volatile unsigned	int*)( CTRL_DDR_BASE + 0xc4 )) = 0x1;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc4 )) = 0xc8;
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0xc4 )) = 0xff;
	//write ctrl TOGCNT100N
	//*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xcc )) = 0x28;
#if (CONFIG_DDR_BUS_CLK == 195000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xcc )) = 0x13;	//195mhz
#elif (CONFIG_DDR_BUS_CLK == 312000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xcc )) = 0x1f;	//312mhz
#elif (CONFIG_DDR_BUS_CLK == 351000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xcc )) = 0x23;	//351mhz
#else
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xcc )) = 0x27;	//390mhz
#endif
	//write ctrl TRSTH
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc8 )) = 0x0;

	//STEP 2:  Configure uPCTL MCFG
	//write ctrl MCFG
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0x80 )) = 0xf60001;
	//*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x80 )) = 0xf60041; //BY ZHENGJR 1229
		*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x80 )) = 0xe60041; //BY ZHENGJR 1229   bl=8byte

	//STEP 3:  Start PHY initialization
	//read phy PUB_PGCR
	data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x08 ));
	//write phy PUB_PGCR
	//swg_120422 *((volatile unsigned  int*)( PHY_DDR_BASE + 0x08 )) = 0x018c2e02;
	//*((volatile unsigned  int*)( PHY_DDR_BASE + 0x08 )) = 0x014c2e02; //bit23 bit22
#if defined(CONFIG_4GBIT_DDR)
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x08 )) = 0x01442e02;	//BY ZHENGJR 0528 for 4gb ddr
#elif defined(CONFIG_8GBIT_DDR) || defined(CONFIG_16GBIT_DDR)
  *((volatile unsigned  int*)( PHY_DDR_BASE + 0x08 )) = 0x014c2e02; //bit23 bit22
#endif
	//read phy PUB_DCR
	data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x30 ));
	//write phy PUB_DCR
	//	   *((volatile unsigned  int*)( PHY_DDR_BASE + 0x30 )) = 0x10c; //ddr2-s2
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x30 )) = 0x00c; //BY ZHENGJR 1229 //ddr2-s4
	//swg_new_120422 *((volatile unsigned  int*)( PHY_DDR_BASE + 0x30 )) = 0x800000c; //BY ZHENGJR 1229 //ddr2-s4 //memtest max 9 mins
	//read phy PUB_DXCCR
	data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x28 ));
	//write phy PUB_DXCCR
	if (cpu_is_lc1813s())
		*((volatile unsigned  int*)( PHY_DDR_BASE + 0x28 )) = 0x8c40;//50ohm
	else
		*((volatile unsigned  int*)( PHY_DDR_BASE + 0x28 )) = 0xc40;//50ohm
	//swg_new_120422 *((volatile unsigned  int*)( PHY_DDR_BASE + 0x28 )) = 0xf70;//50ohm
	//read phy PUB_MR0
	data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x40 ));
	//write phy PUB_MR0
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x40 )) = 0x421;
	//read phy PUB_MR1
	data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x44 ));
	//write phy PUB_MR1
//	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x44 )) = 0x084;  //ZJR BL=16byte
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x44 )) = 0x08b;  //ZJR BL=8byte ,wrap mode
	//read phy PUB_MR2
	data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x48 ));
	//write phy PUB_MR2
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x48 )) = 0x4;	//zjr RL = 6 / WL = 3
	//read phy PUB_MR3
	data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x4c ));
	//write phy PUB_MR3
	//*((volatile unsigned	int*)( PHY_DDR_BASE + 0x4c )) = 0x4;  //60OHM
	//*((volatile unsigned	int*)( PHY_DDR_BASE + 0x4c )) = 0x3;  //48OHM
	//*((volatile unsigned  int*)( PHY_DDR_BASE + 0x4c )) = 0x2;  //40OHM
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x4c )) = 0x1; // zjr 34.3ohm
	//read phy PUB_DSGCR
	data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x2c ));
	//write phy PUB_DSGCR
	//*((volatile unsigned	int*)( PHY_DDR_BASE + 0x2c )) = 0xfa00025f;
	//*((volatile unsigned	int*)( PHY_DDR_BASE + 0x2c )) = 0xf800025f;   //NL2OE=0
	//*((volatile unsigned	int*)( PHY_DDR_BASE + 0x2c )) = 0xf900025f;   //NL2PD=1
	//read phy PUB_DTPR0
	data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x34 ));
	//write phy PUB_DTPR0
	//	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x34 )) = 0x3691aa6d;
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x34 )) = 0x3091886e; //t_rc=0x18 _
	//read phy PUB_DTPR1
	data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x38 ));
	//write phy PUB_DTPR1
	//*((volatile unsigned	int*)( PHY_DDR_BASE + 0x38 )) = 0x193400a0;
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x38 )) = 0x193300a0; //t_rfc=0x33
	//read phy PUB_DTPR2
	data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x3c ));
	//write phy PUB_DTPR2
	//*((volatile unsigned	int*)( PHY_DDR_BASE + 0x3c )) = 0x1001a0c8;
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x3c )) = 0x10018c38; //t_xs=0x38, t_xp=0x3
	//write phy PUB_PTR0
	data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x18 ));
	//*((volatile unsigned	int*)( PHY_DDR_BASE + 0x18 )) = 0x0020051b;
	//*((volatile unsigned	int*)( PHY_DDR_BASE + 0x18 )) = 0x0021f414; //t_dllsrst=0x14,t_dlllock=0x7d0

	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x18 )) = 0x002071ff;//0x002071e8; //t_dllsrst=0x28,t_dlllock=0x1c7
	//write phy PUB_PTR1
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x1c )) = 0x014130b4;
	//write phy PUB_PTR2
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x20 )) = 0x030e10c3;

	//wait phy PGSR;
	loop = 1;
	while(loop)
	{
	 //read phy PUB_PGSR
	 data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x0c ));

	 if((data & 0x1)==0x1)
	 {
		 loop = 0;
	 }
	}
	//read phy PUB_PIR
	data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x04 ));
	//write phy PUB_PIR
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x04 )) = 0x40001;
	//*((volatile unsigned	int*)( PHY_DDR_BASE + 0x04 )) = 0x40003;
	//*((volatile unsigned	int*)( PHY_DDR_BASE + 0x04 )) = 0x00023;	  ///zzzz-2012-4-15 ,ok

	//Wait phy PUB_PGSR;
	loop = 1;
	while(loop)
	{
	 //read phy PUB_PGSR
	 data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x0c ));
	 if((data & 0x7)==0x7)
	 {
		 loop = 0;
	 }
	}
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x14 )) |=(0x1<<30) ;//ACDLLCR
	//------ dqs training --------//
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x04 )) = 0x27; 	 ///zzzz-2012-4-15 ,ok
	//Wait phy PUB_PGSR;
	loop = 1;
	while(loop)
	{
	 //read phy PUB_PGSR
	 data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x0c ));
	 if((data & 0xf)==0xf)
	 {
		 //printf("ddr dram init \n");
		 loop = 0;
	 }
	}


/*
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x04 )) = 0x9;
	//Wait phy PUB_PGSR;
	loop = 1;
	while(loop)
	{
	 //read phy PUB_PGSR
	 data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x0c ));
	 if((data & 0x4)==0x4)
	 {
		 loop = 0;
	 }
	}
*/

	//------ dqs training --------//
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x04 )) = 0x81; 	 ///zzzz-2012-4-15 ,ok
	//Wait phy PUB_PGSR;
	loop = 1;
	while(loop)
	{
	 //read phy PUB_PGSR
	 data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x0c ));
	 if((data & 0x10)==0x10)
	 {
		 loop = 0;
	 }
	}

	//STEP 5:  Monitor DFI init done;
	//Wait ctrl DFISTSTAT0;
	loop = 1;
	while(loop)
	{
	 //read ctrl DFISTSTAT0
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x2c0 ));
	 if((data & 0x1) == 0x1)
	 {
		 loop = 0;
	 }
	}
	//STEP 6:  Set Power-up sequence;
	//write ctrl POWCTL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x44 )) = 0x1;
	//wait ctrl POWSTAT;
	loop = 1;
	while(loop)
	{
	 //read ctrl POWSTAT
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x48 ));
	 if((data & 0x1) == 0x1)
	 {
		 loop = 0;
	 }
	}
	//STEP 7:  Program all timing registers;
	//write ctrl TOGCNT1U
	//	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc0 )) = 0x190;
#if (CONFIG_DDR_BUS_CLK == 195000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc0 )) = 0xc3;	   //195mhz
#elif (CONFIG_DDR_BUS_CLK == 312000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc0 )) = 0x138;      //312mhz
#elif (CONFIG_DDR_BUS_CLK == 351000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc0 )) = 0x15f;	   //351mhz
#else
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc0 )) = 0x186;	   //390mhz
#endif

	//write ctrl TINIT
	//	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc4 )) = 0x1;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc4 )) = 0xc8;		//200us
	//write ctrl TOGCNT100N
	//	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xcc )) = 0x28;
#if (CONFIG_DDR_BUS_CLK == 195000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xcc )) = 0x13;	   //195mhz
#elif (CONFIG_DDR_BUS_CLK == 312000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xcc )) = 0x1f;        //312mhz
#elif (CONFIG_DDR_BUS_CLK == 351000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xcc )) = 0x23;        //351mhz
#else
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xcc )) = 0x27;	    //390mhz
#endif
	//write ctrl TREFI
	// *((volatile unsigned  int*)( CTRL_DDR_BASE + 0xd0 )) = 0x4e;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xd0 )) = 0x27;	   //ref=3.9us
	//write ctrl TMRD
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xd4 )) = 0x5;
	//write ctrl TRFC
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0xd8 )) = 0x34;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xd8 )) = 0x33;
	//write ctrl TRP
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xdc )) = 0x20006;
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0xdc )) = 0x20008;
	//write ctrl TAL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xe4 )) = 0x0;
	//write ctrl TCL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xe8 )) = 0x6;
	//write ctrl TCWL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xec )) = 0x3;
	//write ctrl TRAS
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xf0 )) = 0x11;
	//write ctrl TRC
	// *((volatile unsigned  int*)( CTRL_DDR_BASE + 0xf4 )) = 0x17;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xf4 )) = 0x18;
	//write ctrl TRCD
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0xf8 )) = 0x6;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xf8 )) = 0x8;
	//write ctrl TRRD
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xfc )) = 0x4;
	//write ctrl TRTP
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x100 )) = 0x3;
	//write ctrl TWR
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x104 )) = 0x6;
	//write ctrl TWTR
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x108 )) = 0x3;
	//write ctrl TEXSR
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x10c )) = 0x38;
	//write ctrl TXP
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x110 )) = 0x3;
	//write ctrl TDQS
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x120 )) = 0x2;
	//write ctrl TRTW
	//	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xe0 )) = 0x2;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xe0 )) = 0x3;
	//write ctrl TCKSRE
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x124 )) = 0x0;
	//write ctrl TCKSRX
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x128 )) = 0x0;
	//write ctrl TMOD
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x130 )) = 0x0;
	//write ctrl TCKE
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x12c )) = 0x3;
	//write ctrl TRSTH
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc8 )) = 0x0;
	//write ctrl TRSTL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x134 )) = 0x0;
	//write ctrl TZQCS
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x118 )) = 0x24;
	//write ctrl TZQCL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x138 )) = 0x90;
	//write ctrl TXPDLL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x114 )) = 0x0;
	//write ctrl TZQCSI
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0x11c )) = 0x5;
	//zhxl 20120423 *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x11c )) = 0x3200;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x11c )) = 0x0;//zhxl 20120423

	//write ctrl TCKESR
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x140 )) = 0x6;
	//write ctrl TDPD
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0x144 )) = 0x1;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x144 )) = 0x1f4;
	//write ctrl SCFG
	//V2 zhxl 0613 *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x0 )) = 0x420;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x0 )) = 0x401;//V2 zhxl 0613

		//STEP 8:  Start memory initialization

	//write ctrl MCMD	MR63=0x3f  RESET
#if defined(CONFIG_4GBIT_DDR)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x891003f3;	 //BY ZHENGJR 0528 for 4gb ddr
#elif defined(CONFIG_8GBIT_DDR) || defined(CONFIG_16GBIT_DDR)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x893003f3;
#endif
		//wait MCMD;
	loop = 1;
	while(loop)
	{
	 //read ctrl MCMD
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 ));
	 if((data>>31) == 0)
	 {
		 loop = 0;
	 }
	}

	for(loop=0; loop<300; loop++);	//add delay to satisfy tini5 eco1

#if defined(CONFIG_4GBIT_DDR)
	//write ctrl MCMD  mr10 reset
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x891c30a3;	//zjr calibration mode //BY ZHENGJR 0528 for 4gb ddr
	//wait MCMD;
	loop = 1;
	while(loop)
	{
	 //read ctrl MCMD
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 ));
	 if((data>>31) == 0)
	 {
		 loop = 0;
	 }
	}

	//write ctrl MCMD  mr10 init
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x891ff0a3;	//zjr calibration mode //BY ZHENGJR 0528 for 4gb ddr
#elif defined(CONFIG_8GBIT_DDR) || defined(CONFIG_16GBIT_DDR)
#if defined(CONFIG_ZQ_CALI_INDEP)
	//write ctrl MCMD  mr10 reset rank0
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x891c30a3;	//zjr calibration mode
	//wait MCMD;
	loop = 1;
	while(loop)
	{
	 //read ctrl MCMD
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 ));	
	 if((data>>31) == 0)
	 {
	   loop = 0;
	 }
	}

	//write ctrl MCMD  mr10 reset rank1
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x892c30a3;	//zjr calibration mode
	//wait MCMD;
	loop = 1;
	while(loop)
	{
	 //read ctrl MCMD
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 ));	
	 if((data>>31) == 0)
	 {
	   loop = 0;
	 }
	}

	//write ctrl MCMD  mr10 init rank0
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x891ff0a3;	//zjr calibration mode
	//wait MCMD;
	loop = 1;
	while(loop)
	{
	 //read ctrl MCMD
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 ));	
	 if((data>>31) == 0)
	 {
	   loop = 0;
	 }
	}

	//write ctrl MCMD  mr10 init rank1
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x892ff0a3;	//zjr calibration mode

#else

	//write ctrl MCMD  mr10 reset
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x893c30a3;	//zjr calibration mode
	//wait MCMD;
	loop = 1;
	while(loop)
	{
	 //read ctrl MCMD
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 ));	
	 if((data>>31) == 0)
	 {
	   loop = 0;
	 }
	}

	//write ctrl MCMD  mr10 init
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x893ff0a3;	//zjr calibration mode
#endif
#endif
	//wait MCMD;
	loop = 1;
	while(loop)
	{
	 //read ctrl MCMD
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 ));
	 if((data>>31) == 0)
	 {
		 loop = 0;
	 }
	}

		//write ctrl MCMD  mr1
//	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x80384013; //zjr  BL=16;
#if defined(CONFIG_4GBIT_DDR)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x8018b013; //zjr  BL=8; //BY ZHENGJR 0528 for 4gb ddr
#elif defined(CONFIG_8GBIT_DDR) || defined(CONFIG_16GBIT_DDR)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x8038b013; //zjr  BL=8;
#endif
		//wait MCMD;
	loop = 1;
	while(loop)
	{
	 //read ctrl MCMD
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 ));
	 if((data>>31) == 0)
	 {
		 loop = 0;
	 }
	}
		//write ctrl MCMD  mr2
#if defined(CONFIG_4GBIT_DDR)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x80104023; //RL6/WL3 //BY ZHENGJR 0528 for 4gb ddr
#elif defined(CONFIG_8GBIT_DDR) || defined(CONFIG_16GBIT_DDR)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x80304023; //RL6/WL3
#endif
		//wait MCMD;
	loop = 1;
	while(loop)
	{
	 //read ctrl MCMD
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 ));
	 if((data>>31) == 0)
	 {
		 loop = 0;
	 }
	}

		//write ctrl MCMD  mr3
		//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0x40 )) = 0x80f04033; // 60ohm
		//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0x40 )) = 0x80f03033; // 48ohm
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0x40 )) = 0x80f02033; // 40ohm
#if defined(CONFIG_4GBIT_DDR)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x80101033; //zjr 34.3ohm  //BY ZHENGJR 0528 for 4gb ddr
#elif defined(CONFIG_8GBIT_DDR) || defined(CONFIG_16GBIT_DDR)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x80301033; //zjr 34.3ohm
		//wait MCMD;
	loop = 1;
	while(loop)
	{
	 //read ctrl MCMD  
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 ));	
	 if((data>>31) == 0)
	 {
	   loop = 0;
	 }
	}
		//write ctrl MCMD	mr0
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x80300002;	//REFRESH
#endif
		//wait MCMD;
	loop = 1;
	while(loop)
	{
	 //read ctrl MCMD
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 ));
	 if((data>>31) == 0)
	 {
		 loop = 0;
	 }
	}
#if defined(CONFIG_4GBIT_DDR)
		//write ctrl MCMD	mr0
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x80100002;	//REFRESH //BY ZHENGJR 0528 for 4gb ddr
		//wait MCMD;
	loop = 1;
	while(loop)
	{
	 //read ctrl MCMD
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 ));
	 if((data>>31) == 0)
	 {
		 loop = 0;
	 }
	}
#elif defined(CONFIG_8GBIT_DDR) || defined(CONFIG_16GBIT_DDR)
#endif

	//STEP 9:  Write cfg to SCTL
	//read ctrl STAT
	data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x08 ));
	//write ctrl SCTL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x04 )) = 0x1;
		//wait STAT
	loop = 1;
	while(loop)
	{
	 //read ctrl STAT
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x08 ));
	 if((data & 0x7) == 0x1)
	 {
		 loop = 0;
	 }
	}
	//STEP 10:	Configure uPCTL to refine configuration;
	//write ctrl MCFG  bit5-0=NA
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0x80 )) = 0xf60001; //LPDDR2-S2
	//*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x80 )) = 0xf60041; //BY ZHENGJR S4
		*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x80 )) = 0xe60041; //BY ZHENGJR S4  .bl=8byte
	//write ctrl SCFG
	//V2 zhxl 0613 *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x0 )) = 0x400;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x0 )) = 0x401; //V2 zhxl 0613
	//write ctrl PPCFG
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x84 )) = 0x0;  //needn't care
	//write ctrl DFISTCFG0
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x2c4 )) = 0x7;
	//write ctrl DFISTCFG1
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x2c8 )) = 0x3;
	//write ctrl DFISTCFG2
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x2d8 )) = 0x3;

	//write ctrl DFILPCFG0
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x2f0 )) = 0x78181; //0x76161;
	//write ctrl ECCCFG
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x180 )) = 0x0;
	//write ctrl TOGCNT1U
	// *((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc0 )) = 0x190;
#if (CONFIG_DDR_BUS_CLK == 195000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc0 )) = 0xc3;	//195mhz
#elif (CONFIG_DDR_BUS_CLK == 312000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc0 )) = 0x138;   //312mhz
#elif (CONFIG_DDR_BUS_CLK == 351000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc0 )) = 0x15f;	//351mhz
#else
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc0 )) = 0x187;	//390mhz
#endif
	//write ctrl TINIT
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0xc4 )) = 0x1;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc4 )) = 0xc8;	//200us
	//write ctrl TOGCNT100N
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0xcc )) = 0x28;
#if (CONFIG_DDR_BUS_CLK == 195000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xcc )) = 0x13;	//195mhz
#elif (CONFIG_DDR_BUS_CLK == 312000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xcc )) = 0x1f;    //312mhz
#elif (CONFIG_DDR_BUS_CLK == 351000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xcc )) = 0x23;	//351mhz
#else
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xcc )) = 0x27;	//390mhz
#endif
	//write ctrl TREFI
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0xd0 )) = 0x4e;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xd0 )) = 0x27;	   //ref=3.9us
	//write ctrl TMRD
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xd4 )) = 0x5;
	//write ctrl TRFC
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0xd8 )) = 0x34;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xd8 )) = 0x33;
	//write ctrl TRP
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0xdc )) = 0x20006;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xdc )) = 0x20008;
	//write ctrl TAL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xe4 )) = 0x0;
	//write ctrl TCL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xe8 )) = 0x6;
	//write ctrl TCWL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xec )) = 0x3;
	//write ctrl TRAS
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xf0 )) = 0x11;
	//write ctrl TRC
	// *((volatile unsigned  int*)( CTRL_DDR_BASE + 0xf4 )) = 0x17;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xf4 )) = 0x18;
	//write ctrl TRCD
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0xf8 )) = 0x6;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xf8 )) = 0x8;
	//write ctrl TRRD
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xfc )) = 0x4;
	//write ctrl TRTP
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x100 )) = 0x3;
	//write ctrl TWR
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x104 )) = 0x6;
	//write ctrl TWTR
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x108 )) = 0x3;
	//write ctrl TEXSR
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x10c )) = 0x38;
	//write ctrl TXP
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x110 )) = 0x3;
	//write ctrl TDQS
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x120 )) = 0x2;
	//write ctrl TRTW
	// *((volatile unsigned  int*)( CTRL_DDR_BASE + 0xe0 )) = 0x2;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xe0 )) = 0x3;
	//write ctrl TCKSRE
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x124 )) = 0x0;
	//write ctrl TCKSRX
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x128 )) = 0x0;
	//write ctrl TMOD
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x130 )) = 0x0;
	//write ctrl TCKE
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x12c )) = 0x3;
	//write ctrl TRSTH
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0xc8 )) = 0x0; 	 //0~1024 rst time
	//write ctrl TRSTL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x134 )) = 0x0;
	//write ctrl TZQCS
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x118 )) = 0x24;
	//write ctrl TZQCL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x138 )) = 0x90;
	//write ctrl TXPDLL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x114 )) = 0x0;
	//write ctrl TZQCSI
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0x11c )) = 0x5;
	//zhxl 20120423*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x11c )) = 0x3200;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x11c )) = 0x0;//zhxl 20120423
	//write ctrl TCKESR
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x140 )) = 0x6;
	//write ctrl TDPD
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0x144 )) = 0x1;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x144 )) = 0x1f4;
	//write ctrl DFITPHYWRLAT
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x254 )) = 0x3;
	//write ctrl DFITPDDATAEN
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x260 )) = 0x5;
	//write ctrl FITPHYWRDATA
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x250 )) = 0x1;
	//write ctrl DFITPHYRDLAT
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x264 )) = 0xf;
	//write ctrl ITDRAMCLKDIS
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x2d4 )) = 0x2;
	//write ctrl FITDRAMCLKEN
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x2d0 )) = 0x2;
	//write ctrl FITCTRLDELAY
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x240 )) = 0x2;
	//write ctrl MCMD	mr mode select
#if defined(CONFIG_4GBIT_DDR)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x80100003;	//BY ZHENGJR 0528 for 4gb ddr
#elif defined(CONFIG_8GBIT_DDR) || defined(CONFIG_16GBIT_DDR)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x80300003;
#endif
		//wait MCMD
	loop = 1;
	while(loop)
	{
	 //read ctrl MCMD
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 ));
	 if((data>>31) == 0)
	 {
		 loop = 0;
	 }
	}
	//write ctrl MCMD  ddr2  mr1
	//*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x80384013;	//BL=16
#if defined(CONFIG_4GBIT_DDR)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x8018b013;	//BL=8 //BY ZHENGJR 0528 for 4gb ddr
#elif defined(CONFIG_8GBIT_DDR) || defined(CONFIG_16GBIT_DDR)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 )) = 0x8038b013;	//BL=8
#endif
		//wait MCMD
	loop = 1;
	while(loop)
	{
	 //read ctrl MCMD
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40 ));
	 if((data>>31) == 0)
	 {
		 loop = 0;
	 }
	}

	//write phy PUB_MR1    DBB MR1
	//*((volatile unsigned  int*)( PHY_DDR_BASE + 0x44 )) = 0x084;  //BL=16
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x44 )) = 0x08b;  //BL=8
	//adr_map={cs,bank,row,col};14bit row,10 col, 4gb/cs,
	//write ctrl DCFG
#if defined(CONFIG_16GBIT_DDR)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x484 )) = 0x15a;
#else
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x484 )) = 0x15b;
#endif
	//write ctrl CCFG
	//data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x480 ));  //memory page disable
	//*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x480 )) = data & 0xffffffef; //0xffffffe7;
	//write PCFG_0
	//write ctrl CCFG
	data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x480 ));  //memory page disable
	data |=  (0xff<<6) | (0xff<<16) | (0x3<<26);
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x480 )) = data;

	//write ctrl CCFG1
#if (CONFIG_DDR_BUS_CLK==312000000)
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x48c )) = 0x3312024; //312mhz
#else
	/* Default 390MHz. */
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x48c )) = 0x3319032;	//390mhz
#endif
	//write PCFG_0
	//zhxl 0616*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x400 )) = 0x0;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x400 )) = 0x200430;
	//write PCFG_1
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x404 )) = 0x30; //0x0;
	//write PCFG_2
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x408 )) = 0x31;
	//write PCFG_3
#if CONFIG_DDR_AXI_M3_OPTIMIZATION
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40c )) = 0x410;
#else
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40c )) = 0x430;
#endif
	//*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x40c )) = 0x0;//zhxl 20120506 bp_rd_en = 0,bp_wr_en = 0

	////write phy DX3DQTR
	//*((volatile unsigned	int*)( PHY_DDR_BASE + 0x290 )) = 0xaaaaaaaa;  //DL0
	*((volatile unsigned	int*)( PHY_DDR_BASE + 0x290 )) = 0xfffffffa;  //DL0 zhxl 20120423

	//write phy DX0DQSTR
	data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x1d4 ));
	data = (data & (~(0x3f<<20)));
	data2 = (0x12<<20);
	data = (data | data2);
	//*((volatile unsigned  int*)( PHY_DDR_BASE + 0x1d4 )) = data;

	//write phy DX1DQSTR
	data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x214 ));
	data = (data & (~(0x3f<<20)));
	data2 = (0x12<<20);
	data = (data | data2);
	//*((volatile unsigned  int*)( PHY_DDR_BASE + 0x214 )) = data;

	//write phy DX2DQSTR
	data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x254 ));
	data = (data & (~(0x3f<<20)));
	data2 = (0x9<<20);
	data = (data | data2);
	//*((volatile unsigned  int*)( PHY_DDR_BASE + 0x254 )) = data;


	//write phy DX3DQSTR
	data = *((volatile unsigned  int*)( PHY_DDR_BASE + 0x294 ));
	data = (data & (~(0x3f<<20)));
	data2 = (0x9<<20);
	data = (data | data2);
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x294 )) = data;

	//*((volatile unsigned	int*)( PHY_DDR_BASE + 0x294 )) |= (0x3f<<20);  //DL0
	//write SCFG
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0x00 )) = 0x401;   // enable power down
	//write MCFG1
	//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0x7c )) = 0x80200000;
	//read ctrl STAT
	data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x08 ));
	//write ctrl SCTL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x04 )) = 0x2;
	//wait STAT
	loop = 1;
	while(loop)
	{
	 //read ctrl STAT
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x08 ));
	 if((data & 0x7) == 0x3)
	 {
		 loop = 0;
	 }
	}
	//End LPDDR2 Configuration
	//printf("end ddr init \n");

	data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x08 ));
	//write ctrl SCTL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x04 )) = 0x1;
		//wait STAT
	loop = 1;
	while(loop)
	{
	 //read ctrl STAT
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x08 ));
	 if((data & 0x7) == 0x1)
	 {
		 loop = 0;
	 }
	}
	//zhxl 20120423 *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x298 )) = 0x32000;
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x298 )) = 0x0; //zhxl 20120423

	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x290 )) = 0x0;

	////BY ZHENGJR 0528 for low power enable
	//if(LOW_POWER)
	//{
		//write SCFG
		//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0x00 )) = 0x401;   // enable power down
		//write MCFG1
		//*((volatile unsigned	int*)( CTRL_DDR_BASE + 0x7c )) = 0x80000080;
	//}

	data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x08 ));
	//write ctrl SCTL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x04 )) = 0x2;
	//wait STAT
	loop = 1;
	while(loop)
	{
	 //read ctrl STAT
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x08 ));
	 if((data & 0x7) == 0x3)
	 {
		 loop = 0;
	 }
	}

	//zhxl 20120423 mdelay(15);
	mdelay(300);//zhxl 20120423

	//zhxl add 20120425
	//    #define A9_CLK_REG_TMP		0xA010A844
	//    #define A9_CLK_REG_VALUE_TMP	0x10004  //zhxl 20120425 0x10008=>0x10004
	//    __raw_writel(A9_CLK_REG_VALUE_TMP, A9_CLK_REG_TMP);
	//zhxl  end

	// the following is for test on 20120424
	// zhxl 201204124*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x11c )) = 0x0;
	// zhxl 201204124*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x298 )) = 0x0; //zhxl new 20120424
	//zhengjr 20120424 ddr enter config state
	data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x08 ));
	//write ctrl SCTL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x04 )) = 0x1;
		//wait STAT
	loop = 1;
	while(loop)
	{
	 //read ctrl STAT
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x08 ));
	 if((data & 0x7) == 0x1)
	 {
		 loop = 0;
	 }
	}
	/*
	//zhxl 20120423 set zqcsi disable
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x11c )) = 0x0;
	bist_train();

	//zhxl 20120504 begin
	//add test output
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x08 )) |= (0x9<<5);
	*((volatile unsigned  int*)( PHY_DDR_BASE + 0x10 )) |= (0xf<<5);
	//while(1){

	//}
	while(1);
	//zhxl 20120504 end
	*/

	//bist_train();
	//write SCFG
	*((volatile unsigned	int*)( CTRL_DDR_BASE + 0x00 )) = 0x401;   // enable power down
	//write MCFG1
	*((volatile unsigned	int*)( CTRL_DDR_BASE + 0x7c )) = 0x800000ff;//0x80ff0000;	//for eco1

	//mdelay(100);
	//zhengjr 20120424 ddr enter access state
	data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x08 ));
	//write ctrl SCTL
	*((volatile unsigned  int*)( CTRL_DDR_BASE + 0x04 )) = 0x2;
	//wait STAT
	loop = 1;
	while(loop)
	{
	 //read ctrl STAT
	 data = *((volatile unsigned  int*)( CTRL_DDR_BASE + 0x08 ));
	 if((data & 0x7) == 0x3)
	 {
		 loop = 0;
	 }
	}

	printf("init end\n");
}

static void memctl_gpv_config(void)
{
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x2008)) = 0x02;

#if CONFIG_DDR_AXI_M3_OPTIMIZATION
	//ISP
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x47100)) = 0x00;
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x47104)) = 0x00;
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x47108)) = 0x02;

	//on2 dec
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x43100)) = 0x00;		//READ QOS
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x43104)) = 0x00;		//WRITE QOS
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x43108)) = 0x00;		//GPVFN_MOD
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x4310c)) = 1<<6;		//QOS_CNTL
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x43110)) = 3<<24;    //MAX_OT

	//on2 enc0
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x44100)) = 0x00;		//READ QOS
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x44104)) = 0x00;		//WRITE QOS
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x44108)) = 0x00;		//GPVFN_MOD
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x4410c)) = 1<<6;		//QOS_CNTL
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x44110)) = 3<<24;    //MAX_OT
	
	//on2 enc1
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x45100)) = 0x00;		//READ QOS
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x45104)) = 0x00;		//WRITE QOS
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x45108)) = 0x00;		//GPVFN_MOD
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x4510c)) = 1<<6;		//QOS_CNTL
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x45110)) = 3<<24;    //MAX_OT

	//gpu
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x46100)) = 0x00;		//READ QOS
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x46104)) = 0x00;		//WRITE QOS
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x46108)) = 0x00;		//GPVFN_MOD
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x4610c)) = 1<<6;		//QOS_CNTL
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x46110)) = 2<<24;    //MAX_OT

	//DMA AXI
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x48100)) = 0x00;		//READ QOS
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x48104)) = 0x00;		//WRITE QOS
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x48108)) = 0x00;		//GPVFN_MOD
#else
	//ISP
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x47100)) = 0x00;
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x47104)) = 0x00;
	*((volatile unsigned int*)(DDR_AXI_GPV_BASE + 0x47108)) = 0x02;
#endif	
}

int memctl_init(void)
{
	lpddr2_400MHz_config();
	memctl_gpv_config();

	return 0;
}

