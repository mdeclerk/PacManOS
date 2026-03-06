#include "pacman/game/pacman.h"

#include "pacman/assets/assets.h"
#include "pacman/game/board.h"
#include "engineos/include/log.h"


static struct pacman {
    vec_t tile;
    vec_t pos;
    dir_t dir;
    dir_t queued_dir;
    uint8_t anim_phase;
    uint32_t move_accum_ms;
    uint32_t anim_accum_px;
} pacman;

static bool can_go(vec_t tile, dir_t dir)
{
    vec_t next = grid_step(tile, dir, LEVEL_TILE_DIMS);
    return board_walkable_pacman(next);
}

static void try_turn(void)
{
    if (pacman.queued_dir != DIR_NONE && can_go(pacman.tile, pacman.queued_dir))
        pacman.dir = pacman.queued_dir;
    if (pacman.dir != DIR_NONE && !can_go(pacman.tile, pacman.dir))
        pacman.dir = DIR_NONE;
}

void pacman_init(int level)
{
    const struct levels_entry *entry = &assets.levels->entries[level];
    vec_t spawn = { 0 };
    int spawn_count = 0;

    for (int r = 0; r < LEVEL_ROWS; r++)
        for (int c = 0; c < LEVEL_COLS; c++)
            if (entry->tiles[r][c] == LEVEL_TILE_PACMAN_SPAWN) {
                spawn = (vec_t){ .row = r, .col = c };
                spawn_count++;
            }

    if (spawn_count != 1)
        PANIC("level %d must have exactly one pacman spawn", level);

    pacman = (struct pacman){
        .tile = spawn,
        .pos = { .x = spawn.col * LEVEL_TILE, .y = spawn.row * LEVEL_TILE },
    };
}

void pacman_queue_dir(dir_t dir)
{
    pacman.queued_dir = dir;
}

void pacman_step(uint32_t step_ms)
{
    uint32_t moved = 0;
    pacman.move_accum_ms += step_ms * LEVEL_TILE;
    uint32_t pixels = pacman.move_accum_ms / PACMAN_STEP_MS;
    pacman.move_accum_ms %= PACMAN_STEP_MS;

    for (uint32_t i = 0; i < pixels; i++) {
        if (grid_aligned_px(pacman.pos, LEVEL_TILE))
            try_turn();
        if (pacman.dir == DIR_NONE) break;
        grid_move_px(&pacman.pos, pacman.dir, LEVEL_PIXEL_DIMS);
        pacman.tile = grid_px_to_tile(pacman.pos, LEVEL_TILE, LEVEL_TILE_DIMS);
        moved++;
    }

    grid_advance_anim(&pacman.anim_accum_px, &pacman.anim_phase, moved, LEVEL_TILE / 4);
}

void pacman_render(vec_t origin)
{
    dir_t dir = (pacman.dir != DIR_NONE) ? pacman.dir : pacman.queued_dir;
    int col = pacman.anim_phase & 3;
    struct image fb = fb_subrect(&assets.pacman,
                                 col * PACMAN_SPRITE_SZ,
                                 dir_to_row(dir) * PACMAN_SPRITE_SZ,
                                 PACMAN_SPRITE_SZ, PACMAN_SPRITE_SZ);
    fb_blit(origin.x + pacman.pos.x - PACMAN_OFFSET,
            origin.y + pacman.pos.y - PACMAN_OFFSET, &fb, true, FB_WHITE);
}

vec_t pacman_tile(void)
{
    return pacman.tile;
}

vec_t pacman_pos(void)
{
    return pacman.pos;
}
