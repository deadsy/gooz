
CURRENT_DIR = $(shell pwd)

DL_DIR = $(CURRENT_DIR)/dl
SRC_DIR = $(CURRENT_DIR)/src
XTOOLS_DIR = $(CURRENT_DIR)/xtools
TOOLS_DIR = $(CURRENT_DIR)/tools
BUILD_DIR = $(CURRENT_DIR)/build
OUTPUT_DIR = $(CURRENT_DIR)/output

# cross compilation tools for ARM cortex-m
GCC_RELEASE = 10-2020q4
GCC_VERSION = 10-2020-q4-major
GCC_FILE = gcc-arm-none-eabi-$(GCC_VERSION)-x86_64-linux.tar.bz2
GCC_TBZ = $(DL_DIR)/$(GCC_FILE)
GCC_URL = https://developer.arm.com/-/media/Files/downloads/gnu-rm/$(GCC_RELEASE)/$(GCC_FILE)

# zephyr os
ZEPHYR_VER = 2.4.0
ZEPHYR_URL = https://github.com/zephyrproject-rtos/zephyr/archive/zephyr-v$(ZEPHYR_VER).tar.gz
ZEPHYR_FILE = zephyr-$(ZEPHYR_VER).tar.gz
ZEPHYR_TGZ = $(DL_DIR)/$(ZEPHYR_FILE)
ZEPHYR_SRC = $(SRC_DIR)/zephyr
ZEPHYR_BASE = $(ZEPHYR_SRC)
ZEPHYR_MODULES = 
ZEPHYR_TOOLCHAIN_VARIANT = cross-compile
CROSS_COMPILE = $(XTOOLS_DIR)/bin/arm-none-eabi-

# stm32 hal (https://github.com/zephyrproject-rtos/hal_stm32)
STM32_VER = cc8731dba4fd9c57d7fe8ea6149828b89c2bd635
STM32_URL = https://github.com/zephyrproject-rtos/hal_stm32/archive/$(STM32_VER).tar.gz
STM32_FILE = stm32-$(STM32_VER).tar.gz
STM32_TGZ = $(DL_DIR)/$(STM32_FILE)
STM32_SRC = $(ZEPHYR_SRC)/modules/hal/stm32
MODULES += $(STM32_SRC)

# ARM CMSIS (https://github.com/zephyrproject-rtos/cmsis)
CMSIS_VER = 421dcf358fa420e9721a8452c647f0d42af8d68c
CMSIS_URL = https://github.com/zephyrproject-rtos/cmsis/archive/$(CMSIS_VER).tar.gz
CMSIS_FILE = cmsis-$(CMSIS_VER).tar.gz
CMSIS_TGZ = $(DL_DIR)/$(CMSIS_FILE)
CMSIS_SRC = $(ZEPHYR_SRC)/modules/hal/cmsis
MODULES += $(ZEPHYR_SRC)/modules/hal/cmsis

# segger (https://github.com/zephyrproject-rtos/segger)
SEGGER_VER = 38c79a447e4a47d413b4e8d34448316a5cece77c
SEGGER_URL = https://github.com/zephyrproject-rtos/segger/archive/$(SEGGER_VER).tar.gz
SEGGER_FILE = segger-$(CMSIS_VER).tar.gz
SEGGER_TGZ = $(DL_DIR)/$(SEGGER_FILE)
SEGGER_SRC = $(ZEPHYR_SRC)/modules/hal/segger
MODULES += $(SEGGER_SRC)

#BOARD = axoloti
BOARD = stm32f4_disco

GGM_APP = $(CURRENT_DIR)/ggm
GGM_BUILD = $(BUILD_DIR)/ggm

# build the zephyr modules string
EMPTY = 
SPACE = $(EMPTY) $(EMPTY)
ZEPHYR_MODULES="$(subst $(SPACE),;,$(MODULES))"

# other build tools
FWBUILD = $(CURRENT_DIR)/fwbuild
IMGTOOL = $(MCUBOOT_SRC)/scripts/imgtool.py

