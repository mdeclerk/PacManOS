ALL_DEPS :=
.DEFAULT_GOAL := all

include mk/_config.mk

include mk/stdlib.mk
include mk/engineos.mk
include mk/games.mk
include mk/iso.mk

.PHONY: all clean

all: iso

clean:
	rm -rf $(OUT_DIR)

-include $(ALL_DEPS)
