#ifndef WIDE_ISLAND_LAYOUT_H
#define WIDE_ISLAND_LAYOUT_H

#include <stddef.h>

#define WIDE_ISLAND_GRID_COLUMNS 128
#define WIDE_ISLAND_MAP_STRIDE 256

/* World X positions above cut move east by width, opening a strip of
 * `tiles` playfield columns. A zero layout leaves the course unchanged. */
typedef struct {
    int tiles;
    int cut;
    int width;
    int cutColumn;
    int columns;
} WideIslandLayout;

int WideIslandLayout_ValidTiles(int tiles);
WideIslandLayout WideIslandLayout_Make(int tiles, float scale, float left, float right);
int WideIslandLayout_Shift(const WideIslandLayout *layout, int x);
/* Maps a position on the widened course back to the stock course.
 * Positions inside the strip map to the cut. */
int WideIslandLayout_Unshift(const WideIslandLayout *layout, int x);
float WideIslandLayout_Right(const WideIslandLayout *layout, float scale, float left, float right);
/* Repeats the column west of the cut across the strip in each of the 128 rows. */
void WideIslandLayout_InsertColumns(const WideIslandLayout *layout, unsigned char *map);
/* Describes the width on the loaded course and the width requested for the next Resort Island race. */
void WideIslandLayout_Status(char *text, size_t size, int requested, int active);

#endif
