# If use the arm gcc, please set the arm gcc path.
# ARM_GCC = ~/ARM_Linux_GCC/bin/arm-none-linux-gnueabi-

CC = $(ARM_GCC)gcc

config_eigencomm = yes

CFLAGS = -g -Wall

ifeq ($(config_eigencomm), yes)
CFLAGS += -DCONFIG_EIGENCOMM
endif



LD_LIBRARY = -lpthread

INCLUDE = -I./zte_code -I./qcom_code -I./unisoc_code -I./misc_code -I./eigencomm_code -I./smr5300_code
SOURCES = $(wildcard ./zte_code/*.c ./qcom_code/*.c ./unisoc_code/*.c ./misc_code/*.c ./smr5300_code/*.c ./*.c)


ifeq ($(config_eigencomm), yes)
SOURCES += $(wildcard ./eigencomm_code/*.c)
endif

BIN_TARGET = upgrade_tool

linux: clean
	${CC} ${CFLAGS} -s ${SOURCES} ${INCLUDE} -o ${BIN_TARGET} $(LD_LIBRARY)

.PHONY: clean

clean:
	@rm -rvf $(BIN_TARGET)
