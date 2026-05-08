# If use the arm gcc, please set the arm gcc path.
# ARM_GCC = ~/ARM_Linux_GCC/bin/arm-none-linux-gnueabi-

CC = $(ARM_GCC)gcc
CXX = $(ARM_GCC)g++
AR = $(ARM_GCC)ar
CFLAGS = -Wall

#If the customer only wants to compile a tool for a specific platform,
#set the macro for the corresponding platform to yes, and the macro for other platforms to no,
#the tool for a specific platform will be compiled separately.
config_unisoc_x = yes
config_unisoc = yes
config_qcom = yes
config_zte = yes
config_eigencomm = yes
config_sl8563_x = no
config_qcom_localdump = yes
config_samsung = no

.DEFAULT_GOAL := all

ifeq ($(config_unisoc_x), yes)
CFLAGS += -DCONFIG_UNISOC_X
endif

ifeq ($(config_sl8563_x), yes)
CFLAGS += -DCONFIG_SL8563_X
endif

ifeq ($(config_unisoc), yes)
CFLAGS += -DCONFIG_UNISOC
endif

ifeq ($(config_qcom), yes)
CFLAGS += -DCONFIG_QCOM
endif

ifeq ($(config_zte), yes)
CFLAGS += -DCONFIG_ZTE
endif

ifeq ($(config_eigencomm), yes)
CFLAGS += -DCONFIG_EIGENCOMM
endif

ifeq ($(config_qcom_localdump), yes)
CFLAGS += -DCONFIG_QCOM_DUMP
endif


ifeq ($(config_unisoc_x), yes)
LD_LIB = -lpthread -lxlog
else
LD_LIB = -lpthread
endif

ifeq ($(config_qcom_localdump), yes)
LD_LIB += -lz
endif

ifeq ($(config_sl8563_x), yes)
LD_LIB += -lxlog8563
endif

ifeq ($(config_samsung), yes)
LD_LIB = -lpthread -lrt -ldl
CFLAGS += -DCONFIG_SAMSUNG
SILENTLOGGING_CFLAGS = -DCONFIG_SAMSUNG -Wall -g
LOGCAT_CFLAGS = -DCONFIG_SAMSUNG -O2 -g -DNDEBUG -std=gnu99 -Wall -Wextra -Wno-variadic-macros -Wno-strict-aliasing
LOGCAT_CFLAGS += -DCONFIGURATION_FILES_DIR=\"/usr/local/etc\" -DDLT_DAEMON_USE_FIFO_IPC -DDLT_LIB_USE_FIFO_IPC -DDLT_NETWORK_TRACE_ENABLE -DDLT_USER_IPC_PATH=\"/tmp\" -D_GNU_SOURCE
endif

