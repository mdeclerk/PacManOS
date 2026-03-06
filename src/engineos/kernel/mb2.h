#pragma once

#include "stdlib/stdint.h"
#include "stdlib/stddef.h"

#define MB2_MAX_MMAP_ENTRIES     8
#define MB2_MAX_MODULE_ENTRIES   8
#define MB2_MODULE_CMDLINE_MAX  64

struct mb2_module_entry {
    uintptr_t start;
    uintptr_t end;
    char cmdline[MB2_MODULE_CMDLINE_MAX];
};

struct mb2_modules_info {
    struct mb2_module_entry entries[MB2_MAX_MODULE_ENTRIES];
    size_t count;
};

struct mb2_mmap_entry {
    uintptr_t base;
    size_t size;
};

struct mb2_mmap_info {
    struct mb2_mmap_entry entries[MB2_MAX_MMAP_ENTRIES];
    size_t count;
    uintptr_t max_mem;
};

struct mb2_fb_info {
    uintptr_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
};

struct mb2_info {
    struct mb2_modules_info modules;
    struct mb2_mmap_info mmap;
    struct mb2_fb_info fb;
};

extern struct mb2_info mb2_info;

void mb2_init(void *ptr);
const struct mb2_module_entry *mb2_find_module_by_magic(const void *magic, size_t len);
