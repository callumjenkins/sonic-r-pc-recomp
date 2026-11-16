#include "../source/sdl/src/screen_misc.c"
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
static int frame;
static int errorPixels;
void platform_pump_events(void)
{
    frame++;
    if (frame == 1) {
        g_fadeState = FADE_VISIBLE;
        g_fadeLevel = 0;
        s_lsColumn = 0;
        s_lsActionState = 2;
        g_inputBits = 0;
    } else if (frame == 2) {
        assert(g_fadeState == FADE_VISIBLE && g_screenResult == 0 && s_lsActionState == 0);
        assert(errorPixels > 0);
        assert(rmdir("SAVE/R01.SAV.tmp") == 0);
        s_lsActionState = 2;
    } else {
        assert(frame == 3 && g_fadeState == FADE_OUT && g_screenResult == 1);
        g_fadeLevel = -256;
    }
}

void R_DrawQuad2DSolid(float x1, float y1, float x2, float y2, float z, uint32_t color)
{
    (void)x1; (void)x2; (void)y2; (void)z; (void)color;
    if (y1 >= 448) errorPixels++;
}

void Music_SetVolume(int volume) { (void)volume; }

int main(int argc, char **argv)
{
    assert(argc == 2 && chdir(argv[1]) == 0);
    InitSaveData();
    assert(mkdir("SAVE/R01.SAV.tmp", 0700) == 0);
    assert(LoadSaveScreen() == 1 && frame == 3);
    assert(access("SAVE/R01.SAV", F_OK) == 0);
    puts("Save menu reports failure, stays open, and retries successfully");
}

void *g_tpagePixelBuf[52];
unsigned int timeGetTime(void) { return 0; }
int GetLogicalCDTrack(void) { return 5; }
void BeginFrame(void) {}
void DrawTexturedQuad(int x, int y, int depth, int w, int h, int page,
                      int u, int v, int uw, int vh, unsigned int color)
{ (void)x; (void)y; (void)depth; (void)w; (void)h; (void)page;
  (void)u; (void)v; (void)uw; (void)vh; (void)color; }
void EndFrame(void) {}
void FinalizeMenuTexturesD3D(void) {}
void FlipD3D(void) {}
void LoadAllGhostTimes(void) {}
void LoadTPageRGB(int page, const char *path) { (void)page; (void)path; }
void PlaySoundEffect(int command, int distance, int frequency)
{ (void)command; (void)distance; (void)frequency; }
void ProcessTpageStates(void) {}
void ReadInput(void) {}
void RenderBackground(void) {}
void RenderFadeOverlay(void) {}
void RenderWavingMenuBackground(void) {}
void SetupMenuTexturesD3D(void) {}
void TintBackgroundTPage(int r, int g, int b) { (void)r; (void)g; (void)b; }
void UpdateCDPlayback(int track) { (void)track; }
void UpdateFade(void) {}
void WaitForFrameCap(void) {}
