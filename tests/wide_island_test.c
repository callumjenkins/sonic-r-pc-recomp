#include "wide_island_layout.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static const float A = 25.0f, B = -476.802490234375f, D = 631.408447265625f;

static void island_cut_sits_on_the_column_boundary_nearest_zero(void)
{
    WideIslandLayout layout = WideIslandLayout_Make(16, A, B, D);
    assert(layout.tiles == 16);
    assert(layout.cutColumn == 55);
    assert(layout.cut == -16);
    assert(layout.width == 3463);
    assert(layout.columns == 144);
}

static void shift_moves_only_points_east_of_the_cut(void)
{
    WideIslandLayout layout = WideIslandLayout_Make(16, A, B, D);
    assert(WideIslandLayout_Shift(&layout, -9000) == -9000);
    assert(WideIslandLayout_Shift(&layout, -16) == -16);
    assert(WideIslandLayout_Shift(&layout, -15) == -15 + 3463);
    assert(WideIslandLayout_Shift(&layout, 9000) == 9000 + 3463);
}

static void unshift_returns_strip_positions_to_the_cut(void)
{
    WideIslandLayout layout = WideIslandLayout_Make(16, A, B, D);
    assert(WideIslandLayout_Unshift(&layout, -9000) == -9000);
    assert(WideIslandLayout_Unshift(&layout, -16) == -16);
    assert(WideIslandLayout_Unshift(&layout, -15) == -16);
    assert(WideIslandLayout_Unshift(&layout, -16 + 3463) == -16);
    assert(WideIslandLayout_Unshift(&layout, -15 + 3463) == -15);
    for (int x = -10000; x < 10000; x += 7)
        assert(WideIslandLayout_Unshift(&layout, WideIslandLayout_Shift(&layout, x)) == x);
}

static void off_and_invalid_counts_leave_the_course_alone(void)
{
    for (int tiles = -1; tiles <= 64; tiles++) {
        if (tiles == 8 || tiles == 16 || tiles == 32) continue;
        WideIslandLayout layout = WideIslandLayout_Make(tiles, A, B, D);
        assert(layout.tiles == 0 && layout.columns == 128);
        assert(WideIslandLayout_Shift(&layout, 5000) == 5000);
        assert(WideIslandLayout_Unshift(&layout, 5000) == 5000);
        assert(WideIslandLayout_Right(&layout, A, B, D) == D);
    }
    assert(!WideIslandLayout_ValidTiles(64) && WideIslandLayout_ValidTiles(0));
}

static void widened_bounds_keep_the_tile_size(void)
{
    for (int tiles = 8; tiles <= 32; tiles *= 2) {
        WideIslandLayout layout = WideIslandLayout_Make(tiles, A, B, D);
        float right = WideIslandLayout_Right(&layout, A, B, D);
        float before = (D - B) * A / 128;
        float after = (right - B) * A / layout.columns;
        assert(fabsf(before - after) < 0.01f);
        assert(fabsf(layout.width - tiles * before) < 1.0f);
    }
    assert(WideIslandLayout_Make(8, A, B, D).width == 1732);
    assert(WideIslandLayout_Make(32, A, B, D).width == 6926);
}

static void degenerate_bounds_leave_the_course_alone(void)
{
    WideIslandLayout layout = WideIslandLayout_Make(16, A, B, B);
    assert(layout.tiles == 0 && layout.columns == 128);
}

static void status_names_the_course_and_the_next_race(void)
{
    char text[128];
    WideIslandLayout_Status(text, sizeof(text), 0, 0);
    assert(strcmp(text, "Wider Resort Island: off") == 0);
    WideIslandLayout_Status(text, sizeof(text), 16, 0);
    assert(strcmp(text, "Wider Resort Island: 16 tiles from the next Resort Island race") == 0);
    WideIslandLayout_Status(text, sizeof(text), 16, 16);
    assert(strcmp(text, "Wider Resort Island: 16 tiles on this course") == 0);
    WideIslandLayout_Status(text, sizeof(text), 32, 16);
    assert(strcmp(text, "Wider Resort Island: 16 tiles on this course, 32 from the next race") == 0);
    WideIslandLayout_Status(text, sizeof(text), 0, 16);
    assert(strcmp(text, "Wider Resort Island: 16 tiles on this course, off from the next race") == 0);
}

static void widest_strip_keeps_island_terrain_inside_int16(void)
{
    WideIslandLayout layout = WideIslandLayout_Make(32, A, B, D);
    assert(WideIslandLayout_Shift(&layout, 9534) == 16460);
    assert(WideIslandLayout_Shift(&layout, 11160) <= 32767);
    assert(layout.columns <= WIDE_ISLAND_MAP_STRIDE);
}

static void strip_columns_repeat_the_column_west_of_the_cut(void)
{
    static unsigned char map[WIDE_ISLAND_MAP_STRIDE * WIDE_ISLAND_MAP_STRIDE];
    for (int row = 0; row < 128; row++)
        for (int col = 0; col < 128; col++)
            map[row * 256 + col] = (unsigned char)(col + row);
    WideIslandLayout layout = WideIslandLayout_Make(16, A, B, D);
    WideIslandLayout_InsertColumns(&layout, map);
    for (int row = 0; row < 128; row++) {
        unsigned char *line = map + row * 256;
        assert(line[54] == (unsigned char)(54 + row));
        for (int col = 55; col < 71; col++) assert(line[col] == (unsigned char)(54 + row));
        assert(line[71] == (unsigned char)(55 + row));
        assert(line[143] == (unsigned char)(127 + row));
    }
}

int main(void)
{
    island_cut_sits_on_the_column_boundary_nearest_zero();
    shift_moves_only_points_east_of_the_cut();
    unshift_returns_strip_positions_to_the_cut();
    off_and_invalid_counts_leave_the_course_alone();
    widened_bounds_keep_the_tile_size();
    degenerate_bounds_leave_the_course_alone();
    status_names_the_course_and_the_next_race();
    widest_strip_keeps_island_terrain_inside_int16();
    strip_columns_repeat_the_column_west_of_the_cut();
    puts("wide island layout passed");
}
