#ifndef HD_TEXTURES_FORMAT_H
#define HD_TEXTURES_FORMAT_H

#include <stddef.h>

/* The size of a buffer that holds any pack name, with its terminator. */
#define HD_TEXTURES_PACK_MAX 64

/* Whether `name` can name a pack: one folder directly inside HD, not hidden, and printable. */
int HdTexturesFormat_PackName(const char *name);
/* Writes the path of a stock page's replacement in `pack`, for a stock path relative to the data folder. Returns 0
 * when the pack name is not valid, the stock path has no folder or extension, or the result does not fit. `part`
 * picks one of the two pages a playfield file fills, or is -1 for a whole page. */
int HdTexturesFormat_Path(char *out, size_t size, const char *pack, const char *stockPath, int part);
/* The side of a square RGBA page of `bytes`, or 0 when the size is not 512, 1024, or 2048. */
int HdTexturesFormat_Side(long bytes);
/* Writes the next mip level. Alpha stays 0 or 255, because averaged alpha would pass the 0.01 alpha test and grow
 * the cutout with distance. */
void HdTexturesFormat_Halve(const unsigned char *page, int side, unsigned char *half);
/* `pack` is the chosen pack, empty when off, and `loadedPack` the one the replaced pages came from. */
void HdTexturesFormat_Status(char *text, size_t size, const char *pack, const char *loadedPack, int pages, int hasPack);

#endif
