#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <dlfcn.h>
#include <link.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "sonicr_types.h"
#include "player_struct.h"
#include "symbols.h"

static uintptr_t base;
static unsigned present;
static int keys[SDL_NUM_SCANCODES];
static int saw_countdown;
static unsigned race_start;
static SDL_Joystick *pad;

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
    Sint16 lx = 0, ly = 0, rx = 0;
    if (race_start) {
        unsigned tick = present - race_start;
        if (tick >= 30 && tick < 90) ly = -32767;
        if (tick >= 90 && tick < 120) lx = -32767;
        if (tick >= 120 && tick < 150) { lx = -19660; ly = -19660; }
        if (tick >= 150 && tick < 180) ly = 32767;
        if (tick >= 210 && tick < 240) rx = 32767;
        desired[SDL_SCANCODE_R] = tick >= 240 && tick < 243;
        if (tick >= 270) ly = -32767;
        desired[SDL_SCANCODE_SPACE] = tick >= 270 && tick < 273;
    }
    static Sint16 last[3];
    Sint16 axes[3] = {lx, ly, rx};
    for (int i = 0; pad && i < 3; i++) {
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

#ifdef SYM_s_tpageHDSide
/* Logs, once a frame, each slot whose HD page changed as "present tpage side". A page reloaded at the same address
 * within a frame goes unlogged. */
static void LogHdPages(void)
{
    static void *last[52];
    static FILE *log;
    void *const *pages = (void *const *)(base + SYM_s_tpageHD);
    const int *sides = (const int *)(base + SYM_s_tpageHDSide);
    if (!log) log = OpenOutput("hd.txt", "w");
    for (int i = 0; i < 52; i++) {
        if (pages[i] == last[i]) continue;
        fprintf(log, "%u %d %d\n", present, i, sides[i]);
        last[i] = pages[i];
    }
    fflush(log);
}
#endif

void SDL_GL_SwapWindow(SDL_Window *window)
{
    static void (*next)(SDL_Window *);
    static FILE *trace;
    static FILE *video;
    static int initialized;
    if (!initialized) {
        next = dlsym(RTLD_NEXT, "SDL_GL_SwapWindow");
        dl_iterate_phdr(LocateExecutable, NULL);
        *(int *)(base + SYM_g_lastMenuSelection) = 1;
        trace = OpenOutput("trace.csv", "w");
        fputs("tick,x,y,z,yaw,velocity_x,velocity_y,velocity_z,camera_x,camera_y,camera_z\n", trace);
        if (getenv("SONICR_TEST_VIDEO")) video = OpenOutput("frames.rgb", "wb");
        initialized = 1;
    }
#ifdef SYM_s_tpageHDSide
    LogHdPages();
#endif
    int countdown = *(int *)(base + SYM_g_introCountdown);
    if (countdown > 0) saw_countdown = 1;
    if (saw_countdown && countdown == 0 && !race_start) race_start = present;
    if (race_start) {
        unsigned tick = present - race_start;
        Player *player = *(Player **)(base + SYM_g_playerBase);
        RenderCamera *camera = (RenderCamera *)(base + SYM_g_viewportConfigArray);
        fprintf(trace, "%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", tick,
            player->posX, player->posY, player->posZ, player->angleYaw,
            player->velX, player->velY, player->velZ,
            camera->intX, camera->intY, camera->intZ);
        fflush(trace);
        int snapshot = tick % 30 == 0;
        if (video || snapshot) {
            int width, height;
            SDL_GL_GetDrawableSize(window, &width, &height);
            unsigned char *pixels = malloc((size_t)width * height * 4);
            if (!pixels) exit(2);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            for (size_t i = 0; i < (size_t)width * height; i++) {
                pixels[i * 3] = pixels[i * 4];
                pixels[i * 3 + 1] = pixels[i * 4 + 1];
                pixels[i * 3 + 2] = pixels[i * 4 + 2];
            }
            if (tick == 0) {
                FILE *size = OpenOutput("size.txt", "w");
                fprintf(size, "%dx%d\n", width, height);
                fclose(size);
            }
            if (video) fwrite(pixels, 3, (size_t)width * height, video);
            if (snapshot) {
                char name[64];
                snprintf(name, sizeof(name), "frame-%04u.ppm", tick);
                FILE *file = OpenOutput(name, "wb");
                fprintf(file, "P6\n%d %d\n255\n", width, height);
                for (int y = height - 1; y >= 0; y--)
                    fwrite(pixels + (size_t)y * width * 3, 3, width, file);
                fclose(file);
            }
            free(pixels);
        }
        if (tick >= 300) {
            fclose(trace);
            if (video) fclose(video);
            exit(0);
        }
    }
    next(window);
    present++;
    if (present > 1800) exit(3);
}
