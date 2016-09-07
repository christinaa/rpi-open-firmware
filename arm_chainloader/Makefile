
TARGET_ARM_CHAINLOADER = arm_chainloader.bin

SRCS = \
	start.s \
	boot.s \
	lib/arm_bcopy.s \
	lib/arm_bzero.s \
	lib/arm_locore.s \
	drivers/uart.c \
	drivers/cprman.cc \
	drivers/fatfs/ff.c \
	drivers/sdhost_impl.cc \
	drivers/mbr_disk.cc \
	drivers/mailbox.cc \
	../lib/xprintf.c \
	../lib/panic.c \
	../lib/udelay.c \
	../lib/tlsf/tlsf.c \
	firmware_rendezvous.c \
	minicxx.cc \
	loader.cc \
	main.c

ARCH = armv6zk

BUILD_DIR = build
TARGET_BUILD_DIR = $(BUILD_DIR)/$(ARCH)-objects
PRODUCT_DIRECTORY = $(BUILD_DIR)

NO_COLOR=""
OK_COLOR=""
ERROR_COLOR=""
WARN_COLOR=""

.PHONY: default all clean create_build_directory device

default: $(TARGET_ARM_CHAINLOADER)

OBJ := $(addprefix $(TARGET_BUILD_DIR)/, $(addsuffix .o, $(basename $(SRCS))))

CROSS_COMPILE = arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)gcc
AS = $(CC)
OBJCOPY = $(CROSS_COMPILE)objcopy
LINKFLAGS = -nostdlib -march=$(ARCH) -Wl,--build-id=none -T linker.lds
COMMON_FLAGS = -c -nostdlib -nostartfiles -ffreestanding -march=$(ARCH) -I../ -I./ -mfpu=vfp -mfloat-abi=hard -mtune=arm1176jzf-s -Wall -Wextra
CFLAGS = $(COMMON_FLAGS) -std=c11 
CXXFLAGS = $(COMMON_FLAGS) -std=c++11 -fno-exceptions -fno-rtti
ASFLAGS = $(COMMON_FLAGS) -x assembler-with-cpp

HEADERS := \
	$(shell find . -type f -name '*.h') \
	$(shell find . -type f -name '*.hpp')

create_build_directory:
	@mkdir -p $(TARGET_BUILD_DIR)
	@mkdir -p $(PRODUCT_DIRECTORY)

CREATE_SUBDIR = \
	@DIR="$(dir $@)"; \
	if [ ! -d $$DIR ]; then mkdir -p $$DIR; fi

#
# rules to build c/asm files.
# 
$(TARGET_BUILD_DIR)/%.o: %.c $(HEADERS)
	$(CREATE_SUBDIR)
	@echo $(WARN_COLOR)CC  $(NO_COLOR) $@
	@$(CC) $(CFLAGS) $< -o $@

$(TARGET_BUILD_DIR)/%.o: %.cc $(HEADERS)
	$(CREATE_SUBDIR)
	@echo $(WARN_COLOR)CXX $(NO_COLOR) $@
	@$(CXX) $(CXXFLAGS) $< -o $@

$(TARGET_BUILD_DIR)/%.o: %.s $(HEADERS)
	$(CREATE_SUBDIR)
	@echo $(WARN_COLOR)AS  $(NO_COLOR) $@
	@$(AS) $(ASFLAGS) $< -o $@

.PRECIOUS: $(OBJ)

$(TARGET_ARM_CHAINLOADER): create_build_directory $(OBJ)
	@echo $(WARN_COLOR)LD  $(NO_COLOR) $@.elf
	@$(CC) $(LINKFLAGS) $(OBJ) -o $(PRODUCT_DIRECTORY)/$@.elf -lgcc
	@echo $(WARN_COLOR)OBJ$(NO_COLOR) $@
	@$(OBJCOPY) -O binary $(PRODUCT_DIRECTORY)/$@.elf $(PRODUCT_DIRECTORY)/$@

clean:
	@echo $(ERROR_COLOR)CLEAN$(NO_COLOR)
	@-rm -rf ./$(BUILD_DIR)
