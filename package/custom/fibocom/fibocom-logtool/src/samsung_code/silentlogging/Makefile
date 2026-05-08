# 指定编译器
#CC = $cc

# 编译选项
CFLAGS = -Wall -g -I./include -I.

# 源文件
SRCS = silentlogging.c DMFileManager.c network_api.c

# 目标文件
OBJS = $(SRCS:.c=.o)

# 可执行文件的名称
TARGET = silentlogging

# 默认目标
all: $(TARGET)

# 链接目标文件生成可执行文件
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) -ldl -lpthread

# 编译每个源文件为目标文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理目标
clean:
	rm -f $(OBJS) $(TARGET)

# 伪目标
.PHONY: all clean
