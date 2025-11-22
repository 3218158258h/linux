CC := $(CROSS_COMPILE)gcc

# 工具链配置
TOOLCHAIN_DIR := /usr/local/arm/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf
TOOLCHAIN_BIN := $(TOOLCHAIN_DIR)/bin  # 编译器所在目录
export PATH := $(TOOLCHAIN_BIN):$(PATH) 
# 工具链 sysroot 路径
REAL_SYSROOT := /usr/local/arm/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/../arm-linux-gnueabihf/libc

# 编译选项
CFLAGS += -Wall -Wextra -std=c99 -D_GNU_SOURCE
CFLAGS += -I/home/test/gateway  # 项目自身头文件目录
ifdef CROSS_COMPILE
# 启用 sysroot
CFLAGS += --sysroot=$(REAL_SYSROOT)
# 链接时指定 sysroot
LDFLAGS += --sysroot=$(REAL_SYSROOT)
endif

# 链接选项（依赖的 ARM 库，保持不变）
LDFLAGS += -lpaho-mqtt3c -lcurl -lcrypto -lpthread -lrt

# 开发板配置
PEER := root@192.168.1.201

# 源文件/目标文件（保持不变）
SRCS += $(shell find app -type f -name "*.c")
SRCS += $(shell find daemon -type f -name "*.c")
SRCS += $(shell find ota -type f -name "*.c")
SRCS += $(shell find thirdparty -type f -name "*.c")
OBJS = $(SRCS:.c=.o)
TARGET = gateway


.PHONY:  clean cross  test_buffer test_mqtt test_message test_task all-local


# 本地编译
all-local: $(TARGET)

# 交叉编译
cross:
	CROSS_COMPILE=arm-linux-gnueabihf- make all-local

# 编译规则
$(TARGET): $(OBJS) main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 清理目标
clean:
	$(RM) $(OBJS) main.o $(TARGET) test_buffer test_mqtt test_message test_task


# 测试目标
test_buffer: $(OBJS) test/test_buffer.o
	-@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	-@./$@
	-@$(RM) $@ $^

test_mqtt: $(OBJS) test/test_mqtt.o
	-@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	-@./$@
	-@$(RM) $@ $^

test_message: $(OBJS) test/test_message.o
	-@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	-@./$@
	-@$(RM) $@ $^

test_task: $(OBJS) test/test_task.o
	-@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	-@./$@
	-@$(RM) $@ $^
