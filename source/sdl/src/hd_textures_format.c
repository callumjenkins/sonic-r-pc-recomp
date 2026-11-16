#include "hd_textures_format.h"
#include <stdio.h>
#include <string.h>

int HdTexturesFormat_PackName(const char *name)
{
    if (!name || !name[0] || name[0] == '.' || strlen(name) >= HD_TEXTURES_PACK_MAX) return 0;
    for (const char *c = name; *c; c++)
        if (*c == '/' || *c == '\\' || (unsigned char)*c < 0x20 || *c == 0x7f) return 0;
    return 1;
}

int HdTexturesFormat_Path(char *out, size_t size, const char *pack, const char *stockPath, int part)
{
    if (!stockPath || !HdTexturesFormat_PackName(pack)) return 0;
    while (stockPath[0] == '.' && (stockPath[1] == '/' || stockPath[1] == '\\')) stockPath += 2;
    const char *folder = strpbrk(stockPath, "/\\");
    const char *dot = strrchr(stockPath, '.');
    if (!folder || folder == stockPath || stockPath[0] == '.' || !dot || dot < folder) return 0;
    int written = part < 0
        ? snprintf(out, size, "HD/%s/%.*s.RGBA", pack, (int)(dot - stockPath), stockPath)
        : snprintf(out, size, "HD/%s/%s.%d.RGBA", pack, stockPath, part);
    return written > 0 && (size_t)written < size;
}

int HdTexturesFormat_Side(long bytes)
{
    for (int side = 512; side <= 2048; side *= 2)
        if (bytes == (long)side * side * 4) return side;
    return 0;
}

void HdTexturesFormat_Halve(const unsigned char *page, int side, unsigned char *half)
{
    int halfSide = side / 2;
    for (int y = 0; y < halfSide; y++) {
        for (int x = 0; x < halfSide; x++) {
            const unsigned char *texels[4] = {
                page + ((y * 2) * side + x * 2) * 4,
                page + ((y * 2) * side + x * 2 + 1) * 4,
                page + ((y * 2 + 1) * side + x * 2) * 4,
                page + ((y * 2 + 1) * side + x * 2 + 1) * 4,
            };
            int opaque = 0;
            for (int i = 0; i < 4; i++) opaque += texels[i][3] != 0;
            unsigned char *dst = half + (y * halfSide + x) * 4;
            for (int c = 0; c < 3; c++) {
                int sum = 0, count = 0;
                for (int i = 0; i < 4; i++) {
                    if (opaque && !texels[i][3]) continue;
                    sum += texels[i][c];
                    count++;
                }
                dst[c] = (unsigned char)((sum + count / 2) / count);
            }
            dst[3] = opaque >= 2 ? 255 : 0;
        }
    }
}

void HdTexturesFormat_Status(char *text, size_t size, const char *pack, const char *loadedPack, int pages, int hasPack)
{
    const char *noun = pages == 1 ? "page" : "pages";
    if (!pack[0] && pages)
        snprintf(text, size, "HD textures off from the next race, %d %s replaced now", pages, noun);
    else if (!pack[0])
        snprintf(text, size, "HD textures off");
    else if (pages && strcmp(pack, loadedPack) != 0)
        snprintf(text, size, "HD pack %s from the next race, %d %s of %s in use now", pack, pages, noun, loadedPack);
    else if (pages)
        snprintf(text, size, "HD pack %s, %d %s replaced", pack, pages, noun);
    else if (!hasPack)
        snprintf(text, size, "HD pack %s is not in the game data", pack);
    else
        snprintf(text, size, "HD pack %s, no replacement pages in use", pack);
}
