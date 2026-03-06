#include "pacman/game/ghosts.h"

#include "pacman/assets/assets.h"
#include "pacman/game/pacman.h"
#include "pacman/game/board.h"
#include "engineos/include/log.h"
#include "stdlib/string.h"

#define BLINK_MS         3000u
#define BLINK_STEP_MS     200u
#define ATLAS_ROW_FRIGHT    4
#define ATLAS_ROW_EYES      5
#define ATLAS_WALK_STRIDE   4
#define COLLISION_HALF ((PACMAN_SPRITE_SZ + GHOST_SPRITE_SZ) / 4)

static struct ghost {
    vec_t tile;
    vec_t pos;
    dir_t dir;
    ghost_mode_t mode;
    uint32_t rng;
    uint32_t move_accum_ms;
    uint32_t anim_accum_px;
    uint32_t respawn_ms;
    uint8_t anim_phase;
    int id;
} ghosts[GHOST_COUNT];

static struct {
    vec_t spawn;
    vec_t gate_exit_tile;
    uint8_t gate_cells[LEVEL_ROWS][LEVEL_COLS];
} ghost_map;

static struct image atlas;

static bool is_gate_tile(vec_t tile)
{
    int r = grid_wrap(tile.row, LEVEL_ROWS);
    int c = grid_wrap(tile.col, LEVEL_COLS);
    return ghost_map.gate_cells[r][c] != 0;
}

