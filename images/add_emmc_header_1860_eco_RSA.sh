#!/bin/bash
output=u-boot.bin
rm  $output

if [ ! -e ../u-boot.bin ] ; then
	echo "compile error!"
	exit -1
fi

cp ../u-boot.bin .

python sign.py signkey.pub signkey.priv u-boot.bin

echo convert complete.
