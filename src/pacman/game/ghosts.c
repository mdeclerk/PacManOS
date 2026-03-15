#include "pacman/game/ghosts.h"

#include "pacman/assets/assets.h"
#include "pacman/game/pacman.h"
#include "pacman/game/board.h"
#include "engineos/include/log.h"
#include "stdlib/string.h"

#define COLLISION_HALF ((PACMAN_SPRITE_SZ + GHOST_SPRITE_SZ) / 4)

static bool is_gate_tile(const struct ghost_map *map, vec_t tile)
{
    int r = grid_wrap(tile.row, LEVEL_ROWS);
    int c = grid_wrap(tile.col, LEVEL_COLS);
    return map->gate_cells[r][c] != 0;
}

static void parse_level_topology(struct ghost_map *map, const struct board *board,
                                 int level, const struct levels_entry *entry)
{
    vec_t spawn = { 0 };
    int spawn_count = 0;

    int gate_row = -1;
    int gate_min_col = LEVEL_COLS;
    int gate_max_col = -1;
    int gate_count = 0;

    memset(map->gate_cells, 0, sizeof(map->gate_cells));

    for (int r = 0; r < LEVEL_ROWS; r++)
        for (int c = 0; c < LEVEL_COLS; c++) {
            uint8_t t = entry->tiles[r][c];
            if (t == LEVEL_TILE_GHOST_SPAWN) {
                spawn = (vec_t){ .row = r, .col = c };
                spawn_count++;
            }
            if (t == LEVEL_TILE_GHOST_GATE) {
                map->gate_cells[r][c] = 1;
                if (gate_row < 0)
                    gate_row = r;
                else if (gate_row != r)
                    PANIC("level %d ghost gate must be on a single row", level);

                if (c < gate_min_col) gate_min_col = c;
                if (c > gate_max_col) gate_max_col = c;
                gate_count++;
            }
        }

    if (spawn_count != 1)
        PANIC("level %d must have exactly one ghost spawn", level);
    if (gate_count == 0)
        PANIC("level %d must include a ghost gate", level);

    int gate_span = gate_max_col - gate_min_col + 1;
    int gate_center = gate_min_col + gate_span / 2;
    vec_t exit = { .row = grid_wrap(gate_row - 1, LEVEL_ROWS), .col = gate_center };

    if (!board_walkable_ghost(board, exit) || is_gate_tile(map, exit))
        PANIC("level %d has invalid ghost gate exit", level);

    map->spawn = spawn;
    map->gate_exit_tile = exit;
}

static uint32_t rng_next(struct ghost *g)
{
    g->rng = g->rng * 1664525u + 1013904223u;
    return g->rng;
}

static bool can_step(const struct board *board, const struct ghost_map *map,
                     ghost_mode_t mode, vec_t tile)
{
    if (!board_walkable_ghost(board, tile))
        return false;

    if (!is_gate_tile(map, tile))
        return true;

    return mode == GHOST_EYES || mode == GHOST_EXITING;
}

static uint32_t ghost_step_ms(ghost_mode_t mode)
{
    return mode == GHOST_FRIGHTENED ? GHOST_STEP_FRIGHTENED
         : mode == GHOST_EYES       ? GHOST_STEP_EYES
         :                            GHOST_STEP_NORMAL;
}

