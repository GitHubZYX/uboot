# Copyright (c) 2010 Wind River Systems, Inc.
#
# The right to copy, distribute, modify, or otherwise make use
# of this software may be licensed only pursuant to the terms
# of an applicable Wind River license agreement.

ifeq ($(TARGET_BOOT_DIR),bootable/bootloader/uboot)
LOCAL_PATH := $(call my-dir)
uboot_MAKE_CMD := make -C $(LOCAL_PATH) CROSS_COMPILE=$(shell pwd)/$(CROSS_COMPILE)
uboot_MAKE_IMG := cd $(LOCAL_PATH)/images;./add_header.sh

uboot_IMG := $(LOCAL_PATH)/images/u-boot.bin
uboot_COPY_IMG := cp -pf $(uboot_IMG) $(PRODUCT_OUT)/u-boot.bin

ifeq ($(TARGET_BOOTLOADER_BOARD_NAME),evb2)
uboot_COPY_IMG_1 := cp -pf $(uboot_IMG) $(PRODUCT_OUT)/u-boot_512m.bin
endif

$(LOCAL_PATH)/images/u-boot.bin: u-boot.bin

.PHONY: u-boot.bin
u-boot.bin:
	$(hide) echo "lc181x uboot $(TARGET_UBOOT_CONFIG) config..."
	$(hide) $(uboot_MAKE_CMD) $(TARGET_UBOOT_CONFIG)
	$(hide) $(uboot_MAKE_CMD) 
	$(hide) $(uboot_MAKE_IMG)
	$(hide) $(uboot_COPY_IMG)
ifeq ($(TARGET_BOOTLOADER_BOARD_NAME),evb2)
	$(hide) $(uboot_MAKE_CMD) comip_lc1860_evb2_4x4_config
	$(hide) $(uboot_MAKE_CMD)
	$(hide) $(uboot_MAKE_IMG)
	$(hide) $(uboot_COPY_IMG_1)
endif

.PHONY: u-boot.clean
u-boot.clean:
	$(hide) echo "lc181x uboot clean..."
	$(hide) $(uboot_MAKE_CMD) distclean

include $(CLEAR_VARS)
LOCAL_MODULE       := u-boot 
LOCAL_MODULE_TAGS  := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := images/u-boot.bin 
LOCAL_MODULE_PATH  := $(PRODUCT_OUT)
include $(BUILD_PREBUILT)
endif


ifeq ($(BOOT_DIR),bootable/bootloader/uboot)
LOCAL_PATH := $(call my-dir)
uboot_MAKE_CMD := make -C $(LOCAL_PATH) CROSS_COMPILE=$(shell pwd)/$(CROSS_COMPILE)
uboot_MAKE_IMG := cd $(LOCAL_PATH)/images;./add_header.sh

uboot_IMG := $(LOCAL_PATH)/images/u-boot.bin
uboot_COPY_IMG := cp -pf $(uboot_IMG) $(PRODUCT_OUT)/u-boot.bin

.PHONY: u-boot.bin
u-boot.bin:
	$(hide) echo "lc181x uboot for android 4.0.3 $(UBOOT_CONFIG) config..."
	$(hide) $(uboot_MAKE_CMD) $(UBOOT_CONFIG)
	$(hide) $(uboot_MAKE_CMD)
	$(hide) $(uboot_MAKE_IMG)
	$(hide) $(uboot_COPY_IMG)

.PHONY: u-boot.clean
u-boot.clean:
	$(hide) echo "lc181x uboot for android 4.0.3 clean..."
	$(hide) $(uboot_MAKE_CMD) distclean
endif

