#include "heap.h"
#include "log.h"
#include "mb2.h"
#include "stdlib/stdint.h"
#include "stdlib/stdbool.h"

#define HEAP_ALIGN     16
#define HEAP_MIN_ALLOC 16

static inline uintptr_t align_up(uintptr_t addr)
{
    return (addr + HEAP_ALIGN - 1) & ~(uintptr_t)(HEAP_ALIGN - 1);
}

static inline uintptr_t align_down(uintptr_t addr)
{
    return addr & ~(uintptr_t)(HEAP_ALIGN - 1);
}

extern uint8_t _heap_start[]; // linker.ld

static void *heap_base;
static void *heap_limit;

struct block {
    bool          free;
    struct block *prev;
    struct block *next;
} __attribute__((aligned(HEAP_ALIGN)));

static inline size_t block_size(struct block *blk)
{
    size_t start = (size_t)blk;
    size_t end = (blk->next != NULL) ? (size_t)blk->next : (size_t)heap_limit;
    return  end - start;
}

static inline void *block_to_ptr(struct block *blk)
{
    return (void *)((uint8_t *)blk + sizeof(struct block));
}

static inline struct block *ptr_to_block(void *ptr)
{
    return (struct block *)((uint8_t *)ptr - sizeof(struct block));
}

static inline size_t bytes_to_kib(size_t bytes)
{
    return (bytes + 1023u) / 1024u;
}

#ifndef NDEBUG
static void heap_selftest()
{
    log("%s: running...", __func__);

    // Test 1: Basic allocation
    void *p1 = heap_alloc(32);
    bool t1 = (p1 != NULL);
    log("  alloc(32)        : %s (%p)", t1 ? "PASS" : "FAIL", p1);

    // Test 2: Second allocation
    void *p2 = heap_alloc(64);
    bool t2 = (p2 != NULL) && (p2 != p1);
    log("  alloc(64)        : %s (%p)", t2 ? "PASS" : "FAIL", p2);

    // Test 3: Alignment check
    bool t3 = ((uintptr_t)p1 % HEAP_ALIGN == 0) && ((uintptr_t)p2 % HEAP_ALIGN == 0);
    log("  alignment(%u)    : %s", HEAP_ALIGN, t3 ? "PASS" : "FAIL");

    // Test 4: Free and realloc (coalesce test)
    heap_free(p1);
    heap_free(p2);
    void *p3 = heap_alloc(128);
    bool t4 = (p3 != NULL);
    log("  free+realloc     : %s (%p)", t4 ? "PASS" : "FAIL", p3);

    // Test 5: Zero-size allocation returns NULL
    void *p4 = heap_alloc(0);
    bool t5 = (p4 == NULL);
    log("  alloc(0)==NULL   : %s", t5 ? "PASS" : "FAIL");

    // Test 6: Reserve exact previously-allocated address
    heap_free(p3);
    void *p5 = heap_alloc(96);
    heap_free(p5);
    void *p6 = heap_alloc_at(p5, 96);
    bool t6 = (p6 == p5);
    log("  alloc_at(exact)  : %s (%p)", t6 ? "PASS" : "FAIL", p6);

    // Test 7: Normal alloc should avoid reserved fixed address
    void *p7 = heap_alloc(96);
    bool t7 = (p7 != NULL) && (p7 != p6);
    log("  alloc avoids at  : %s (%p)", t7 ? "PASS" : "FAIL", p7);
    heap_free(p7);
    heap_free(p6);

    // Test 8: Reserve with non-zero prefix split
    void *p8_base = heap_alloc(256);
    heap_free(p8_base);
    void *p8_target = (void *)((uint8_t *)p8_base + (HEAP_ALIGN * 4));
    void *p8 = heap_alloc_at(p8_target, 64);
    bool t8 = (p8 == p8_target);
    log("  alloc_at(split)  : %s (%p)", t8 ? "PASS" : "FAIL", p8);

    // Test 9: Allocation from prefix/suffix free space still works
    void *p9 = heap_alloc(32);
    bool t9 = (p9 != NULL) && (p9 != p8);
    log("  alloc around at  : %s (%p)", t9 ? "PASS" : "FAIL", p9);
    heap_free(p9);
    heap_free(p8);

    // Test 10: Test full free and heap state
    bool t10 = heap_base == (void *)align_up((uintptr_t)_heap_start)
        && heap_limit == (void *)align_down(mb2_info.mmap.max_mem)
        && ((struct block *)heap_base)->free == true
        && ((struct block *)heap_base)->next == NULL 
        && ((struct block *)heap_base)->prev == NULL;
    log("  heap reset       : %s", t10 ? "PASS" : "FAIL");
}
#endif

