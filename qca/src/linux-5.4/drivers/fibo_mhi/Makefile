ccflags-y += -g -Wno-incompatible-pointer-types -Wno-unused-variable -Wformat=0 -Wno-unused-function
#ccflags-y += -DCONFIG_MHI_NETDEV_MBIM
#obj-${CONFIG_PCIE_MHI} := fibo_mhi.o
obj-m := fibo_mhi.o
fibo_mhi-objs := core/mhi_init.o core/mhi_main.o core/mhi_pm.o core/mhi_boot.o core/mhi_dtr.o devices/mhi_netdev.o devices/mhi_uci.o controllers/mhi_qcom.o

PWD := $(shell pwd)
ifeq ($(ARCH),)
ARCH := $(shell uname -m)
endif
ifeq ($(CROSS_COMPILE),)
CROSS_COMPILE :=
endif
ifeq ($(KDIR),)
KDIR := /lib/modules/$(shell uname -r)/build
endif

fibo_mhi: clean
	$(MAKE) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -C $(KDIR) M=$(PWD) clean
