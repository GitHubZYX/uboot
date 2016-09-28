cd images
./add_emmc_header.sh u-boot.bin
./tobyte.py u-boot.bin a9_secram.dat_bootloader
scp a9_secram.dat_bootloader veloce@172.20.2.98:/home/veloce/lc1810/user0
rm a9_secram.dat_bootloader 

