# ---------------------------------------------------------------------------------
# 			Makefile to compile lowi_test binary on INFRA platform
# ---------------------------------------------------------------------------------

ifneq ($(strip $(TOOLPREFIX)),)
CROSS:=$(TOOLPREFIX)
endif

CC= $(CROSS)gcc

INC_DIRS+= -I. \
    -I../inc


SHARED_LIB_PATH= \
    -L$(INSTALL_ROOT)/lib -Wl,-rpath=/usr/lib/lowi

SHARED_LIBS= \
  -llowi_client -llowi_interface

CFLAGS+= ${INC_DIRS} -MMD -O2 -g -lm -lpthread

CFLAGS+= -fpie
LDFLAGS = -pie

HEADERS=$(wildcard *.h)
COMMON_HEADERS=$(wildcard ../inc/*.h)

%.o: %.cpp
	$(CXX) -c -o $@ ${CXXFLAGS} $(CFLAGS) ${COPTS} $<
	@echo "  CXX " $<

ifndef CONFIG_OS
CONFIG_OS=unix
endif

LIB_OBJS=$(patsubst %.cpp,%.o,$(wildcard *.cpp))
LIB_OBJS+=$(patsubst %.cpp,%.o,$(wildcard infra/*.cpp))

ALL= lowi-lib-test

all: ${ALL}

lowi-lib-test: $(LIB_OBJS) $(STATIC_LIBS)
	$(CXX) $(SHARED_LIB_PATH) -o $@ ${CXXFLAGS} ${CFLAGS} ${LDFLAGS} -Wall -Werror $(LIB_OBJS) $(SHARED_LIBS)

clean:
	rm -f ${ALL}
