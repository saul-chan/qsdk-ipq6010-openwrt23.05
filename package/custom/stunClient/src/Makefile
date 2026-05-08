VERSION    =  2.00
CXX        =  g++
DEBUG      =  -DUSE_DEBUG
CFLAGS     =  -Wall
SOURCES	   =  client.cpp  stun.cpp  udp.cpp
INCLUDES   =
LIB_NAMES  =  -lpthread
LIB_PATH   =  
OBJ        =  $(SOURCES:.c=.o)
TARGET     =  stunClient

#links
$(TARGET):$(OBJ)
	$(CXX) $(SOURCES) -o $(TARGET) $(LIB_NAMES)
	
#compile
%.o: %.c
	$(CXX) $(INCLUDES) $(DEBUG) -c $(CFLAGS) $< -o $@

.PHONY:clean
clean:
	@echo "Remove linked and compiled files......"
	rm -rf $(OBJ) $(TARGET) output 


