#include "wide_island_layout.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

int WideIslandLayout_ValidTiles(int tiles)
{
    return tiles == 0 || tiles == 8 || tiles == 16 || tiles == 32;
}

static float TileSize(float scale, float left, float right)
{
    return (right - left) * scale / WIDE_ISLAND_GRID_COLUMNS;
}

WideIslandLayout WideIslandLayout_Make(int tiles, float scale, float left, float right)
{
    WideIslandLayout layout = {0};
    float tile = TileSize(scale, left, right);
    if (!WideIslandLayout_ValidTiles(tiles) || tiles == 0 || tile <= 0) {
        layout.columns = WIDE_ISLAND_GRID_COLUMNS;
        return layout;
    }
    layout.tiles = tiles;
    layout.cutColumn = (int)lroundf(-left * scale / tile);
    layout.cut = (int)floorf(left * scale + layout.cutColumn * tile);
    layout.width = (int)lroundf(tiles * tile);
    layout.columns = WIDE_ISLAND_GRID_COLUMNS + tiles;
    return layout;
}

int WideIslandLayout_Shift(const WideIslandLayout *layout, int x)
{
    return layout->tiles && x > layout->cut ? x + layout->width : x;
}

int WideIslandLayout_Unshift(const WideIslandLayout *layout, int x)
{
    if (!layout->tiles || x <= layout->cut) return x;
    if (x > layout->cut + layout->width) return x - layout->width;
    return layout->cut;
}

float WideIslandLayout_Right(const WideIslandLayout *layout, float scale, float left, float right)
{
    if (!layout->tiles) return right;
    return right + layout->tiles * TileSize(scale, left, right) / scale;
}

void WideIslandLayout_InsertColumns(const WideIslandLayout *layout, unsigned char *map)
{
    if (!layout->tiles || layout->cutColumn < 1) return;
    for (int row = 0; row < WIDE_ISLAND_GRID_COLUMNS; row++) {
        unsigned char *line = map + row * WIDE_ISLAND_MAP_STRIDE;
        memmove(line + layout->cutColumn + layout->tiles, line + layout->cutColumn,
                WIDE_ISLAND_GRID_COLUMNS - layout->cutColumn);
        memset(line + layout->cutColumn, line[layout->cutColumn - 1], layout->tiles);
    }
}

void WideIslandLayout_Status(char *text, size_t size, int requested, int active)
{
    if (active && requested == active)
        snprintf(text, size, "Wider Resort Island: %d tiles on this course", active);
    else if (active && requested)
        snprintf(text, size, "Wider Resort Island: %d tiles on this course, %d from the next race", active, requested);
    else if (active)
        snprintf(text, size, "Wider Resort Island: %d tiles on this course, off from the next race", active);
    else if (requested)
        snprintf(text, size, "Wider Resort Island: %d tiles from the next Resort Island race", requested);
    else
        snprintf(text, size, "Wider Resort Island: off");
}