ifeq ($(config_samsung_localdump), yes)
CFLAGS += -DCONFIG_SAMSUNG_DUMP
endif
INCLUDE = -I./misc_code -I./tftp_code -I./zte_code -I./qcom_code -I./unisoc_code -I./zte_code/config -I./qcom_code/config
INCLUDE += -I./unisoc_x_code/xlog/inc -I./unisoc_x_code/xlog/lib/inc -I./sl8563_x_code/xlog/sour
LIB_PATH = -L./unisoc_x_code/xlog/lib -L./sl8563_x_code/xlog/lib
SRC_FILES = $(wildcard ./misc_code/*.c ./*.c)
SRC_FILES += $(wildcard ./tftp_code/*.c)

ifeq ($(config_unisoc_x), yes)
SRC_FILES += $(wildcard ./unisoc_x_code/*.c)
endif

ifeq ($(config_sl8563_x), yes)
SRC_FILES += $(wildcard ./sl8563_x_code/*.c)
endif

ifeq ($(config_unisoc), yes)
SRC_FILES += $(wildcard ./unisoc_code/*.c)
endif

ifeq ($(config_qcom), yes)
SRC_FILES += $(wildcard ./qcom_code/*.c)
endif

ifeq ($(config_zte), yes)
SRC_FILES += $(wildcard ./zte_code/*.c)
endif

ifeq ($(config_eigencomm), yes)
SRC_FILES += $(wildcard ./eigencomm_code/*.c)
endif

ifeq ($(config_samsung), yes)
INCLUDE += -I./samsung_code/samsunglogtool -I./samsung_code/silentlogging -I./samsung_code/dlt-logcat/include/dlt -I./samsung_code/dlt-logcat/include_from_cmake/dlt -I./samsung_code/dlt-logcat/src/shared
SRC_FILES += $(wildcard ./samsung_code/samsunglogtool/*.c)
LOGCAT_SRC_FILES = $(wildcard ./samsung_code/dlt-logcat/src/console/*.c   ./samsung_code/dlt-logcat/src/lib/*.c )
LOGCAT_SRC_FILES += ./samsung_code/dlt-logcat/src/shared/dlt_common.c ./samsung_code/dlt-logcat/src/shared/dlt_protocol.c ./samsung_code/dlt-logcat/src/shared/dlt_user_shared.c
LOGCAT_SRC_FILES += ./samsung_code/dlt-logcat/src/shared/network_api.c
SILENTLOGGING_SRC_FILES = $(wildcard ./samsung_code/silentlogging/*.c)
endif

SUBDIRS := ./unisoc_x_code/xlog
SUBDIRS2 := ./sl8563_x_code/xlog

BIN_TARGET = logtool
LOGCAT_TARGET = dlt-logcat
SILENTLOGGING_TARGET = silentlogging

OBJECTS = $(patsubst %.c,%.o,$(SRC_FILES))
LOGCAT_OBJECTS = $(patsubst %.c,%.o,$(LOGCAT_SRC_FILES))
SILENTLOGGING_OBJECTS = $(SILENTLOGGING_SRC_FILES:.c=.o)

$(BIN_TARGET) : $(OBJECTS)
ifeq ($(config_unisoc_x), yes)
	make -C $(SUBDIRS) CC=$(CC) CXX=$(CXX) AR=$(AR)
endif
ifeq ($(config_sl8563_x), yes)
	make -C $(SUBDIRS2) CC=$(CC) CXX=$(CXX) AR=$(AR)
endif
	$(CXX) $^ -o $@ $(LD_LIB) $(LIB_PATH)
	@rm -f ./misc_code/*.o ./tftp_code/*.o ./qcom_code/*.o ./zte_code/*.o ./unisoc_code/*.o ./eigencomm_code/*.o ./*.o
	@rm -f ./unisoc_x_code/*.o ./sl8563_x_code/*.o
	@rm -f ./samsung_code/samsunglogtool/*.o

$(LOGCAT_TARGET) : $(LOGCAT_OBJECTS)
	$(CXX) $^ -o $@ $(LD_LIB)
	@rm -f ./src/console/*.o ./src/lib/*.o ./src/shared/*.o

$(SILENTLOGGING_TARGET) : $(SILENTLOGGING_OBJECTS)
	$(CC) $^ -o $@ $(LD_LIB)
	@rm -f ./samsung_code/silentlogging/*.o

$(OBJECTS) : %.o : %.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE)

$(LOGCAT_OBJECTS) : %.o : %.c
	$(CC) -c $< -o $@ $(LOGCAT_CFLAGS) $(INCLUDE)

$(SILENTLOGGING_OBJECTS) : %.o : %.c
	$(CC) -c $< -o $@ $(SILENTLOGGING_CFLAGS) $(INCLUDE)

all: $(BIN_TARGET)

ifeq ($(config_samsung), yes)
all: $(BIN_TARGET) $(LOGCAT_TARGET) $(SILENTLOGGING_TARGET)
endif

.PHONY: all clean

clean:
	@rm -rvf $(BIN_TARGET) $(LOGCAT_TARGET) $(SILENTLOGGING_TARGET) ./misc_code/*.o ./tftp_code/*.o ./qcom_code/*.o ./zte_code/*.o ./eigencomm_code/*.o ./unisoc_code/*.o ./*.o ./unisoc_x_code/xlog/lib/*.a ./unisoc_x_code/xlog/obj ./sl8563_x_code/xlog/lib/*.a ./sl8563_x_code/xlog/obj
	@rm -rvf $(OBJECTS) $(LOGCAT_OBJECTS) $(SILENTLOGGING_OBJECTS)
