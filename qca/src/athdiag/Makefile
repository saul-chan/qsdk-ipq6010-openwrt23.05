
CC := $(TOOLPREFIX)gcc
TARGET_TYPE ?= AR900B
TARGET_VERS ?= v1

ifeq ($(TARGET_TYPE), AR900B)
CFLAGS += -DAR900B -DWLAN_HEADERS
endif

ifeq ($(CONFIG_PACKAGE_kmod-ath11k),)
INCS = -I include/
OBJS_c = athdiag_lite.c
else ifeq ($(WIFI_TARGET_2_0),1)
INCS = \
	-I include/ \
	-I $(DL_DIR)/include/$(TARGET_TYPE)/$(TARGET_VERS)/ \
	-I $(DL_DIR)/include/$(TARGET_TYPE)/extra/ \
	-I $(DL_DIR)/include/$(TARGET_TYPE)/hw/ \
	-I $(DL_DIR)/include/$(TARGET_TYPE)/extra/hw/ \
	-I $(DL_DIR)/include/

OBJS_c = athdiag.c Qc98xxEepromStructGet.c Qc98xxDevice.c Qc98xxEepromStruct.c
else
INCS = -I include/
OBJS_c = athdiag_lite.c
endif

ifeq ($(CONFIG_BUILD_YOCTO),y)
CFLAGS += -I$(STAGING_INCDIR)
CFLAGS += -I$(STAGING_LIBDIR)
CFLAGS += -I$(STAGING_INCDIR)/glib-2.0
CFLAGS += -I$(STAGING_LIBDIR)/glib-2.0/include
CFLAGS +="--sysroot=${STAGING_DIR_TARGET}"
ifeq ($(ARCH), arm)
CFLAGS += -march=armv7-a -mfloat-abi=hard -mfpu=neon -include glib.h -Dstrlcat=g_strlcat -Dstrlcpy=g_strlcpy
else
CFLAGS += -include glib.h -Dstrlcat=g_strlcat -Dstrlcpy=g_strlcpy
endif
LDFLAGS += -lglib-2.0  -lc -lgcc
LDFLAGS += -L$(STAGING_LIBDIR) -L${STAGING_DIR_TARGET}/lib
endif

CFLAGS += -fpie
LDFLAGS += -pie

all:
	$(CC) -g3 -Wall  -Werror\
		$(CFLAGS) \
		$(LDFLAGS) \
		$(INCS) \
		$(OBJS_c) -o athdiag


clean:
	rm -f athdiag