void heap_init(void)
{
    if (!mb2_info.mmap.count || !mb2_info.mmap.max_mem)
        PANIC("mb2 mmap info invalid");

    heap_base  = (void *)align_up((uintptr_t)_heap_start);
    heap_limit = (void *)align_down(mb2_info.mmap.max_mem);

    struct block *blk = (struct block *)heap_base;
    blk->free = true;
    blk->prev = NULL;
    blk->next = NULL;

    log("%s: base %p, limit %p, size %u KiB",
        __func__, heap_base, heap_limit, block_size(blk) / 1024);

#ifndef NDEBUG
    heap_selftest();
#endif
}

void heap_alloc_mb2_modules(void)
{
    uintptr_t heap_start = (uintptr_t)heap_base;
    uintptr_t heap_end = (uintptr_t)heap_limit;

    for (size_t i = 0; i < mb2_info.modules.count; i++) {
        const struct mb2_module_entry *module = &mb2_info.modules.entries[i];
        uintptr_t mod_start = module->start;
        uintptr_t mod_end = module->end;

        if (mod_end <= mod_start)
            PANIC("module[%u] has invalid range %p-%p",
                  (uint32_t)i, (void *)mod_start, (void *)mod_end);

        if (mod_end <= heap_start || mod_start >= heap_end) {
            log("%s: module[%u] %p-%p outside heap range %p-%p, skipping",
                __func__, (uint32_t)i, (void *)mod_start, (void *)mod_end,
                (void *)heap_start, (void *)heap_end);
            continue;
        }

        if (mod_start < heap_start)
            mod_start = heap_start;
        if (mod_end > heap_end)
            mod_end = heap_end;

        size_t module_size = (size_t)(mod_end - mod_start);
        heap_alloc_at((void *)mod_start, module_size);
    }

    log("%s: %d module(s) reserved", __func__, (int)mb2_info.modules.count);
}

void *heap_alloc(size_t size)
{
    if (size == 0)
        return NULL;

    size_t alloc_size = align_up(size);
    if (alloc_size < HEAP_MIN_ALLOC)
        alloc_size = HEAP_MIN_ALLOC;

    size_t total_size = sizeof(struct block) + alloc_size;

    struct block *blk = (struct block *)heap_base;
    while (blk != NULL) {
        size_t blk_size = block_size(blk);
        if (blk->free && blk_size >= total_size) {
            // Split block if remainder is large enough
            size_t remainder = blk_size - total_size;
            if (remainder >= sizeof(struct block) + HEAP_MIN_ALLOC) {
                struct block *new_blk = (struct block *)((uint8_t *)block_to_ptr(blk) + alloc_size);
                new_blk->free = true;
                new_blk->prev = blk;
                new_blk->next = blk->next;
                if (blk->next != NULL)
                    blk->next->prev = new_blk;
                blk->next = new_blk;
            }

            blk->free = false;
            return block_to_ptr(blk);
        }
        blk = blk->next;
    }

    PANIC("out of memory (requested %u bytes)", size);
    return NULL;
}

