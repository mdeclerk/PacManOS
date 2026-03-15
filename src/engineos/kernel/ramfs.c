#include "engineos/kernel/ramfs.h"
#include "engineos/kernel/heap.h"
#include "engineos/kernel/log.h"
#include "engineos/kernel/mb2.h"
#include "stdlib/stddef.h"
#include "stdlib/stdint.h"
#include "stdlib/string.h"

#define RAMFS_ID_MAX 128u
#define RAMFS_VERSION 1u
#define RAMFS_HEADER_SIZE 12u
#define RAMFS_MAGIC "ASB0"

struct ramfs_entry {
    const char *id;
    size_t id_len;
    const uint8_t *base;
    size_t size;
};

static struct ramfs_entry *ramfs_entries;
static uint32_t ramfs_entry_count;

static uint16_t read_u16_le(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8u);
}

static uint32_t read_u32_le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) | ((uint32_t)p[2] << 16u)
        | ((uint32_t)p[3] << 24u);
}

void ramfs_init(void)
{
    const struct mb2_module_entry *mod = mb2_find_module_by_magic(RAMFS_MAGIC, 4);
    if (!mod) {
        log("%s: no ramfs found", __func__);
        return;
    }

    const uint8_t *start = (const uint8_t *)mod->start;
    size_t size = (size_t)(mod->end - mod->start);
    const uint8_t *end = start + size;

    if (!start)
        PANIC("ramfs data is nullptr");
    if ((uintptr_t)start + size < (uintptr_t)start)
        PANIC("ramfs data range overflows");
    if (size < RAMFS_HEADER_SIZE)
        PANIC("ramfs blob too small for header");
    if (strncmp((const char *)start, RAMFS_MAGIC, 4u) != 0)
        PANIC("ramfs blob has invalid magic");
    if (read_u16_le(start + 4u) != RAMFS_VERSION)
        PANIC("ramfs blob has unsupported version");
    if (read_u16_le(start + 6u) != 0u)
        PANIC("ramfs blob reserved header field must be zero");

    uint32_t count = read_u32_le(start + 8u);
    const uint8_t *cursor = start + RAMFS_HEADER_SIZE;
    ramfs_entry_count = count;

    if (count > 0u) {
        if ((size_t)count > ((size_t)-1) / sizeof(struct ramfs_entry))
            PANIC("ramfs blob has too many entries");
        ramfs_entries = (struct ramfs_entry *)heap_alloc(
            (size_t)count * sizeof(struct ramfs_entry));
    } else {
        ramfs_entries = nullptr;
    }

    for (uint32_t i = 0u; i < count; i++) {
        if ((size_t)(end - cursor) < 8u)
            PANIC("ramfs blob truncated entry header at index %u", (unsigned)i);

        size_t id_len = read_u32_le(cursor);
        size_t data_size = read_u32_le(cursor + 4u);
        cursor += 8u;

        if (id_len == 0u || id_len > RAMFS_ID_MAX)
            PANIC("ramfs blob has invalid id length at index %u", (unsigned)i);

        if ((size_t)(end - cursor) < id_len)
            PANIC("ramfs blob truncated id bytes at index %u", (unsigned)i);
        const char *id = (const char *)cursor;
        cursor += id_len;

        if ((size_t)(end - cursor) < data_size)
            PANIC("ramfs blob truncated data bytes at index %u", (unsigned)i);
        ramfs_entries[i] = (struct ramfs_entry){
            .id = id,
            .id_len = id_len,
            .base = cursor,
            .size = data_size,
        };
        cursor += data_size;
    }

    if (cursor != end)
        PANIC("ramfs blob has trailing bytes after index");

    log("%s: %u file(s), %u KiB total",
        __func__, (unsigned)count, (uint32_t)(size / 1024u));

    for (uint32_t i = 0u; i < ramfs_entry_count; i++) {
        char name[RAMFS_ID_MAX + 1u];
        strncpy(name, ramfs_entries[i].id, ramfs_entries[i].id_len);
        name[ramfs_entries[i].id_len] = '\0';
        log("   %s (%u KiB)", name,
            (uint32_t)(ramfs_entries[i].size / 1024u));
    }
}

struct ramfs_binary ramfs_get_binary(const char *id)
{
    size_t id_len = strnlen(id, RAMFS_ID_MAX + 1u);
    if (!id || id_len == 0u || id_len > RAMFS_ID_MAX)
        PANIC("invalid ramfs id");

    for (uint32_t i = 0u; i < ramfs_entry_count; i++) {
        if (ramfs_entries[i].id_len == id_len
            && strncmp(ramfs_entries[i].id, id, id_len) == 0) {
            return (struct ramfs_binary){
                .base = (void *)ramfs_entries[i].base,
                .size = ramfs_entries[i].size,
            };
        }
    }

    PANIC("ramfs file not found: %s", id);
    return (struct ramfs_binary){ nullptr, 0 };
}

bool ramfs_validate_binary(const struct ramfs_binary *bin)
{
    return bin->base != nullptr && bin->size != 0u;
}
