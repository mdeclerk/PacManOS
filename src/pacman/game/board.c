#include "pacman/game/board.h"
#include "pacman/assets/assets.h"
#include "stdlib/string.h"

static inline bool tile_walkable(uint8_t t)
{
    return !level_is_wall(t);
}

static inline bool tile_walkable_pacman(uint8_t t)
{
    return !level_is_wall(t) && t != LEVEL_TILE_GHOST_GATE;
}

static bool walkable_role(const struct board *self, vec_t tile, bool pacman_role)
{
    int r = grid_wrap(tile.row, LEVEL_ROWS);
    int c = grid_wrap(tile.col, LEVEL_COLS);
    bool (*walkable_fn)(uint8_t) = pacman_role ? tile_walkable_pacman : tile_walkable;

    if (!walkable_fn(self->tiles[r][c]))
        return false;

    if (r == 0 || r == LEVEL_ROWS - 1) {
        int opposite_r = (r == 0) ? (LEVEL_ROWS - 1) : 0;
        if (!walkable_fn(self->tiles[opposite_r][c]))
            return false;
    }

    if (c == 0 || c == LEVEL_COLS - 1) {
        int opposite_c = (c == 0) ? (LEVEL_COLS - 1) : 0;
        if (!walkable_fn(self->tiles[r][opposite_c]))
            return false;
    }

    return true;
}

void board_init(struct board *self, int level)
{
    const struct levels_entry *entry = &assets.levels->entries[level];
    memcpy(self->tiles, entry->tiles, sizeof(self->tiles));
    self->theme_color = entry->theme_color;
}

bool board_walkable(const struct board *self, vec_t tile, enum pathfinder_flags flags)
{
    return walkable_role(self, tile, flags == PF_WALLS_AND_GATES);
}

dir_t board_find_path(const struct board *self, vec_t start, vec_t target,
                      enum pathfinder_flags flags)
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
            if (visited[nr][nc] || !board_walkable(self, next, flags))
                continue;
            visited[nr][nc] = true;
            first_dir[nr][nc] = (first_dir[r][c] < 0) ? (int8_t)d : first_dir[r][c];
            if (nr == tr && nc == tc)
                return (dir_t)first_dir[nr][nc];
            qr[tail] = nr;
            qc[tail] = nc;
            tail++;
        }
    }

    return DIR_NONE;
}
