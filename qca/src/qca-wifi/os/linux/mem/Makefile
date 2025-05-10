ifeq ($(obj),)
obj := $(shell pwd)
endif

CMNDEV_PATH := $(obj)/../../..
DEPTH := $(obj)/../../..
HOST_CMN_CONVG_SRC := $(CMNDEV_PATH)/cmn_dev

ifeq ($(CNSS2_CONFIG_MODULE), 1)
EXTRA_CFLAGS += -DCNSS2_CONFIG_MODULE=1
endif

KBUILD_CFLAGS += -I${KERNELPATH}/arch/arm/include/
KBUILD_CFLAGS += -I${KERNELPATH}/arch/arm/include/asm/
KBUILD_CFLAGS += -I$(DEPTH)/include/
KBUILD_CFLAGS += -I$(DEPTH)/pld/inc/
KBUILD_CFLAGS += -I$(DEPTH)/qca_ol/
KBUILD_CFLAGS += -I$(DEPTH)/component_dev/qal/inc
KBUILD_CFLAGS += -I$(DEPTH)/offload/os/linux/include/
KBUILD_CFLAGS += -I$(DEPTH)/offload/include/
KBUILD_CFLAGS += -I$(HOST_CMN_CONVG_SRC)/qdf/linux/src
KBUILD_CFLAGS += -I$(HOST_CMN_CONVG_SRC)/qdf/inc/
KBUILD_CFLAGS += -I$(HOST_CMN_CONVG_SRC)/utils/sys/
KBUILD_CFLAGS += -I$(HOST_CMN_CONVG_SRC)/fw_hdr/fw/
KBUILD_CFLAGS += -I$(HOST_CMN_CONVG_SRC)/qal/inc
KBUILD_CFLAGS += -I$(HOST_CMN_CONVG_SRC)/qal/linux/src
obj-m += mem_manager.o