static void parse_level_topology(int level, const struct levels_entry *entry)
{
    vec_t spawn = { 0 };
    int spawn_count = 0;

    int gate_row = -1;
    int gate_min_col = LEVEL_COLS;
    int gate_max_col = -1;
    int gate_count = 0;

    memset(ghost_map.gate_cells, 0, sizeof(ghost_map.gate_cells));

    for (int r = 0; r < LEVEL_ROWS; r++)
        for (int c = 0; c < LEVEL_COLS; c++) {
            uint8_t t = entry->tiles[r][c];
            if (t == LEVEL_TILE_GHOST_SPAWN) {
                spawn = (vec_t){ .row = r, .col = c };
                spawn_count++;
            }
            if (t == LEVEL_TILE_GHOST_GATE) {
                ghost_map.gate_cells[r][c] = 1;
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

    if (!board_walkable_ghost(exit) || is_gate_tile(exit))
        PANIC("level %d has invalid ghost gate exit", level);

    ghost_map.spawn = spawn;
    ghost_map.gate_exit_tile = exit;
}

static uint32_t rng_next(struct ghost *g)
{
    g->rng = g->rng * 1664525u + 1013904223u;
    return g->rng;
}

static bool can_step(ghost_mode_t mode, vec_t tile)
{
    if (!board_walkable_ghost(tile))
        return false;

    if (!is_gate_tile(tile))
        return true;

    return mode == GHOST_EYES || mode == GHOST_EXITING;
}

static uint32_t step_ms(ghost_mode_t mode)
{
    return mode == GHOST_FRIGHTENED ? GHOST_STEP_FRIGHTENED
         : mode == GHOST_EYES       ? GHOST_STEP_EYES
         :                            GHOST_STEP_NORMAL;
}

static dir_t bfs_to(vec_t start, vec_t target, ghost_mode_t mode)
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
            if (visited[nr][nc] || !can_step(mode, next)) continue;
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

static dir_t pick_dir(struct ghost *g)
{
    dir_t opts[4], fallback = DIR_NONE;
    int n = 0, total = 0;
    dir_t rev = (g->dir != DIR_NONE) ? dir_opposite(g->dir) : DIR_NONE;
    bool can_continue = false;

    for (int d = 0; d < 4; d++) {
        dir_t dir = (dir_t)d;
        vec_t next = grid_step(g->tile, dir, LEVEL_TILE_DIMS);
        if (!can_step(g->mode, next)) continue;
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

static void update_at_center(struct ghost *g, bool power_active)
{
    vec_t spawn = ghost_map.spawn;
    vec_t exit = ghost_map.gate_exit_tile;

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
        next = bfs_to(g->tile, spawn, GHOST_EYES);
    else if (g->mode == GHOST_EXITING)
        next = bfs_to(g->tile, exit, GHOST_EXITING);
    else
        next = pick_dir(g);

    if (next != DIR_NONE) g->dir = next;
}

static void ghost_reset(struct ghost *g)
{
    static const int dr[] = { 0, -1, 0, 0 };
    static const int dc[] = { 0, 0, -1, 1 };

    vec_t spawn = ghost_map.spawn;
    int id = g->id;
    uint32_t rng = g->rng;

    vec_t pos = {
        .row = grid_wrap(spawn.row + dr[id % 4], LEVEL_ROWS),
        .col = grid_wrap(spawn.col + dc[id % 4], LEVEL_COLS),
    };
    if (!can_step(GHOST_EXITING, pos))
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

static void ghost_update(struct ghost *g, uint32_t dt_ms, bool power_active)
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
        update_at_center(g, power_active);

    g->move_accum_ms += dt_ms * LEVEL_TILE;
    for (;;) {
        uint32_t ms = step_ms(g->mode);
        if (g->move_accum_ms < ms) break;
        g->move_accum_ms -= ms;

        if (grid_aligned_px(g->pos, LEVEL_TILE))
            update_at_center(g, power_active);
        if (g->dir == DIR_NONE) break;

        grid_move_px(&g->pos, g->dir, LEVEL_PIXEL_DIMS);
        g->tile = grid_px_to_tile(g->pos, LEVEL_TILE, LEVEL_TILE_DIMS);
        moved++;
    }

    grid_advance_anim(&g->anim_accum_px, &g->anim_phase, moved, (LEVEL_TILE * 2) / 3);
}

static void ghost_render(const struct ghost *g, bool blink_normal, vec_t origin)
{
    bool eyes = g->mode == GHOST_EYES || g->mode == GHOST_RESPAWN_WAIT;
    int row = eyes ? ATLAS_ROW_EYES
            : (g->mode == GHOST_FRIGHTENED && !blink_normal) ? ATLAS_ROW_FRIGHT
            : g->id % GHOST_COUNT;
    static const int dir_col[] = { 2, 3, 0, 1, 1 };
    int col = dir_col[g->dir];
    if (!eyes && (g->anim_phase & 1))
        col += ATLAS_WALK_STRIDE;

    struct image fb = fb_subrect(&atlas,
                                 col * GHOST_SPRITE_SZ,
                                 row * GHOST_SPRITE_SZ,
                                 GHOST_SPRITE_SZ,
                                 GHOST_SPRITE_SZ);
    fb_blit(origin.x + g->pos.x - GHOST_OFFSET,
            origin.y + g->pos.y - GHOST_OFFSET, &fb, true, FB_WHITE);
}

static void ghost_set_eaten(struct ghost *g)
{
    g->mode = GHOST_EYES;
    g->dir = DIR_NONE;
    g->move_accum_ms = 0;
    g->pos.x = g->tile.col * LEVEL_TILE;
    g->pos.y = g->tile.row * LEVEL_TILE;
}

static bool ghost_overlaps_pacman(const struct ghost *g, vec_t pacman_pos)
{
    return grid_overlap_wrap_px(g->pos, pacman_pos, LEVEL_PIXEL_DIMS, COLLISION_HALF);
}

void ghosts_init(uint32_t seed, int level)
{
    atlas = assets.ghosts;
    const struct levels_entry *entry = &assets.levels->entries[level];
    parse_level_topology(level, entry);

    memset(&ghosts, 0, sizeof(ghosts));
    for (int i = 0; i < GHOST_COUNT; i++) {
        ghosts[i].id = i;
        ghosts[i].rng = seed ^ (0x9E3779B9u * (uint32_t)(i + 1));
    }
}

void ghosts_reset(void)
{
    for (int i = 0; i < GHOST_COUNT; i++)
        ghost_reset(&ghosts[i]);
}

void ghosts_set_mode(ghost_mode_t from, ghost_mode_t to)
{
    for (int i = 0; i < GHOST_COUNT; i++)
        if (ghosts[i].mode == from)
            ghosts[i].mode = to;
}

void ghosts_step(uint32_t step_ms, bool power_active)
{
    for (int i = 0; i < GHOST_COUNT; i++)
        ghost_update(&ghosts[i], step_ms, power_active);
}

void ghosts_render(uint32_t power_ms_left, vec_t origin)
{
    bool blink = power_ms_left > 0 && power_ms_left <= BLINK_MS
                 && ((power_ms_left / BLINK_STEP_MS) & 1u);
    for (int i = 0; i < GHOST_COUNT; i++)
        ghost_render(&ghosts[i], blink, origin);
}

int ghosts_collide_pacman(vec_t pacman_pos)
{
    for (int i = 0; i < GHOST_COUNT; i++) {
        struct ghost *g = &ghosts[i];
        if (!ghost_overlaps_pacman(g, pacman_pos))
            continue;
        if (g->mode == GHOST_NORMAL || g->mode == GHOST_EXITING)
            return -1;
    }

    int eaten = 0;
    for (int i = 0; i < GHOST_COUNT; i++) {
        struct ghost *g = &ghosts[i];
        if (!ghost_overlaps_pacman(g, pacman_pos))
            continue;
        if (g->mode == GHOST_FRIGHTENED) {
            ghost_set_eaten(g);
            eaten++;
        }
    }

    return eaten;
}
