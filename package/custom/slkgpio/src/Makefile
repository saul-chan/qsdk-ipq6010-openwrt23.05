VERSION    =  2.00
CC       =  g++
DEBUG      =  -DUSE_DEBUG
CFLAGS     =  -Wall
SOURCES	   =  slkgpio.cpp
INCLUDES   =  -I StdAfx.h
LIB_NAMES  =  -lpthread -lstdc++
LIB_PATH   =  
OBJ        =  $(SOURCES:.c=.o)
TARGET     =  slkgpio

#links
$(TARGET):$(OBJ)
	$(CC) $(SOURCES) -o $(TARGET) $(LIB_NAMES)
	
#compile
%.o: %.c
	$(CC) $(INCLUDES) $(DEBUG) -c $(CFLAGS) $< -o $@

.PHONY:clean
clean:
	@echo "Remove linked and compiled files......"
	rm -rf $(OBJ) $(TARGET) output 


