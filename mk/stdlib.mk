# ---------------------------------------------------------------------------
# stdlib  --  freestanding C standard library (static archive)
# ---------------------------------------------------------------------------

STDLIB_SRC_DIR   := $(SRC_DIR)/stdlib
STDLIB_BUILD_DIR := $(BUILD_DIR)/stdlib
STDLIB_OBJ_DIR   := $(BUILD_DIR)/stdlib/obj

STDLIB_SRCS := $(shell find $(SRC_DIR)/stdlib \( -name '*.c' -o -name '*.S' \) -print | sort)
STDLIB_OBJS := $(patsubst $(STDLIB_SRC_DIR)/%,$(STDLIB_OBJ_DIR)/%.o,$(STDLIB_SRCS))
STDLIB_DEPS := $(STDLIB_OBJS:.o=.d)

STDLIB_LIB := $(STDLIB_BUILD_DIR)/libstd.a

# --- Compile ----------------------------------------------------------------

$(STDLIB_OBJ_DIR)/%.c.o: $(STDLIB_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(STDLIB_OBJ_DIR)/%.S.o: $(STDLIB_SRC_DIR)/%.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# --- Archive ----------------------------------------------------------------

.PHONY: stdlib
stdlib: $(STDLIB_LIB)

$(STDLIB_LIB): $(STDLIB_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

ALL_DEPS += $(STDLIB_DEPS)
