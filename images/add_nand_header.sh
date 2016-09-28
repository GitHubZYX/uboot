#!/bin/bash
output=u-boot.bin
rm  $output

if [ ! -e ../u-boot.bin ] ; then
	echo "compile error!"
	exit -1
fi

#byte0-byte3 bootloader size in byte(max 112K byte)
let header=`wc -c < ../u-boot.bin`
#echo file length:$header
header=`printf '0x%08x' $header`
echo  file length:$header;
header1=`expr substr $header 9 2`
header2=`expr substr $header 7 2`
header3=`expr substr $header 5 2`
header4=`expr substr $header 3 2`

echo -n -e "\x${header1}" > $output
echo -n -e "\x${header2}" >> $output
echo -n -e "\x${header3}" >> $output
echo -n -e "\x${header4}" >> $output

#byte4-byte103 reserved
for((i=4;i<104;i++))
do
	echo -n -e "\x00" >> $output
done

#now add u-boot.bin 
cat ../$output >> $output
echo convert complete.
