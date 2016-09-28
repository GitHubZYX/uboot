#!/bin/bash
output=u-boot.bin
rm  $output

if [ ! -e first_boot_bin ] ; then
	echo "first_boot.bin no exist!"
	echo "exit"
	exit -
fi

if [ ! -e second_boot_bin ] ; then
	echo "second_boot.bin no exist!"
	echo "exit"
	exit -2
fi


if [ ! -e ../u-boot.bin ] ; then
	echo "u-boot.bin no exist"
	echo "exit"
	exit -3
fi


let first_boot_len=`wc -c < first_boot_bin`
echo "first_boot_len=$first_boot_len"
let  "first_pad_count=128-($first_boot_len/4)"
echo "first_pad_count=$first_pad_count"



let second_boot_len=`wc -c < second_boot_bin`
echo "second_boot_len=$second_boot_len"
let  "second_pad_count=128 -($second_boot_len/4)"
echo "second_pad_count=$second_pad_count"


let u_boot_len=`wc -c < ../u-boot.bin`
let  "u_boot_len=$u_boot_len + 1024"
#echo file length:$header
header=`printf '0x%08x' $u_boot_len`
echo  file length:$header;


header1=`expr substr $header 9 2`
header2=`expr substr $header 7 2`
header3=`expr substr $header 5 2`
header4=`expr substr $header 3 2`


echo -n -e "\x${header1}" > $output
echo -n -e "\x${header2}" >> $output
echo -n -e "\x${header3}" >> $output
echo -n -e "\x${header4}" >> $output

#byte4-byte35 bootloader SHA-AES code
for((i=1;i<=508;i++))
do
	echo -n -e "\x00" >> $output
done



cat  first_boot_bin>> $output
for((i=0;i<first_pad_count;i++))
do
	echo -n -e "\x00\xF0\x20\xE3" >> $output
done

cat  second_boot_bin>> $output
for((i=0;i<second_pad_count;i++))
do
	echo -n -e "\x00\xF0\x20\xE3" >> $output
done

cat ../$output >> $output
echo convert complete.