PATCHFILES := $(sort $(wildcard patches/*.patch ))

PATCH_CMD = \
  for f in $(PATCHFILES); do\
      echo $$f ":"; \
      patch -b -p1 < $$f || exit 1; \
  done

COPY_CMD = tar cf - -C files . | tar xf - -C $(SRC_DIR)

.PHONY: all
all: .stamp_output

.stamp_output: .stamp_xtools .stamp_build
	mkdir -p $(OUTPUT_DIR)
	touch $@

.stamp_build: .stamp_cmake
	ninja -C $(GGM_BUILD)
	#ninja -C $(MCUBOOT_BUILD)
	touch $@

.stamp_cmake: .stamp_src
	# ggm app
	-rm -rf $(GGM_BUILD)
	ZEPHYR_BASE=$(ZEPHYR_BASE) \
	ZEPHYR_TOOLCHAIN_VARIANT=$(ZEPHYR_TOOLCHAIN_VARIANT) \
	CROSS_COMPILE=$(CROSS_COMPILE) \
	cmake -GNinja -DZEPHYR_MODULES=$(ZEPHYR_MODULES) -DBOARD=$(BOARD) -S $(GGM_APP) -B $(GGM_BUILD)
	touch $@

.PHONY: ggm_config
ggm_config: .stamp_cmake
	ninja -C $(GGM_BUILD) menuconfig
	-mv $(GGM_BUILD)/zephyr/kconfig/defconfig $(GGM_APP)/prj.conf

.PHONY: rebuild
rebuild:
	-rm -rf $(BUILD_DIR) .stamp_build .stamp_cmake
	-rm -rf $(OUTPUT_DIR) .stamp_output
	make

.PHONY: clean
clean:
	-rm -rf $(BUILD_DIR) .stamp_build .stamp_cmake
	-rm -rf $(SRC_DIR) .stamp_src
	-rm -rf $(OUTPUT_DIR) .stamp_output

.PHONY: clobber
clobber: clean
	-rm -rf $(XTOOLS_DIR) .stamp_xtools

$(ZEPHYR_TGZ):
	mkdir -p $(DL_DIR)
	wget $(ZEPHYR_URL) -O $(ZEPHYR_TGZ)

$(GCC_TBZ):
	mkdir -p $(DL_DIR)
	wget $(GCC_URL) -O $(GCC_TBZ)

$(CMSIS_TGZ):
	mkdir -p $(DL_DIR)
	wget $(CMSIS_URL) -O $(CMSIS_TGZ)

$(STM32_TGZ):
	mkdir -p $(DL_DIR)
	wget $(STM32_URL) -O $(STM32_TGZ)

$(SEGGER_TGZ):
	mkdir -p $(DL_DIR)
	wget $(SEGGER_URL) -O $(SEGGER_TGZ)

.stamp_xtools: $(GCC_TBZ)
	-rm -rf $(XTOOLS_DIR)
	mkdir -p $(XTOOLS_DIR)
	tar -C $(XTOOLS_DIR) -jxf $(GCC_TBZ) --strip-components 1
	touch $@

.stamp_src: $(ZEPHYR_TGZ) $(STM32_TGZ) $(CMSIS_TGZ) $(SEGGER_TGZ) $(PATCHFILES)
	mkdir -p $(ZEPHYR_SRC)
	tar -C $(ZEPHYR_SRC) -zxf $(ZEPHYR_TGZ) --strip-components 1
	mkdir -p $(STM32_SRC)
	tar -C $(STM32_SRC) -zxf $(STM32_TGZ) --strip-components 1
	mkdir -p $(CMSIS_SRC)
	tar -C $(CMSIS_SRC) -zxf $(CMSIS_TGZ) --strip-components 1
	mkdir -p $(SEGGER_SRC)
	tar -C $(SEGGER_SRC) -zxf $(SEGGER_TGZ) --strip-components 1
	$(COPY_CMD)
	$(PATCH_CMD)
	touch $@
