#include "hd_textures_format.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void maps_a_stock_page_path_to_its_path_in_the_pack(void)
{
    char path[256];
    assert(HdTexturesFormat_Path(path, sizeof(path), "Upscaled", "./ISLAND/ISLAND00.RAW", -1));
    assert(strcmp(path, "HD/Upscaled/ISLAND/ISLAND00.RGBA") == 0);
    assert(HdTexturesFormat_Path(path, sizeof(path), "Upscaled", "./ISLAND/SNOW/ISL_S_04.RAW", -1));
    assert(strcmp(path, "HD/Upscaled/ISLAND/SNOW/ISL_S_04.RGBA") == 0);
    assert(HdTexturesFormat_Path(path, sizeof(path), "4x UltraSharp", "GENERAL/PLAYER00.RAW", -1));
    assert(strcmp(path, "HD/4x UltraSharp/GENERAL/PLAYER00.RGBA") == 0);
}

static void maps_each_playfield_page_to_its_own_file(void)
{
    char path[256];
    assert(HdTexturesFormat_Path(path, sizeof(path), "Upscaled", "./ISLAND/ISLAND.PLY", 0));
    assert(strcmp(path, "HD/Upscaled/ISLAND/ISLAND.PLY.0.RGBA") == 0);
    assert(HdTexturesFormat_Path(path, sizeof(path), "Upscaled", "./ISLAND/SNOW/ISLAND_S.PLY", 1));
    assert(strcmp(path, "HD/Upscaled/ISLAND/SNOW/ISLAND_S.PLY.1.RGBA") == 0);
}

static void rejects_stock_paths_it_cannot_map(void)
{
    char path[256] = "untouched";
    const char *full = "HD/Upscaled/ISLAND/ISLAND00.RGBA";
    assert(!HdTexturesFormat_Path(path, sizeof(path), "Upscaled", NULL, -1));
    assert(!HdTexturesFormat_Path(path, sizeof(path), "Upscaled", "", -1));
    assert(!HdTexturesFormat_Path(path, sizeof(path), "Upscaled", "/data/ISLAND/ISLAND00.RAW", -1));
    assert(!HdTexturesFormat_Path(path, sizeof(path), "Upscaled", "../ISLAND/ISLAND00.RAW", -1));
    assert(!HdTexturesFormat_Path(path, sizeof(path), "Upscaled", "./ISLAND00.RAW", -1));
    assert(!HdTexturesFormat_Path(path, sizeof(path), "Upscaled", "./ISLAND/ISLAND00", -1));
    assert(!HdTexturesFormat_Path(path, sizeof(path), "Upscaled", "./IS.LAND/ISLAND00", -1));
    assert(!HdTexturesFormat_Path(path, strlen(full), "Upscaled", "./ISLAND/ISLAND00.RAW", -1));
    assert(HdTexturesFormat_Path(path, strlen(full) + 1, "Upscaled", "./ISLAND/ISLAND00.RAW", -1));
}

static void a_pack_name_is_one_folder_inside_hd(void)
{
    char path[256];
    char longest[HD_TEXTURES_PACK_MAX];
    memset(longest, 'x', sizeof(longest) - 1);
    longest[sizeof(longest) - 1] = '\0';
    assert(HdTexturesFormat_PackName("Upscaled"));
    assert(HdTexturesFormat_PackName("4x UltraSharp"));
    assert(HdTexturesFormat_PackName(longest));
    assert(!HdTexturesFormat_PackName(NULL));
    assert(!HdTexturesFormat_PackName(""));
    assert(!HdTexturesFormat_PackName("."));
    assert(!HdTexturesFormat_PackName(".."));
    assert(!HdTexturesFormat_PackName(".hidden"));
    assert(!HdTexturesFormat_PackName("Packs/Upscaled"));
    assert(!HdTexturesFormat_PackName("Packs\\Upscaled"));
    assert(!HdTexturesFormat_PackName("x" "\x7f"));
    assert(!HdTexturesFormat_PackName("a\tb"));
    char tooLong[HD_TEXTURES_PACK_MAX + 1];
    memset(tooLong, 'x', sizeof(tooLong) - 1);
    tooLong[sizeof(tooLong) - 1] = '\0';
    assert(!HdTexturesFormat_PackName(tooLong));
    assert(!HdTexturesFormat_Path(path, sizeof(path), "..", "./ISLAND/ISLAND00.RAW", -1));
    assert(!HdTexturesFormat_Path(path, sizeof(path), "", "./ISLAND/ISLAND00.RAW", -1));
}