static dir_t bfs_to(const struct board *board, const struct ghost_map *map,
                    vec_t start, vec_t target, ghost_mode_t mode)
{
    bool visited[LEVEL_ROWS][LEVEL_COLS];
    int8_t first_dir[LEVEL_ROWS][LEVEL_COLS];
    int qr[LEVEL_ROWS * LEVEL_COLS], qc[LEVEL_ROWS * LEVEL_COLS];
    int head = 0, tail = 0;

    memset(visited, 0, sizeof(visited));
    memset(first_dir, -1, sizeof(first_dir));

    int sr = grid_wrap(start.row, LEVEL_ROWS);
    int sc = grid_wrap(start.col, LEVEL_COLS);
    int tr = grid_wrap(target.row, LEVEL_ROWS);
    int tc = grid_wrap(target.col, LEVEL_COLS);

    visited[sr][sc] = true;
    qr[tail] = sr;
    qc[tail] = sc;
    tail++;

    while (head < tail) {
        int r = qr[head], c = qc[head];
        head++;

        for (int d = 0; d < 4; d++) {
            vec_t next = grid_step((vec_t){ .row = r, .col = c }, (dir_t)d, LEVEL_TILE_DIMS);
            int nr = next.row;
            int nc = next.col;
            if (visited[nr][nc] || !can_step(board, map, mode, next)) continue;
            visited[nr][nc] = true;
            first_dir[nr][nc] = (first_dir[r][c] < 0) ? (int8_t)d : first_dir[r][c];
            if (nr == tr && nc == tc) return (dir_t)first_dir[nr][nc];
            qr[tail] = nr;
            qc[tail] = nc;
            tail++;
        }
    }

    return DIR_NONE;
}

static dir_t pick_dir(struct ghost *g, const struct board *board,
                      const struct ghost_map *map)
{
    dir_t opts[4], fallback = DIR_NONE;
    int n = 0, total = 0;
    dir_t rev = (g->dir != DIR_NONE) ? dir_opposite(g->dir) : DIR_NONE;
    bool can_continue = false;

    for (int d = 0; d < 4; d++) {
        dir_t dir = (dir_t)d;
        vec_t next = grid_step(g->tile, dir, LEVEL_TILE_DIMS);
        if (!can_step(board, map, g->mode, next)) continue;
        total++;
        if (dir == g->dir) can_continue = true;
        if (dir == rev) {
            fallback = dir;
            continue;
        }
        opts[n++] = dir;
    }

    if (can_continue && total < 3) return g->dir;
    if (n == 0) return fallback;
    return opts[rng_next(g) % (uint32_t)n];
}

static void update_at_center(struct ghost *g, const struct board *board,
                             const struct ghost_map *map, bool power_active)
{
    vec_t spawn = map->spawn;
    vec_t exit = map->gate_exit_tile;

    if (g->mode == GHOST_EYES && g->tile.row == spawn.row && g->tile.col == spawn.col) {
        g->mode = GHOST_RESPAWN_WAIT;
        g->respawn_ms = GHOST_RESPAWN_MS;
        g->dir = DIR_NONE;
        return;
    }

    if (g->mode == GHOST_EXITING && g->tile.row == exit.row && g->tile.col == exit.col)
        g->mode = power_active ? GHOST_FRIGHTENED : GHOST_NORMAL;

    dir_t next;
    if (g->mode == GHOST_EYES)
        next = bfs_to(board, map, g->tile, spawn, GHOST_EYES);
    else if (g->mode == GHOST_EXITING)
        next = bfs_to(board, map, g->tile, exit, GHOST_EXITING);
    else
        next = pick_dir(g, board, map);

    if (next != DIR_NONE) g->dir = next;
}

static void ghost_reset_one(struct ghost *g, const struct board *board,
                            const struct ghost_map *map)
{
    static const int dr[] = { 0, -1, 0, 0 };
    static const int dc[] = { 0, 0, -1, 1 };

    vec_t spawn = map->spawn;
    int id = g->id;
    uint32_t rng = g->rng;

    vec_t pos = {
        .row = grid_wrap(spawn.row + dr[id % 4], LEVEL_ROWS),
        .col = grid_wrap(spawn.col + dc[id % 4], LEVEL_COLS),
    };
    if (!can_step(board, map, GHOST_EXITING, pos))
        pos = spawn;

    *g = (struct ghost){
        .id = id,
        .tile = pos,
        .pos = { .x = pos.col * LEVEL_TILE, .y = pos.row * LEVEL_TILE },
        .dir = (id & 1) ? DIR_LEFT : DIR_RIGHT,
        .mode = GHOST_EXITING,
        .anim_phase = (uint8_t)id,
        .rng = rng,
    };
}

