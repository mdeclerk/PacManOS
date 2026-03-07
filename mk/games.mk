# ---------------------------------------------------------------------------
# game  --  generic userland game modules (relocatable ELFs loaded by engineos)
# ---------------------------------------------------------------------------

define GAME_RULE
$(1)_SRC_DIR   := $(SRC_DIR)/$(1)
$(1)_BUILD_DIR := $(BUILD_DIR)/$(1)
$(1)_OBJ_DIR   := $(BUILD_DIR)/$(1)/obj

$(1)_SRCS := $$(shell find $(SRC_DIR)/$(1) \( -name '*.c' -o -name '*.S' \) -print | sort)
$(1)_ASSET_FILES := $$(shell find $(ASSETS_DIR)/$(1) -type f -print | sort)
$(1)_OBJS := $$(patsubst $$($(1)_SRC_DIR)/%,$$($(1)_OBJ_DIR)/%.o,$$($(1)_SRCS))
$(1)_DEPS := $$($(1)_OBJS:.o=.d)

$(1)_ELF   := $$($(1)_BUILD_DIR)/$(1).elf
$(1)_RAMFS := $$($(1)_BUILD_DIR)/$(1).ramfs

# --- Ramfs file -------------------------------------------------------------

$$($(1)_RAMFS): $(ASSETS_PACKER) $$($(1)_ASSET_FILES)
	@mkdir -p $$(dir $$@)
	$(PYTHON) $(ASSETS_PACKER) --dir $(ASSETS_DIR)/$(1) --out $$@

# --- Object files -----------------------------------------------------------

$$($(1)_OBJ_DIR)/%.o: $$($(1)_SRC_DIR)/%
	@mkdir -p $$(dir $$@)
	$(CC) $(CFLAGS) -c $$< -o $$@

# --- Link -------------------------------------------------------------------

.PHONY: $(1)
$(1): $$($(1)_ELF)

$$($(1)_ELF): $$($(1)_OBJS) $(STDLIB_LIB) $$($(1)_RAMFS)
	@mkdir -p $$(dir $$@)
	$(LD) $(LDFLAGS) -r -o $$@ $$($(1)_OBJS) $(STDLIB_LIB)

ALL_DEPS += $$($(1)_DEPS)
endef

$(foreach game,$(GAMES),$(eval $(call GAME_RULE,$(game))))
