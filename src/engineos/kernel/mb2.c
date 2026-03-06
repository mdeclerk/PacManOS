#include "mb2.h"
#include "log.h"
#include "stdlib/string.h"

#define MB2_TAG_TYPE_MODULE      3
#define MB2_TAG_TYPE_MMAP        6
#define MB2_TAG_TYPE_FRAMEBUFFER 8

#define MB2_MMAP_AVAILABLE 1

struct mb2_tag {
    uint32_t type;
    uint32_t size;
}__attribute(( packed ));

struct mb2_tag_module {
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    char cmdline[];
}__attribute(( packed ));

struct mb2_tag_mmap_entry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
}__attribute(( packed ));

struct mb2_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct mb2_tag_mmap_entry entries[];
}__attribute(( packed ));

struct mb2_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    uint8_t fb_type;
    uint16_t reserved;
}__attribute(( packed ));

struct mb2_info mb2_info;

static inline void parse_tag_module(const struct mb2_tag_module *mod)
{
    if (mod->mod_end <= mod->mod_start)
        PANIC("Invalid module range");
    if (mb2_info.modules.count >= MB2_MAX_MODULE_ENTRIES)
        PANIC("Too many module entries");

    mb2_info.modules.entries[mb2_info.modules.count].start = (uintptr_t)mod->mod_start;
    mb2_info.modules.entries[mb2_info.modules.count].end   = (uintptr_t)mod->mod_end;
    strncpy(mb2_info.modules.entries[mb2_info.modules.count].cmdline,
            mod->cmdline,
            MB2_MODULE_CMDLINE_MAX - 1);
    mb2_info.modules.entries[mb2_info.modules.count].cmdline[MB2_MODULE_CMDLINE_MAX - 1] = '\0';

    log("   module[%u]: %p-%p, size %u KiB, cmdline '%s'",
        (uint32_t)mb2_info.modules.count,
        (void *)mb2_info.modules.entries[mb2_info.modules.count].start,
        (void *)mb2_info.modules.entries[mb2_info.modules.count].end,
        (uint32_t)((mb2_info.modules.entries[mb2_info.modules.count].end -
                    mb2_info.modules.entries[mb2_info.modules.count].start) / 1024u),
        mb2_info.modules.entries[mb2_info.modules.count].cmdline);

    mb2_info.modules.count++;
}

static inline void parse_tag_mmap(const struct mb2_tag_mmap *mmap_tag)
{
    uint32_t num_entries = (mmap_tag->size - sizeof(struct mb2_tag_mmap)) / mmap_tag->entry_size;
    for (uint32_t i = 0; i < num_entries; i++) {
        const struct mb2_tag_mmap_entry *entry =
            (const struct mb2_tag_mmap_entry *)((uint8_t *)mmap_tag->entries + i * mmap_tag->entry_size);
        if (entry->type == MB2_MMAP_AVAILABLE) {
            if (mb2_info.mmap.count >= MB2_MAX_MMAP_ENTRIES)
                PANIC("Too many mmap entries");
            mb2_info.mmap.entries[mb2_info.mmap.count].base = (uintptr_t)entry->base_addr;
            mb2_info.mmap.entries[mb2_info.mmap.count].size = (size_t)entry->length;

            uintptr_t entry_end = mb2_info.mmap.entries[mb2_info.mmap.count].base +
                                   (uintptr_t)mb2_info.mmap.entries[mb2_info.mmap.count].size;
            uintptr_t entry_end_aligned = entry_end & ~(uintptr_t)0xFFF;
            if (entry_end_aligned > mb2_info.mmap.max_mem)
                mb2_info.mmap.max_mem = entry_end_aligned;

            log("   mmap[%u]: base %p, size %u KiB",
                mb2_info.mmap.count,
                (void *)mb2_info.mmap.entries[mb2_info.mmap.count].base,
                (uint32_t)(mb2_info.mmap.entries[mb2_info.mmap.count].size / 1024u));

            mb2_info.mmap.count++;
        }
    }
}

static inline void parse_tag_framebuffer(const struct mb2_tag_framebuffer *fb)
{
    mb2_info.fb.addr   = (uintptr_t)fb->addr;
    mb2_info.fb.pitch  = fb->pitch;
    mb2_info.fb.width  = fb->width;
    mb2_info.fb.height = fb->height;

    log("   framebuffer: %ux%u, pitch %u, addr %p",
        mb2_info.fb.width, mb2_info.fb.height,
        mb2_info.fb.pitch, (void *)mb2_info.fb.addr);
}

void mb2_init(void *ptr)
{
    if (!ptr)
        PANIC("Multiboot2 info pointer is NULL");

    log("%s: addr %p", __func__, ptr);

    struct mb2_tag *tag = (struct mb2_tag *)ptr + 1;
    while (tag->type) {
        switch (tag->type) {
            case MB2_TAG_TYPE_MODULE:
                parse_tag_module((const struct mb2_tag_module *)tag);
                break;
            case MB2_TAG_TYPE_MMAP:
                parse_tag_mmap((const struct mb2_tag_mmap *)tag);
                break;
            case MB2_TAG_TYPE_FRAMEBUFFER:
                parse_tag_framebuffer((const struct mb2_tag_framebuffer *)tag);
                break;
            default:
                break;
        }
        tag = (struct mb2_tag *)((uintptr_t)tag + ((tag->size + 7u) & ~7u));
    }
}

const struct mb2_module_entry *mb2_find_module_by_magic(const void *magic, size_t len)
{
    for (size_t i = 0; i < mb2_info.modules.count; i++) {
        const struct mb2_module_entry *mod = &mb2_info.modules.entries[i];
        if (mod->end - mod->start >= len &&
            memcmp((const void *)mod->start, magic, len) == 0) {
            return mod;
        }
    }
    return NULL;
}
