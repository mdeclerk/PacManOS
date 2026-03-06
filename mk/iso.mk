# ---------------------------------------------------------------------------
# iso  --  bootable GRUB ISO image
# ---------------------------------------------------------------------------

ISO_DIR      := $(BUILD_DIR)/iso
ISO_BOOT_DIR := $(ISO_DIR)/boot
ISO_GRUB_DIR := $(ISO_BOOT_DIR)/grub
ISO          := $(BUILD_DIR)/pacmanos.iso
GRUB_CFG     := $(ISO_GRUB_DIR)/grub.cfg

.PHONY: iso
iso: $(ISO)

# --- Copy kernel -----------------------------------------------------------

$(ISO_BOOT_DIR)/engineos.elf: $(ENGINEOS_ELF)
	@mkdir -p $(dir $@)
	cp $< $@

# --- Copy game ELFs and RAMFS into ISO tree --------------------------------

define ISO_GAME_RULE
$(ISO_BOOT_DIR)/$(1).elf: $$($(1)_ELF)
	@mkdir -p $$(dir $$@)
	cp $$< $$@

$(ISO_BOOT_DIR)/$(1).ramfs: $$($(1)_RAMFS)
	@mkdir -p $$(dir $$@)
	cp $$< $$@
endef

$(foreach g,$(GAMES),$(eval $(call ISO_GAME_RULE,$(g))))

# --- GRUB configuration ---------------------------------------------------

GRUB_TIMEOUT := $(if $(word 2,$(GAMES)),-1,0)

$(GRUB_CFG): FORCE
	@mkdir -p $(dir $@)
	@{ \
	  echo 'set timeout=$(GRUB_TIMEOUT)'; \
	  echo 'set default=0'; \
	  echo ''; \
	  $(foreach g,$(GAMES), \
	    echo 'menuentry "$(g)" {'; \
	    echo '  multiboot2 /boot/engineos.elf'; \
	    echo '  module2 /boot/$(g).elf'; \
	    echo '  module2 /boot/$(g).ramfs'; \
	    echo '  boot'; \
	    echo '}'; \
	    echo '';) \
	} > $@

.PHONY: FORCE
FORCE:

# --- Build ISO -------------------------------------------------------------

ISO_ARTIFACTS := $(ISO_BOOT_DIR)/engineos.elf \
                 $(foreach g,$(GAMES),$(ISO_BOOT_DIR)/$(g).elf $(ISO_BOOT_DIR)/$(g).ramfs)

$(ISO): $(ISO_ARTIFACTS) $(GRUB_CFG)
	grub-mkrescue -o $@ $(ISO_DIR)
