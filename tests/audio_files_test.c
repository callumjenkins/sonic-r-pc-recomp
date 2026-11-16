#define SDL_MAIN_HANDLED
#include "../source/sdl/src/sound/sfx_sdl.c"
#include "../source/sdl/src/sound/music_sdl.c"
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

static int failRW;
static int lastDescriptor;
SDL_RWops *__real_SDL_RWFromFP(FILE *file, SDL_bool autoclose);

SDL_RWops *__wrap_SDL_RWFromFP(FILE *file, SDL_bool autoclose)
{
    lastDescriptor = fileno(file);
    if (failRW) { failRW = 0; SDL_SetError("RW allocation failed"); return NULL; }
    return __real_SDL_RWFromFP(file, autoclose);
}

int g_masterVolume;
int g_volumeBase = -2500;
int g_optSfxVolume = 8;
int g_demoMode;
int g_initFeatureB;
void *g_lpDirectSound;
void *g_soundBuffers[64];
int g_soundActive[64];

void SetAllSoundVolumes(void) { g_masterVolume = 0; }

SDL_RWops *SDL_RWFromFile(const char *path, const char *mode)
{
    (void)path;
    (void)mode;
    SDL_SetError("Relative Android SDL paths point outside gamedata");
    return NULL;
}

static SDL_atomic_t audible;
static void Capture(void *data, Uint8 *stream, int length)
{
    (void)data;
    for (int i = 0; i < length / 2; i++) {
        if (((Sint16 *)stream)[i] != 0) SDL_AtomicSet(&audible, 1);
    }
}

static void WriteClip(const char *name)
{
    char path[256];
    snprintf(path, sizeof(path), "SOUND/SFX/%s", name);
    FILE *file = fopen(path, "wb");
    assert(file);
    unsigned char header[44] = {
        'R','I','F','F',0x24,0x20,0,0,'W','A','V','E',
        'f','m','t',' ',16,0,0,0,1,0,1,0,0x22,0x56,0,0,
        0x44,0xac,0,0,2,0,16,0,'d','a','t','a',0,0x20,0,0
    };
    assert(fwrite(header, sizeof(header), 1, file) == 1);
    for (int i = 0; i < 4096; i++) {
        unsigned char sample[2] = {0, (i & 16) ? 0x20 : 0xe0};
        assert(fwrite(sample, 2, 1, file) == 1);
    }
    assert(fclose(file) == 0);
}

static void CheckPlayback(int slot, int loop, int frequency)
{
    SFX_StopAll();
    SDL_AtomicSet(&audible, 0);
    SFX_Play(slot, loop, frequency);
    Uint32 end = SDL_GetTicks() + 1000;
    while (!SDL_AtomicGet(&audible) && SDL_GetTicks() < end) SDL_Delay(5);
    assert(SDL_AtomicGet(&audible));
    SFX_Stop(slot);
}

int main(int argc, char **argv)
{
    assert(argc == 2 && chdir(argv[1]) == 0);
    assert(mkdir("SOUND", 0700) == 0);
    assert(mkdir("SOUND/SFX", 0700) == 0);
    for (int i = 0; s_sfxTable[i].slot >= 0; i++) WriteClip(s_sfxTable[i].filename);
    assert(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER) == 0);
    assert(Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 512) == 0);
    InitDirectSound();
    for (int i = 0; s_sfxTable[i].slot >= 0; i++) {
        int slot = s_sfxTable[i].slot;
        assert(g_soundActive[slot] && s_chunks[slot]);
        assert(SFX_ClipDurationMs(slot) >= 180);
    }
    assert(LoadWAVIntoSlot(63, "MISSING.WAV") == 0);
    assert(!g_soundActive[63]);
    failRW = 1;
    assert(LoadWAVIntoSlot(63, "JUMP.WAV") == 0);
    assert(fcntl(lastDescriptor, F_GETFD) == -1 && errno == EBADF);
    FILE *bad = fopen("SOUND/SFX/CORRUPT.WAV", "wb");
    assert(bad && fwrite("RIFF", 4, 1, bad) == 1);
    assert(fclose(bad) == 0);
    assert(LoadWAVIntoSlot(63, "CORRUPT.WAV") == 0);
    assert(fcntl(lastDescriptor, F_GETFD) == -1 && errno == EBADF);
    assert(LoadWAVIntoSlot(63, "JUMP.WAV") == 1);
    assert(unlink("SOUND/SFX/JUMP.WAV") == 0);
    assert(LoadWAVIntoSlot(63, "JUMP.WAV") == 0);
    assert(!g_soundActive[63] && !s_chunks[63]);
    assert(mkdir("MUSIC", 0700) == 0);
    assert(rename("SOUND/SFX/PAUSE.WAV", "MUSIC/track2.wav") == 0);
    FILE *preferred = fopen("MUSIC/track2.ogg", "wb");
    assert(preferred && fwrite("bad", 3, 1, preferred) == 1);
    assert(fclose(preferred) == 0);
    Mix_Music *music = load_track(2);
    assert(music);
    Mix_SetPostMix(Capture, NULL);
    SDL_AtomicSet(&audible, 0);
    assert(Mix_PlayMusic(music, 0) == 0);
    Uint32 deadline = SDL_GetTicks() + 1000;
    while (!SDL_AtomicGet(&audible) && SDL_GetTicks() < deadline) SDL_Delay(5);
    assert(SDL_AtomicGet(&audible));
    Mix_HaltMusic();
    Mix_FreeMusic(music);
    assert(fcntl(lastDescriptor, F_GETFD) == -1 && errno == EBADF);
    assert(unlink("MUSIC/track2.ogg") == 0);
    failRW = 1;
    assert(load_track(2) == NULL);
    assert(fcntl(lastDescriptor, F_GETFD) == -1 && errno == EBADF);
    assert(load_track(3) == NULL);
    Mix_SetPostMix(Capture, NULL);
    CheckPlayback(7, 0, 0);
    CheckPlayback(13, 0, 0);
    CheckPlayback(3, 1, 16000);
    Mix_SetPostMix(NULL, NULL);
    CloseDirectSound();
    Mix_CloseAudio();
    SDL_Quit();
    puts("Audio file loading and playback passed");
}

void DebugLog(char *format, ...) { (void)format; }
