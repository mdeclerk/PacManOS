#include "engineos/kernel/mb2.h"
#include "engineos/kernel/fb.h"
#include "engineos/kernel/heap.h"
#include "engineos/kernel/log.h"
#include "engineos/kernel/timer.h"
#include "engineos/engine/game.h"
#include "engineos/engine/elf.h"
#include "engineos/engine/event.h"
#include "engineos/engine/screen.h"

#define FIXED_STEP_MS 16u
#define FPS_INTERVAL_MS 1000u

typedef struct screen *(*game_entry_fn)(void);

static void game_init(void) 
{
    const struct mb2_module_entry *mod = mb2_find_module_by_magic("\x7F" "ELF", 4);
    if (!mod)
        PANIC("no game module found");

    struct elf_image *elf = elf_load(
        (const void *)mod->start, (size_t)(mod->end - mod->start));

    if (elf->load_start == 0u || elf->load_end <= elf->load_start)
        PANIC("failed to load game module");

    game_entry_fn game_entry = (game_entry_fn)elf_symbol(elf, "game_entry");
    if (!game_entry)
        PANIC("missing or invalid game_entry");

    elf_free(elf);

    struct screen *root = game_entry();
    if (!root)
        PANIC("no root screen");

    screen_init(root);

    log("%s: image %p-%p (%u KiB)", __func__,
        (void *)elf->load_start, (void *)elf->load_end,
        (uint32_t)((elf->load_end - elf->load_start) / 1024u));
}

static inline void game_on_event(const struct event *e)
{
    if (e->type == ETYPE_KEY
        && e->key.type == KEYEVENT_TYPE_PRESS
        && e->key.keycode == KEYCODE_F10) {
        heap_dump();
    }

    struct screen *s = screen_get_active();
    if (s->on_event)
        s->on_event(e);
}

static inline void game_tick(uint32_t dt_ms)
{
    struct screen *s = screen_get_active();
    if (s->tick)
        s->tick(dt_ms);
}

static inline void game_draw(uint32_t fps)
{
    struct screen *s = screen_get_active();
    if (s->draw)
        s->draw(fps);
}

void game_run(void)
{
    uint32_t prev = timer_get_ticks();
    uint32_t lag = 0u;
    uint32_t fps = 0u;
    uint32_t frames = 0u;
    uint32_t fps_tick = prev;

    game_init();

    log("%s: entering main loop", __func__);

    for (;;) {
        uint32_t now = timer_get_ticks();
        lag += (now - prev) * timer_get_interval_ms();
        prev = now;

        for (; lag >= FIXED_STEP_MS; lag -= FIXED_STEP_MS) {
            struct event e;
            while (event_dequeue(&e))
                game_on_event(&e);

            game_tick(FIXED_STEP_MS);
        }

        game_draw(fps);
        fb_show();

        frames++;
        uint32_t elapsed = (now - fps_tick) * timer_get_interval_ms();
        if (elapsed >= FPS_INTERVAL_MS) {
            fps = frames * 1000u / elapsed;
            frames = 0u;
            fps_tick = now;
        }
    }
}
