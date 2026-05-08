# If use the arm gcc, please set the arm gcc path.
# ARM_GCC = ~/ARM_Linux_GCC/bin/arm-none-linux-gnueabi-

CC = $(ARM_GCC)gcc
CXX = $(ARM_GCC)g++
AR = $(ARM_GCC)ar

CFLAGS = -Wall


CFLAGS += -O2 -g -DNDEBUG   -std=gnu99 -Wall -Wextra -Wno-variadic-macros -Wno-strict-aliasing

CFLAGS += -DCONFIGURATION_FILES_DIR=\"/usr/local/etc\" -DDLT_DAEMON_USE_FIFO_IPC -DDLT_LIB_USE_FIFO_IPC -DDLT_NETWORK_TRACE_ENABLE -DDLT_USER_IPC_PATH=\"/tmp\" -D_GNU_SOURCE


LD_LIB = -lpthread -lrt




INCLUDE = -I./include/dlt  -I./include_from_cmake/dlt -I./src/shared

SRC_FILES = $(wildcard ./src/console/*.c   ./src/lib/*.c )

SRC_FILES += ./src/shared/dlt_common.c ./src/shared/dlt_protocol.c ./src/shared/dlt_user_shared.c

SRC_FILES += ./src/shared/network_api.c

# ./src/shared/*.c

BIN_TARGET = dlt-logcat

OBJECTS = $(patsubst %.c,%.o,$(SRC_FILES))

$(BIN_TARGET) : $(OBJECTS)
	$(CXX) $^ -o $@ $(LD_LIB) $(LIB_PATH)
	@rm -f ./src/console/*.o ./src/lib/*.o ./src/shared/*.o

$(OBJECTS) : %.o : %.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE)

.PHONY: clean

clean:
	@rm -rvf $(BIN_TARGET) ./src/console/*.o ./src/lib/*.o ./src/shared/*.o  
	@rm -rvf $(OBJECTS)
