#define _GNU_SOURCE
#include "hd_textures.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static unsigned char *s_pages[52];
static int s_sides[52];

void R_SetTpageHD(int tpage, unsigned char *rgba, int side)
{
    free(s_pages[tpage]);
    s_pages[tpage] = rgba;
    s_sides[tpage] = side;
}

static int s_dirty[52];

void R_MarkTextureDirty(int tpage)
{
    s_dirty[tpage]++;
}

void DebugLog(char *fmt, ...)
{
    (void)fmt;
}

static void WritePage(const char *path, long bytes, int invert)
{
    FILE *file = fopen(path, "wb");
    assert(file);
    for (long i = 0; i < bytes; i++) fputc((int)((i & 0xFF) ^ invert), file);
    fclose(file);
}

static void UsePack(const char *pack)
{
    HdTexturesOptions options = {{0}};
    strcpy(options.pack, pack);
    HdTextures_SetOptions(options);
}

static void off_ignores_a_present_pack(void)
{
    UsePack("");
    HdTextures_Load(3, "./ISLAND/ISLAND00.RAW", -1);
    assert(!s_pages[3]);
    assert(strcmp(HdTextures_Status(), "HD textures off") == 0);
}

static void a_pack_that_is_not_there_loads_nothing(void)
{
    UsePack("Missing");
    HdTextures_Load(3, "./ISLAND/ISLAND00.RAW", -1);
    assert(!s_pages[3]);
    assert(strcmp(HdTextures_Status(), "HD pack Missing is not in the game data") == 0);
}

static void a_pack_name_outside_hd_turns_textures_off(void)
{
    UsePack("..");
    assert(HdTextures_GetOptions().pack[0] == '\0');
    HdTextures_Load(3, "./ISLAND/ISLAND00.RAW", -1);
    assert(!s_pages[3]);
}

static void on_loads_a_page_from_the_chosen_pack_and_counts_it(void)
{
    UsePack("Upscaled");
    HdTextures_Load(3, "./ISLAND/ISLAND00.RAW", -1);
    assert(s_pages[3] && s_sides[3] == 512 && s_pages[3][300] == (300 & 0xFF));
    assert(s_dirty[3] == 1);
    HdTextures_Load(14, "./ISLAND/ISLAND.PLY", 0);
    assert(s_pages[14] && s_sides[14] == 512);
    assert(strcmp(HdTextures_Status(), "HD pack Upscaled, 2 pages replaced") == 0);
}

static void each_pack_supplies_its_own_pages(void)
{
    UsePack("Inverted");
    assert(strcmp(HdTextures_Status(), "HD pack Inverted from the next race, 2 pages of Upscaled in use now") == 0);
    HdTextures_Load(3, "./ISLAND/ISLAND00.RAW", -1);
    assert(s_pages[3] && s_pages[3][300] == (unsigned char)~(300 & 0xFF));
    HdTextures_Load(14, "./ISLAND/ISLAND.PLY", 0);
    assert(!s_pages[14]);
    assert(strcmp(HdTextures_Status(), "HD pack Inverted, 1 page replaced") == 0);
    UsePack("Upscaled");
    HdTextures_Load(14, "./ISLAND/ISLAND.PLY", 0);
}

static void a_slot_without_a_replacement_goes_back_to_stock(void)
{
    HdTextures_Load(3, "./ISLAND/ISLAND01.RAW", -1);
    assert(!s_pages[3]);
    assert(strcmp(HdTextures_Status(), "HD pack Upscaled, 1 page replaced") == 0);
}

static void a_truncated_page_is_skipped(void)
{
    HdTextures_Load(5, "./ISLAND/ISLAND02.RAW", -1);
    assert(!s_pages[5]);
}

static void drop_returns_a_slot_to_stock(void)
{
    int dirty = s_dirty[14];
    HdTextures_Drop(14);
    assert(!s_pages[14] && s_dirty[14] == dirty + 1);
    HdTextures_Drop(14);
    assert(s_dirty[14] == dirty + 1);
    assert(strcmp(HdTextures_Status(), "HD pack Upscaled, no replacement pages in use") == 0);
}

static void out_of_range_slots_are_ignored(void)
{
    HdTextures_Drop(-1);
    HdTextures_Drop(52);
    HdTextures_Load(-1, "./ISLAND/ISLAND00.RAW", -1);
    HdTextures_Load(52, "./ISLAND/ISLAND00.RAW", -1);
    assert(strcmp(HdTextures_Status(), "HD pack Upscaled, no replacement pages in use") == 0);
}

static void status_needs_the_pack_to_be_a_folder(void)
{
    assert(rename("HD/Upscaled", "HD/Upscaled-moved") == 0);
    WritePage("HD/Upscaled", 1, 0);
    assert(strcmp(HdTextures_Status(), "HD pack Upscaled is not in the game data") == 0);
    assert(unlink("HD/Upscaled") == 0);
    assert(rename("HD/Upscaled-moved", "HD/Upscaled") == 0);
}

int main(void)
{
    char scratch[] = "hd-textures-XXXXXX";
    assert(mkdtemp(scratch));
    assert(chdir(scratch) == 0);
    assert(mkdir("HD", 0700) == 0 && mkdir("HD/Upscaled", 0700) == 0 && mkdir("HD/Upscaled/ISLAND", 0700) == 0);
    assert(mkdir("HD/Inverted", 0700) == 0 && mkdir("HD/Inverted/ISLAND", 0700) == 0);
    WritePage("HD/Upscaled/ISLAND/ISLAND00.RGBA", 512L * 512 * 4, 0);
    WritePage("HD/Upscaled/ISLAND/ISLAND.PLY.0.RGBA", 512L * 512 * 4, 0);
    WritePage("HD/Upscaled/ISLAND/ISLAND02.RGBA", 512L * 512 * 4 - 1, 0);
    WritePage("HD/Inverted/ISLAND/ISLAND00.RGBA", 512L * 512 * 4, 0xFF);
    off_ignores_a_present_pack();
    a_pack_that_is_not_there_loads_nothing();
    a_pack_name_outside_hd_turns_textures_off();
    on_loads_a_page_from_the_chosen_pack_and_counts_it();
    each_pack_supplies_its_own_pages();
    a_slot_without_a_replacement_goes_back_to_stock();
    a_truncated_page_is_skipped();
    drop_returns_a_slot_to_stock();
    out_of_range_slots_are_ignored();
    status_needs_the_pack_to_be_a_folder();
    for (int i = 0; i < 52; i++) free(s_pages[i]);
    puts("HD texture loading tests passed");
    return 0;
}
