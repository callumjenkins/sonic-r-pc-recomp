#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <dlfcn.h>
#include <link.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "sonicr_types.h"
#include "player_struct.h"
#include "symbols.h"

/* Drives player 0 along the race-position spline in a Grand Prix and logs every racer until the four CPUs finish. */

static uintptr_t base;
static unsigned present;
static int keys[SDL_NUM_SCANCODES];
static int saw_countdown;
static unsigned race_start;
static SDL_Joystick *pad;
static Sint16 steer;

#define AT(type, name) (*(type *)(base + SYM_##name))

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

int SDL_Init(Uint32 flags)
{
    int (*next)(Uint32) = dlsym(RTLD_NEXT, "SDL_Init");
    int result = next(flags);
    if (result != 0) return result;
    int device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 21, 0);
    if (device < 0) exit(2);
    char guid[33];
    SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(device), guid, sizeof(guid));
    char mapping[512];
    snprintf(mapping, sizeof(mapping),
        "%s,Playtest Controller,a:b0,b:b1,x:b2,y:b3,start:b6,rightstick:b8,"
        "leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,", guid);
    if (SDL_GameControllerAddMapping(mapping) < 0 || !SDL_IsGameController(device)) exit(2);
    pad = SDL_JoystickOpen(device);
    if (!pad) exit(2);
    return result;
}

int SDL_PollEvent(SDL_Event *event)
{
    static int (*next)(SDL_Event *);
    if (!next) next = dlsym(RTLD_NEXT, "SDL_PollEvent");
    if (next(event)) return 1;
    int desired[SDL_NUM_SCANCODES] = {0};
    if (!saw_countdown && present >= 60 && present % 60 < 3) {
        desired[SDL_SCANCODE_RETURN] = 1;
        desired[SDL_SCANCODE_SPACE] = 1;
    }
    Sint16 axes[2] = {race_start ? steer : 0, race_start ? -32767 : 0};
    static Sint16 last[2];
    for (int i = 0; pad && i < 2; i++) {
        if (axes[i] != last[i]) {
            SDL_JoystickSetVirtualAxis(pad, i, axes[i]);
            last[i] = axes[i];
        }
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

static FILE *OpenOutput(const char *name, const char *mode)
{
    char path[4096];
    snprintf(path, sizeof(path), "%s/%s", getenv("SONICR_TEST_OUTPUT"), name);
    FILE *file = fopen(path, mode);
    if (!file) exit(2);
    return file;
}

static void Steer(Player *player)
{
    int *spline = AT(int *, g_splineWaypoints);
    int count = AT(int, g_posDataCount4);
    if (!spline || count <= 0) return;
    int x = player->posX >> 12, z = player->posZ >> 12;
    int nearest = 0;
    double best = 1e30;
    for (int i = 0; i < count; i++) {
        double dx = spline[i * 3] - x, dz = spline[i * 3 + 2] - z;
        if (dx * dx + dz * dz < best) {
            best = dx * dx + dz * dz;
            nearest = i;
        }
    }
    int *target = &spline[((nearest + 3) % count) * 3];
    int heading = (int)lround(atan2(target[0] - x, target[2] - z) * 2048 / M_PI) & 0xFFF;
    int turn = ((heading - player->angleYaw + 2048) & 0xFFF) - 2048;
    int value = turn * 64;
    steer = (Sint16)(value > 32767 ? 32767 : value < -32767 ? -32767 : value);
}

static void Snapshot(SDL_Window *window, const char *name)
{
    int width, height;
    SDL_GL_GetDrawableSize(window, &width, &height);
    unsigned char *pixels = malloc((size_t)width * height * 4);
    if (!pixels) exit(2);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    FILE *file = OpenOutput(name, "wb");
    fprintf(file, "P6\n%d %d\n255\n", width, height);
    for (int y = height - 1; y >= 0; y--) fwrite(pixels + (size_t)y * width * 3, 3, width, file);
    fclose(file);
    free(pixels);
}

void SDL_GL_SwapWindow(SDL_Window *window)
{
    static void (*next)(SDL_Window *);
    static FILE *trace;
    static int initialized;
    static unsigned gapShots;
    if (!initialized) {
        next = dlsym(RTLD_NEXT, "SDL_GL_SwapWindow");
        dl_iterate_phdr(LocateExecutable, NULL);
        AT(int, g_lastMenuSelection) = atoi(getenv("SONICR_PROBE_MENU") ? getenv("SONICR_PROBE_MENU") : "0");
        trace = OpenOutput("racers.csv", "w");
        fputs("tick,player,x,y,z,laps,finish\n", trace);
        initialized = 1;
    }
    int countdown = AT(int, g_introCountdown);
    if (countdown > 0) saw_countdown = 1;
    if (saw_countdown && countdown == 0 && !race_start) race_start = present;
    if (race_start) {
        unsigned tick = present - race_start;
        Player *players = AT(Player *, g_playerBase);
        Steer(&players[0]);
        if (tick % 10 == 0) {
            for (int i = 0; i < 5; i++)
                fprintf(trace, "%u,%d,%d,%d,%d,%d,%d\n", tick, i, players[i].posX >> 12, players[i].posY >> 12,
                        players[i].posZ >> 12, players[i].lapsCompleted, players[i].finishState);
            fflush(trace);
        }
        int x = players[0].posX >> 12;
        int gapFrom = atoi(getenv("SONICR_PROBE_GAP_FROM") ? getenv("SONICR_PROBE_GAP_FROM") : "0");
        int gapTo = atoi(getenv("SONICR_PROBE_GAP_TO") ? getenv("SONICR_PROBE_GAP_TO") : "0");
        char name[64];
        if (gapTo > gapFrom && x > gapFrom && x < gapTo && tick % 20 == 0 && gapShots < 24) {
            snprintf(name, sizeof(name), "gap-%05u.ppm", tick);
            Snapshot(window, name);
            gapShots++;
        }
        if (tick % 600 == 0) {
            snprintf(name, sizeof(name), "frame-%05u.ppm", tick);
            Snapshot(window, name);
        }
        int done = 1;
        for (int i = 1; i < 5; i++) done &= players[i].lapsCompleted >= 3;
        if (done || tick >= (unsigned)atoi(getenv("SONICR_PROBE_TICKS") ? getenv("SONICR_PROBE_TICKS") : "12000")) {
            fclose(trace);
            exit(done ? 0 : 4);
        }
    }
    next(window);
    present++;
}
