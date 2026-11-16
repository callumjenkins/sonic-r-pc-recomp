#ifndef HD_TEXTURES_H
#define HD_TEXTURES_H

#include "hd_textures_format.h"

typedef struct {
    /* A folder inside HD, or empty when HD textures are off. */
    char pack[HD_TEXTURES_PACK_MAX];
} HdTexturesOptions;

enum {
    HD_TEXTURES_OPT = 1011
};

HdTexturesOptions HdTextures_GetOptions(void);
/* Turns HD textures off when the pack name is not valid. */
void HdTextures_SetOptions(HdTexturesOptions options);
/* Loads the replacement for a page that has just been filled from `stockPath`, when a pack is chosen
 * and has one. Any earlier replacement in the slot goes first, so a page the pack lacks draws stock.
 * `part` is as for HdTexturesFormat_Path. */
void HdTextures_Load(int tpage, const char *stockPath, int part);
/* Returns a page to its stock pixels, for code that is about to change them. */
void HdTextures_Drop(int tpage);
const char *HdTextures_Status(void);

#endif
