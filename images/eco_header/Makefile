CC=arm-eabi-gcc
LD=arm-eabi-ld
OBJCOPY=arm-eabi-objcopy

CFLAGS   =  -O2 -g
ASFLAGS =  -O2 -g

START_LDFLAGS =  -Tstart.lds  -Ttext  0xE0040000
SECOND_LDFLAGS =  -Tsecond.lds  -Ttext  0xA0800000

START_OBJS = start.o
SECOND_OBJS = second.o

.c.o:
	$(CC)	$(CFLAGS)	-c	 $<
.s.o:
	$(CC)	$(ASFLAGS)	-c	 $<

all:first_boot_bin second_boot_bin

first_boot:$(START_OBJS)
	$(CC)	-static  -nostartfiles -nostdlib $(START_LDFLAGS) $? -o $@

first_boot_bin:first_boot
	$(OBJCOPY) -O binary first_boot first_boot_bin

second_boot:$(SECOND_OBJS)
	$(CC)	-static  -nostartfiles -nostdlib $(SECOND_LDFLAGS) $? -o $@

second_boot_bin:second_boot
	$(OBJCOPY) -O binary second_boot second_boot_bin

clean:
	rm *.o first_boot  first_boot_bin second_boot second_boot_bin  -f
