#include <SDL2/SDL.h>
#include <dlfcn.h>
#include <link.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "ter_types.h"
#include "symbols.h"

/* Once a race has loaded, checks that points inside each collision face pass PointInPolygon for its surface. */

static uintptr_t base;
static unsigned present;
static int keys[SDL_NUM_SCANCODES];

#define AT(type, name) (*(type *)(base + SYM_##name))
#define CALL(type, name) ((type)(base + SYM_##name))

static int LocateExecutable(struct dl_phdr_info *info, size_t size, void *data)
{
    (void)size;
    (void)data;
    if (!info->dlpi_name[0]) {
        base = info->dlpi_addr;
        return 1;
    }
    return 0;
}

int SDL_PollEvent(SDL_Event *event)
{
    static int (*next)(SDL_Event *);
    if (!next) next = dlsym(RTLD_NEXT, "SDL_PollEvent");
    if (next(event)) return 1;
    int desired[SDL_NUM_SCANCODES] = {0};
    if (present >= 60 && present % 60 < 3) {
        desired[SDL_SCANCODE_RETURN] = 1;
        desired[SDL_SCANCODE_SPACE] = 1;
    }
    for (int key = 0; key < SDL_NUM_SCANCODES; key++) {
        if (keys[key] == desired[key]) continue;
        keys[key] = desired[key];
        SDL_zero(*event);
        event->type = desired[key] ? SDL_KEYDOWN : SDL_KEYUP;
        event->key.keysym.scancode = key;
        event->key.state = desired[key] ? SDL_PRESSED : SDL_RELEASED;
        return 1;
    }
    return 0;
}

static void Check(void)
{
    char *vertices = AT(char *, g_terVertexTable);
    TerFace *faces = AT(TerFace *, g_terFaceTable);
    TerSurface *surfaces = AT(TerSurface *, g_trackSurfaceData);
    int surfaceCount = (int)((AT(char *, g_terCollisionMesh) - (char *)surfaces) / sizeof(TerSurface));
    int (*pointIn)(int, int, int) = CALL(int (*)(int, int, int), PointInPolygon);
    int (*stretched)(int) = CALL(int (*)(int), WideIslandStretchedSurface);
    int samples[2] = {0}, misses[2] = {0}, surfacesSeen[2] = {0};
    char path[4096];
    snprintf(path, sizeof(path), "%s/ground.txt", getenv("SONICR_TEST_OUTPUT"));
    FILE *out = fopen(path, "w");
    if (!out) exit(2);
    for (int s = 0; s < surfaceCount; s++) {
        int kind = stretched(s) ? 1 : 0;
        surfacesSeen[kind]++;
        for (int f = 0; f < surfaces[s].faceCount; f++) {
            TerFace *face = &faces[surfaces[s].faceBase + f];
            TerVertex *v[4];
            long cx = 0, cz = 0;
            for (int k = 0; k < 4; k++) {
                v[k] = (TerVertex *)(vertices + face->vtxIdx[k]);
                cx += v[k]->x;
                cz += v[k]->z;
            }
            cx = cx * 4096 / 4;
            cz = cz * 4096 / 4;
            /* The centroid, then points halfway from it toward each corner. */
            for (int k = -1; k < 4; k++) {
                long x = k < 0 ? cx : (cx + v[k]->x * 4096L) / 2;
                long z = k < 0 ? cz : (cz + v[k]->z * 4096L) / 2;
                samples[kind]++;
                if (!pointIn((int)x, (int)z, s)) {
                    misses[kind]++;
                    fprintf(out, "miss surface %d face %d at %ld %ld\n", s, f, x >> 12, z >> 12);
                }
            }
        }
    }
    fprintf(out, "stock surfaces %d samples %d misses %d\n", surfacesSeen[0], samples[0], misses[0]);
    fprintf(out, "stretched surfaces %d samples %d misses %d\n", surfacesSeen[1], samples[1], misses[1]);
    fclose(out);
    exit(0);
}

void SDL_GL_SwapWindow(SDL_Window *window)
{
    static void (*next)(SDL_Window *);
    if (!next) {
        next = dlsym(RTLD_NEXT, "SDL_GL_SwapWindow");
        dl_iterate_phdr(LocateExecutable, NULL);
        AT(int, g_lastMenuSelection) = 0;
    }
    if (AT(int, g_introCountdown) > 0) Check();
    next(window);
    if (++present > 3000) exit(3);
}
