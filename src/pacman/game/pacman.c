#include "pacman/game/pacman.h"

#include "pacman/assets/assets.h"
#include "pacman/game/board.h"
#include "engineos/include/log.h"

static bool can_go(const struct board *board, vec_t tile, dir_t dir)
{
    vec_t next = grid_step(tile, dir, LEVEL_TILE_DIMS);
    return board_walkable(board, next, PF_WALLS_AND_GATES);
}

static void try_turn(struct pacman *self, const struct board *board)
{
    if (self->queued_dir != DIR_NONE && can_go(board, self->tile, self->queued_dir))
        self->dir = self->queued_dir;
    if (self->dir != DIR_NONE && !can_go(board, self->tile, self->dir))
        self->dir = DIR_NONE;
}

void pacman_init(struct pacman *self, int level)
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

    *self = (struct pacman){
        .tile = spawn,
        .pos = { .x = spawn.col * LEVEL_TILE, .y = spawn.row * LEVEL_TILE },
    };
}

void pacman_queue_dir(struct pacman *self, dir_t dir)
{
    self->queued_dir = dir;
}

void pacman_step(struct pacman *self, const struct board *board, uint32_t step_ms)
{
    uint32_t moved = 0;
    self->move_accum_ms += step_ms * LEVEL_TILE;
    uint32_t pixels = self->move_accum_ms / PACMAN_STEP_MS;
    self->move_accum_ms %= PACMAN_STEP_MS;

    for (uint32_t i = 0; i < pixels; i++) {
        if (grid_aligned_px(self->pos, LEVEL_TILE))
            try_turn(self, board);
        if (self->dir == DIR_NONE) break;
        grid_move_px(&self->pos, self->dir, LEVEL_PIXEL_DIMS);
        self->tile = grid_px_to_tile(self->pos, LEVEL_TILE, LEVEL_TILE_DIMS);
        moved++;
    }

    grid_advance_anim(&self->anim_accum_px, &self->anim_phase, moved, LEVEL_TILE / 4);
}
