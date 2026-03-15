CROSS ?= i686-elf-
BUILD ?= debug

CC      := $(CROSS)gcc
LD      := $(CROSS)ld
AR      := $(CROSS)ar
OBJCOPY := $(CROSS)objcopy
PYTHON  := python3

SRC_DIR       := src
ASSETS_DIR    := assets
ASSETS_PACKER := scripts/bundle_ramfs.py
OUT_DIR       := out
BUILD_DIR      := $(OUT_DIR)/$(BUILD)

GAMES := pacman helloworld

WFLAGS  := -Wall -Wextra -Werror -Wpedantic
CFLAGS  := -m32 -std=gnu23 -ffreestanding $(WFLAGS) -I$(SRC_DIR) -MMD -MP
LDFLAGS := -m elf_i386

ifeq ($(BUILD),release)
    CFLAGS += -O2 -DNDEBUG
else ifeq ($(BUILD),debug)
    CFLAGS += -Og -g -fno-omit-frame-pointer
else
    $(error Unknown build type: $(BUILD))
endif

.PHONY: print-games
print-games:
	@for game in $(GAMES); do echo "$$game"; done
