#pragma once

#include "stdlib/stdbool.h"
#include "stdlib/stddef.h"

struct ramfs_binary {
    const void *base;
    size_t size;
};

struct ramfs_binary ramfs_get_binary(const char *id);
bool ramfs_validate_binary(const struct ramfs_binary *bin);
