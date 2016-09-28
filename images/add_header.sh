#!/bin/bash

ahstring="CONFIG_COMIP_NAND=y"
ahfile=../include/autoconf.mk
ahstring2="CONFIG_CPU_LC1860=y"

ahline=`grep -n $ahstring $ahfile`
ahline2=`grep -n $ahstring2 $ahfile`

if [ "$ahline" = "" ];then
echo "add emmc header of bootloader for bootrom"
if [ "$ahline2" = "" ];then
./add_emmc_header.sh
else
./add_emmc_header_1860.sh
fi
else
echo "add nand header of bootloader for bootrom"
./add_nand_header.sh
fi