static void accepts_only_square_pages_of_512_1024_and_2048(void)
{
    assert(HdTexturesFormat_Side(512L * 512 * 4) == 512);
    assert(HdTexturesFormat_Side(1024L * 1024 * 4) == 1024);
    assert(HdTexturesFormat_Side(2048L * 2048 * 4) == 2048);
    assert(HdTexturesFormat_Side(256L * 256 * 4) == 0);
    assert(HdTexturesFormat_Side(4096L * 4096 * 4) == 0);
    assert(HdTexturesFormat_Side(1000L * 1000 * 4) == 0);
    assert(HdTexturesFormat_Side(1024L * 512 * 4) == 0);
    assert(HdTexturesFormat_Side(1024L * 1024 * 4 - 1) == 0);
    assert(HdTexturesFormat_Side(1024L * 1024 * 3) == 0);
    assert(HdTexturesFormat_Side(0) == 0);
    assert(HdTexturesFormat_Side(-1) == 0);
}

static void halving_averages_colour_and_keeps_alpha_binary(void)
{
    unsigned char page[4 * 4 * 4] = {0};
    unsigned char half[2 * 2 * 4];
    /* Top-left block: four opaque texels. */
    unsigned char reds[4] = {0, 100, 200, 100};
    int texels[4] = {0, 1, 4, 5};
    for (int i = 0; i < 4; i++) {
        page[texels[i] * 4] = reds[i];
        page[texels[i] * 4 + 3] = 255;
    }
    /* Top-right block: two opaque texels out of four. */
    page[2 * 4 + 3] = 255;
    page[3 * 4 + 3] = 255;
    /* Bottom-left block: one opaque texel out of four. */
    page[8 * 4 + 3] = 255;
    HdTexturesFormat_Halve(page, 4, half);
    assert(half[0] == 100 && half[3] == 255);
    assert(half[4 + 3] == 255);
    assert(half[8 + 3] == 0);
    assert(half[12 + 3] == 0);
}

static void halving_ignores_the_colour_of_transparent_texels_and_rounds(void)
{
    unsigned char page[4 * 4 * 4] = {0};
    unsigned char half[2 * 2 * 4];
    /* Top-left block: opaque 10 and 20 red beside transparent texels holding 200. */
    page[0 * 4] = 10;
    page[0 * 4 + 3] = 255;
    page[1 * 4] = 20;
    page[1 * 4 + 3] = 255;
    page[4 * 4] = 200;
    page[5 * 4] = 200;
    /* Top-right block: all opaque, red 0, 0, 0, and 3. */
    int texels[4] = {2, 3, 6, 7};
    for (int i = 0; i < 4; i++) page[texels[i] * 4 + 3] = 255;
    page[7 * 4] = 3;
    HdTexturesFormat_Halve(page, 4, half);
    assert(half[0] == 15);
    assert(half[4] == 1);
}

static void status_texts(void)
{
    char text[160];
    HdTexturesFormat_Status(text, sizeof(text), "", "", 0, 0);
    assert(strcmp(text, "HD textures off") == 0);
    HdTexturesFormat_Status(text, sizeof(text), "", "Inverted", 9, 0);
    assert(strcmp(text, "HD textures off from the next race, 9 pages replaced now") == 0);
    HdTexturesFormat_Status(text, sizeof(text), "Upscaled", "", 0, 0);
    assert(strcmp(text, "HD pack Upscaled is not in the game data") == 0);
    HdTexturesFormat_Status(text, sizeof(text), "Upscaled", "", 0, 1);
    assert(strcmp(text, "HD pack Upscaled, no replacement pages in use") == 0);
    HdTexturesFormat_Status(text, sizeof(text), "Upscaled", "Upscaled", 1, 1);
    assert(strcmp(text, "HD pack Upscaled, 1 page replaced") == 0);
    HdTexturesFormat_Status(text, sizeof(text), "Upscaled", "Upscaled", 9, 1);
    assert(strcmp(text, "HD pack Upscaled, 9 pages replaced") == 0);
    HdTexturesFormat_Status(text, sizeof(text), "Upscaled", "Inverted", 9, 1);
    assert(strcmp(text, "HD pack Upscaled from the next race, 9 pages of Inverted in use now") == 0);
}

int main(void)
{
    maps_a_stock_page_path_to_its_path_in_the_pack();
    maps_each_playfield_page_to_its_own_file();
    rejects_stock_paths_it_cannot_map();
    a_pack_name_is_one_folder_inside_hd();
    accepts_only_square_pages_of_512_1024_and_2048();
    halving_averages_colour_and_keeps_alpha_binary();
    halving_ignores_the_colour_of_transparent_texels_and_rounds();
    status_texts();
    puts("HD texture tests passed");
    return 0;
}