void *heap_alloc_at(void *ptr, size_t size)
{
    if (size == 0)
        return NULL;
    if (ptr == NULL)
        PANIC("ptr is NULL");

    uintptr_t req_ptr = (uintptr_t)ptr;
    if ((req_ptr % HEAP_ALIGN) != 0)
        PANIC("ptr %p is not %u-byte aligned", ptr, HEAP_ALIGN);

    size_t alloc_size = align_up(size);
    if (alloc_size < HEAP_MIN_ALLOC)
        alloc_size = HEAP_MIN_ALLOC;

    size_t total_size = sizeof(struct block) + alloc_size;
    uintptr_t req_start = req_ptr - sizeof(struct block);
    uintptr_t req_end = req_start + total_size;
    size_t min_split = sizeof(struct block) + HEAP_MIN_ALLOC;

    if (req_end < req_start)
        PANIC("requested range overflows at %p (size %u)", ptr, (uint32_t)size);

    uintptr_t heap_start = (uintptr_t)heap_base;
    uintptr_t heap_end = (uintptr_t)heap_limit;
    if (req_start < heap_start || req_end > heap_end)
        PANIC("requested range %p-%p is outside heap %p-%p",
              (void *)req_start, (void *)req_end, (void *)heap_start, (void *)heap_end);

    struct block *blk = (struct block *)heap_base;
    while (blk != NULL) {
        uintptr_t blk_start = (uintptr_t)blk;
        uintptr_t blk_end = blk_start + block_size(blk);
        if (!(blk_start <= req_start && req_end <= blk_end)) {
            blk = blk->next;
            continue;
        }

        if (!blk->free)
            PANIC("out of memory (requested %u bytes at %p)", size, ptr);

        size_t prefix = (size_t)(req_start - blk_start);
        if (prefix != 0 && prefix < min_split)
            PANIC("prefix fragment %u too small (min %u)",
                  prefix, min_split);

        struct block *alloc_blk = blk;
        if (prefix != 0) {
            struct block *old_next = blk->next;

            alloc_blk = (struct block *)req_start;
            alloc_blk->prev = blk;
            alloc_blk->next = old_next;
            if (old_next != NULL)
                old_next->prev = alloc_blk;
            blk->next = alloc_blk;
            blk->free = true;
        }

        size_t suffix = (size_t)(blk_end - req_end);
        if (suffix >= min_split) {
            struct block *suffix_blk = (struct block *)req_end;
            suffix_blk->free = true;
            suffix_blk->prev = alloc_blk;
            suffix_blk->next = alloc_blk->next;
            if (alloc_blk->next != NULL)
                alloc_blk->next->prev = suffix_blk;
            alloc_blk->next = suffix_blk;
        }

        alloc_blk->free = false;
        return block_to_ptr(alloc_blk);
    }

    PANIC("out of memory (requested %u bytes at %p)", size, ptr);
    return NULL;
}

void heap_free(void *ptr)
{
    if (ptr == NULL)
        return;

    struct block *blk = ptr_to_block(ptr);
    blk->free = true;

    // coalesce with next block (just unlink it)
    if (blk->next != NULL && blk->next->free) {
        blk->next = blk->next->next;
        if (blk->next != NULL)
            blk->next->prev = blk;
    }

    // coalesce with previous block (unlink current)
    if (blk->prev != NULL && blk->prev->free) {
        blk->prev->next = blk->next;
        if (blk->next != NULL)
            blk->next->prev = blk->prev;
    }
}

void heap_dump(void)
{
    size_t total_bytes = (size_t)((uintptr_t)heap_limit - (uintptr_t)heap_base);
    size_t used_bytes = 0;
    size_t unused_bytes = 0;

    for (struct block *blk = (struct block *)heap_base; blk != NULL; blk = blk->next) {
        size_t blk_bytes = block_size(blk);
        if (blk->free)
            unused_bytes += blk_bytes;
        else
            used_bytes += blk_bytes;
    }

    log("%s: total %zu KiB, used %zu KiB, unused %zu KiB",
        __func__,
        bytes_to_kib(total_bytes),
        bytes_to_kib(used_bytes),
        bytes_to_kib(unused_bytes));

    for (struct block *blk = (struct block *)heap_base; blk != NULL; blk = blk->next) {
        if (blk->free)
            continue;

        uintptr_t start = (uintptr_t)blk;
        size_t blk_bytes = block_size(blk);
        uintptr_t end = start + blk_bytes;
        log("   %p-%p, (%zu KiB)",
            (void *)start,
            (void *)end,
            bytes_to_kib(blk_bytes));
    }
}