static void ghost_update(struct ghost *g, const struct board *board,
                         const struct ghost_map *map,
                         uint32_t dt_ms, bool power_active)
{
    uint32_t moved = 0;

    if (g->mode == GHOST_RESPAWN_WAIT) {
        if (dt_ms >= g->respawn_ms) {
            g->respawn_ms = 0;
            g->mode = GHOST_EXITING;
        } else {
            g->respawn_ms -= dt_ms;
        }
    }

    if (grid_aligned_px(g->pos, LEVEL_TILE))
        update_at_center(g, board, map, power_active);

    g->move_accum_ms += dt_ms * LEVEL_TILE;
    for (;;) {
        uint32_t ms = ghost_step_ms(g->mode);
        if (g->move_accum_ms < ms) break;
        g->move_accum_ms -= ms;

        if (grid_aligned_px(g->pos, LEVEL_TILE))
            update_at_center(g, board, map, power_active);
        if (g->dir == DIR_NONE) break;

        grid_move_px(&g->pos, g->dir, LEVEL_PIXEL_DIMS);
        g->tile = grid_px_to_tile(g->pos, LEVEL_TILE, LEVEL_TILE_DIMS);
        moved++;
    }

    grid_advance_anim(&g->anim_accum_px, &g->anim_phase, moved, (LEVEL_TILE * 2) / 3);
}

static void ghost_set_eaten(struct ghost *g)
{
    g->mode = GHOST_EYES;
    g->dir = DIR_NONE;
    g->move_accum_ms = 0;
    g->pos.x = g->tile.col * LEVEL_TILE;
    g->pos.y = g->tile.row * LEVEL_TILE;
}

static bool ghost_overlaps_pacman(const struct ghost *g, vec_t pac_pos)
{
    return grid_overlap_wrap_px(g->pos, pac_pos, LEVEL_PIXEL_DIMS, COLLISION_HALF);
}

void ghosts_init(struct ghosts *self, const struct board *board,
                 uint32_t seed, int level)
{
    const struct levels_entry *entry = &assets.levels->entries[level];

    memset(self, 0, sizeof(*self));
    parse_level_topology(&self->map, board, level, entry);

    for (int i = 0; i < GHOST_COUNT; i++) {
        self->entries[i].id = i;
        self->entries[i].rng = seed ^ (0x9E3779B9u * (uint32_t)(i + 1));
    }
}

void ghosts_reset(struct ghosts *self, const struct board *board)
{
    for (int i = 0; i < GHOST_COUNT; i++)
        ghost_reset_one(&self->entries[i], board, &self->map);
}

void ghosts_set_mode(struct ghosts *self, ghost_mode_t from, ghost_mode_t to)
{
    for (int i = 0; i < GHOST_COUNT; i++)
        if (self->entries[i].mode == from)
            self->entries[i].mode = to;
}

void ghosts_step(struct ghosts *self, const struct board *board,
                 uint32_t step_ms, bool power_active)
{
    for (int i = 0; i < GHOST_COUNT; i++)
        ghost_update(&self->entries[i], board, &self->map, step_ms, power_active);
}

int ghosts_collide_pacman(struct ghosts *self, vec_t pac_pos)
{
    for (int i = 0; i < GHOST_COUNT; i++) {
        struct ghost *g = &self->entries[i];
        if (!ghost_overlaps_pacman(g, pac_pos))
            continue;
        if (g->mode == GHOST_NORMAL || g->mode == GHOST_EXITING)
            return -1;
    }

    int eaten = 0;
    for (int i = 0; i < GHOST_COUNT; i++) {
        struct ghost *g = &self->entries[i];
        if (!ghost_overlaps_pacman(g, pac_pos))
            continue;
        if (g->mode == GHOST_FRIGHTENED) {
            ghost_set_eaten(g);
            eaten++;
        }
    }

    return eaten;
}
