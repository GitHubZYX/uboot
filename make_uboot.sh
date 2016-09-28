#!/bin/bash

PHONE_BOARD_VERSION=`echo "$1"|tr "a-z" "A-Z"`
RSA_ON=`echo "$2"|tr "a-z" "A-Z"`

DIR=$(cd $(dirname "$0"); pwd)
CROSS_COMPILE_DIR=${DIR/bootable\/bootloader\/uboot/prebuilts\/gcc\/linux-x86\/arm\/arm-eabi-4.6\/bin\/};
export PATH="$CROSS_COMPILE_DIR:$PATH"

if [ "$PHONE_BOARD_VERSION" = "YL8150S_EVB" ] ; then
	echo "Select YL8150S EVB board"
	make distclean; make comip_yl8150s_evb_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "LC1813_EVB2" ] ; then
	echo "Select LC1813 EVB2 board"
	make distclean; make comip_lc1813_evb2_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "UST802_V1_1" ] ; then
	echo "Select UST802 phone board verison is v1.1"
	make distclean; make comip_ust802_v1_1_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "LC1860_MENTOR" ] ; then
	echo "Select LC1860 mentor"
	make distclean; make comip_lc1860_mentor_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "TL920_V1_0" ] ; then
	echo "Select TL920 phone board verison is v1.0"
	make distclean; make comip_tl920_v1_0_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "LC1813_TL" ] ; then
	echo "Select LC1813 Target Loader project"
	make distclean; make comip_lc1813_tl_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "LC1860_TL" ] ; then
	echo "Select LC1860 Target Loader project"
	make distclean; make comip_lc1860_tl_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "LC1860_EVB" ] ; then
	echo "Select LC1860 EVB board"
	make distclean; make comip_lc1860_evb_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "LC1860_EVB2" ] ; then
	echo "Select LC1860 EVB2 board"
	make distclean; make comip_lc1860_evb2_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "LC1860_EVB2_4" ] ; then
	echo "Select LC1860 EVB2_4x4 board"
	make distclean; make comip_lc1860_evb2_4x4_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "USL90_V1_0" ] ; then
	echo "Select USL90 phone board verison is v1.0"
	make distclean; make comip_usl90_v1_0_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "LTE26007_V1_0" ] ; then
	echo "Select LTE26007 phone board verison is v1.0"
	make distclean; make comip_lte26007_v1_0_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "LTE26007_V1_1" ] ; then
	echo "Select LTE26007 phone board verison is v1.1"
	make distclean; make comip_lte26007_v1_1_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "A310T_V1_0" ] ; then
	echo "Select A310T phone board verison is v1.0"
	make distclean; make comip_a310t_v1_0_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "A310T_V1_1" ] ; then
	echo "Select A310T phone board verison is v1.1"
	make distclean; make comip_a310t_v1_1_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "LC1860_PHONE" ] ; then
	echo "Select LC1860 phone board verison"
	make distclean; make comip_lc1860_phone_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "M9206_V1_0" ] ; then
	echo "Select m9206 phone board verison is v1.0"
	make distclean; make comip_m9206_v1_0_config; make -s	
elif [ "$PHONE_BOARD_VERSION" = "LX70A_V1_0" ] ; then
	echo "Select lx70a phone board verison is v1.0"
	make distclean; make comip_lx70a_v1_0_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "K706_V1_0" ] ; then
	echo "Select K706 phone board verison is v1.0"
	make distclean; make comip_k706_v1_0_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "YONGDA_V1_0" ] ; then
	echo "Select YONGDA phone board verison is v1.0"
	make distclean; make comip_yongda_v1_0_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "SMARTMODULE_V1_0" ] ; then
	echo "Select smartmodule phone board verison is v1.0"
	make distclean; make comip_smartmodule_v1_0_config; make -s		
elif [ "$PHONE_BOARD_VERSION" = "UNICOMPADEVB3" ] ; then
	echo "Select smartmodule phone board verison is v1.0"
	make distclean; make comip_lc1860_unicompadlc1860evb3_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "FOURMODE" ] ; then
	echo "Select fourmode board verison is v1.0"
	make distclean; make comip_lc1860_fourmode_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "FIVEMODE" ] ; then
	echo "Select fivemode board verison is v1.0"
	make distclean; make comip_lc1860_fivemode_config; make -s
elif [ "$PHONE_BOARD_VERSION" = "LC1860_ZEUS" ] ; then
	echo "Select LC1860 ZEUS board"
	make distclean; make comip_lc1860_zeus_config; make -s
else
	echo "Error message:please selct phone version between
	YL8150S_EVB,
	LC1813_TL, LC1813_EVB2, UST802_V1_1, TL920_V1_0, LC1860_TL, LC1860_MENTOR, LC1860_EVB, LC1860_EVB2, LC1860_EVB2_4, USL90_V1_0, LTE26007_V1_0, A310T_V1_0, A310T_V1_1, LTE26007_V1_1, LC1860_PHONE, M9206_V1_0, LX70A_V1_0,K706_V1_0,YONGDA_V1_0, SMARTMODULE_V1_0, UNICOMPADEVB3, FOURMODE,FIVEMODE,LC1860_ZEUS"
exit 0
fi

if [ "$PHONE_BOARD_VERSION" = "LC1813_TL" -o "$PHONE_BOARD_VERSION" = "LC1860_TL" ]; then
	BOARD_SEG=${PHONE_BOARD_VERSION%%_*}
	DISK_SEG=`grep "CONFIG_TL_DISK" include/autoconf.mk`
	DISK_SEG=${DISK_SEG#*\"}
	DISK_SEG=${DISK_SEG%\"*}
	TRANS_SEG=`grep "CONFIG_TL_TRANS" include/autoconf.mk`
	TRANS_SEG=${TRANS_SEG#*\"}
	TRANS_SEG=${TRANS_SEG%\"*}
	DDR_SEG=`grep "CONFIG_TL_NODDR" include/autoconf.mk`
	DDR_SEG=${DDR_SEG#*\"}
	DDR_SEG=${DDR_SEG%\"*}
	AXF_NAME=${BOARD_SEG}${DISK_SEG}${TRANS_SEG}${DDR_SEG}.axf
	rm images/*.axf
	cp u-boot.bin images/${AXF_NAME}
else
	cd images;
	if [ "$RSA_ON" = "RSA" ] ; then
		echo "use RSA sign"
		./add_emmc_header_1860_eco_RSA.sh
		echo "sign ok"
	else
		./add_header.sh
	fi
	cd -
fi

