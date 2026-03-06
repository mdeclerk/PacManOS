# ---------------------------------------------------------------------------
# engineos  --  kernel binary (loaded by GRUB via multiboot2)
# ---------------------------------------------------------------------------

ENGINEOS_SRC_DIR   := $(SRC_DIR)/engineos
ENGINEOS_BUILD_DIR := $(BUILD_DIR)/engineos
ENGINEOS_OBJ_DIR   := $(BUILD_DIR)/engineos/obj

ENGINEOS_SRCS := $(shell find $(SRC_DIR)/engineos \( -name '*.c' -o -name '*.S' \) -print | sort)
ENGINEOS_OBJS := $(patsubst $(ENGINEOS_SRC_DIR)/%,$(ENGINEOS_OBJ_DIR)/%.o,$(ENGINEOS_SRCS))
ENGINEOS_DEPS := $(ENGINEOS_OBJS:.o=.d)

ENGINEOS_ELF    := $(ENGINEOS_BUILD_DIR)/engineos.elf
ENGINEOS_LINKER := $(ENGINEOS_SRC_DIR)/linker.ld

# --- Compile ----------------------------------------------------------------

$(ENGINEOS_OBJ_DIR)/%.c.o: $(ENGINEOS_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(ENGINEOS_OBJ_DIR)/%.S.o: $(ENGINEOS_SRC_DIR)/%.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# --- Link -------------------------------------------------------------------

.PHONY: engineos
engineos: $(ENGINEOS_ELF)

$(ENGINEOS_ELF): $(ENGINEOS_LINKER) $(ENGINEOS_OBJS) $(STDLIB_LIB)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -T $(ENGINEOS_LINKER) -o $@ $(ENGINEOS_OBJS) $(STDLIB_LIB)

ALL_DEPS += $(ENGINEOS_DEPS)
