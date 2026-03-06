#pragma once

#include "stdlib/stddef.h"
#include "stdlib/stdint.h"

struct elf_image {
    uintptr_t load_start;
    uintptr_t load_end;
};

struct elf_image *elf_load(const void *data, size_t size);
void              elf_free(struct elf_image *img);
uintptr_t         elf_symbol(const struct elf_image *img, const char *name);
