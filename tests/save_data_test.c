#include "sonicr_globals.h"
#include "sonicr_functions.h"
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

static int failWrite;
static int failClose;
size_t __real_fwrite(const void *data, size_t size, size_t count, FILE *file);
int __real_fclose(FILE *file);

size_t __wrap_fwrite(const void *data, size_t size, size_t count, FILE *file)
{
    if (failWrite) { failWrite = 0; return 0; }
    return __real_fwrite(data, size, count, file);
}

int __wrap_fclose(FILE *file)
{
    int result = __real_fclose(file);
    if (failClose) { failClose = 0; return EOF; }
    return result;
}

extern int SaveConfigSlot(int slot);
extern int LoadConfigSlot(int slot);
extern void InitSaveData(void);
extern int LoadGameSettings(void);
extern void *g_configPtrTable[16];
extern const char *s_saveFilenames[10];
extern const char *g_raceSubModeNames[4];
extern const char *g_trackDirNames[6];

void Music_SetVolume(int volume) { (void)volume; }

int main(int argc, char **argv)
{
    assert(argc == 2 && chdir(argv[1]) == 0);
    char overlong[509];
    memset(overlong, 'x', sizeof(overlong) - 1);
    overlong[sizeof(overlong) - 1] = 0;
    assert(WriteSaveFile(overlong, "data", 4) == 0);
    InitSaveData();
    struct stat info;
    assert(stat("SAVE", &info) == 0 && S_ISDIR(info.st_mode));
    for (int mode = 0; mode < 4; mode++) {
        for (int track = 1; track <= 5; track++) {
            char path[256];
            snprintf(path, sizeof(path), "GHOST/%s/%s/SONIC.GHO", g_raceSubModeNames[mode], g_trackDirNames[track]);
            FILE *file = fopen(path, "wb");
            assert(file && fwrite("ghost", 5, 1, file) == 1);
            assert(fclose(file) == 0);
        }
    }
    for (int i = 0; i < 554; i++) g_saveBlock[i] = i * 12345;
    assert(SaveConfigSlot(0) == 1);
    assert(stat("SAVE/R01.SAV", &info) == 0 && info.st_size == 2216);
    InitSaveData();
    g_configPtrTable[0] = NULL;
    assert(ReadConfigSlot(0) == 1);
    assert(stat("SAVE/R01.SAV", &info) == 0 && info.st_size == 2216);
    memset(g_saveBlock, 0, sizeof(int) * 554);
    assert(LoadConfigSlot(0) == 1);
    for (int i = 0; i < 554; i++) assert(g_saveBlock[i] == i * 12345);
    for (int fault = 0; fault < 2; fault++) {
        g_saveBlock[6] = 777;
        failWrite = fault == 0;
        failClose = fault == 1;
        assert(SaveConfigSlot(0) == 0);
        assert(access("SAVE/R01.SAV.tmp", F_OK) != 0);
        assert(LoadConfigSlot(0) == 1 && g_saveBlock[6] == 6 * 12345);
        assert(ReadConfigSlot(0) == 1);
        assert(LoadConfigSlot(0) == 1 && g_saveBlock[6] == 6 * 12345);
    }
    g_saveBlock[6] = 321;
    assert(SaveConfigSlot(0) == 1);
    g_saveBlock[6] = 0;
    assert(LoadConfigSlot(0) == 1 && g_saveBlock[6] == 321);
    g_saveBlock[6] = 6 * 12345;
    assert(SaveConfigSlot(0) == 1);
    assert(SaveConfigSlot(-1) == 0 && SaveConfigSlot(10) == 0);
    assert(ReadConfigSlot(-1) == 0 && ReadConfigSlot(10) == 0);
    assert(LoadConfigSlot(-1) == 0 && LoadConfigSlot(10) == 0);
    assert(LoadConfigSlot(9) == 0);
    const char *previous = s_saveFilenames[0];
    s_saveFilenames[0] = "missing/R01.SAV";
    g_saveBlock[6] = 999;
    assert(SaveConfigSlot(0) == 0);
    g_saveBlock[6] = 0;
    assert(LoadConfigSlot(0) == 1 && g_saveBlock[6] == 6 * 12345);
    s_saveFilenames[0] = previous;
    assert(mkdir("SAVE/R01.SAV.tmp", 0700) == 0);
    g_saveBlock[6] = 888;
    assert(SaveConfigSlot(0) == 0);
    assert(rmdir("SAVE/R01.SAV.tmp") == 0);
    assert(ReadConfigSlot(0) == 1);
    assert(LoadConfigSlot(0) == 1 && g_saveBlock[6] == 6 * 12345);
    assert(mkdir("SAVE/R02.SAV", 0700) == 0);
    assert(SaveConfigSlot(1) == 0 && !g_configPtrTable[1]);
    assert(access("SAVE/R02.SAV.tmp", F_OK) != 0);
    FILE *truncated = fopen("SAVE/R03.SAV", "wb");
    assert(truncated && fwrite("bad", 3, 1, truncated) == 1);
    assert(fclose(truncated) == 0);
    assert(ReadConfigSlot(2) == 0 && !g_configPtrTable[2]);
    g_configPtrTable[2] = g_configPtrTable[0];
    assert(ReadConfigSlot(2) == 0 && !g_configPtrTable[2]);
    assert(ReadConfigSlot(9) == 0);
    assert(SaveConfigSlot(9) == 1 && ReadConfigSlot(9) == 1);
    InitInputMappings();
    g_optSfxVolume = 7;
    g_optMusicVolume = 2;
    SaveGameSettings();
    InitInputMappings();
    LoadGameSettings();
    assert(g_optSfxVolume == 7 && g_optMusicVolume == 2);
    g_optMusicVolume = 6;
    SaveGameSettings();
    InitInputMappings();
    LoadGameSettings();
    assert(g_optMusicVolume == 6);
    g_difficultyConfig = DIFF_HARD;
    g_optSfxVolume = 7;
    SaveGameSettings();
    for (int mode = DEMO_TITLE; mode <= DEMO_REPLAY; mode++) {
        g_demoMode = mode;
        g_difficultyConfig = DIFF_NORMAL;
        g_optSfxVolume = 3;
        SaveGameSettings();
        InitInputMappings();
        LoadGameSettings();
        assert(g_difficultyConfig == DIFF_HARD && g_optSfxVolume == 7);
    }
    g_demoMode = DEMO_NONE;
    FILE *file = fopen("SONICR.INF", "wb");
    assert(file && fwrite("bad", 3, 1, file) == 1);
    assert(fclose(file) == 0);
    InitInputMappings();
    LoadGameSettings();
    assert(g_optSfxVolume == 4 && g_optMusicVolume == 8);
    g_optSfxVolume = 7;
    SaveGameSettings();
    unsigned char settings[161] = {0};
    file = fopen("SONICR.INF", "rb");
    assert(file && fread(settings, 160, 1, file) == 1);
    assert(fclose(file) == 0);
    const size_t settingsSizes[] = {0, 159, 160, 161};
    for (int i = 0; i < 4; i++) {
        file = fopen("SONICR.INF", "wb");
        assert(file && fwrite(settings, 1, settingsSizes[i], file) == settingsSizes[i]);
        assert(fclose(file) == 0);
        InitInputMappings();
        LoadGameSettings();
        assert(g_optSfxVolume == (settingsSizes[i] >= 160 ? 7 : 4));
    }
    unsigned char slotBytes[2217] = {0};
    file = fopen("SAVE/R01.SAV", "rb");
    assert(file && fread(slotBytes, 2216, 1, file) == 1);
    assert(fclose(file) == 0);
    const size_t slotSizes[] = {0, 2215, 2216, 2217};
    for (int i = 0; i < 4; i++) {
        file = fopen("SAVE/R04.SAV", "wb");
        assert(file && fwrite(slotBytes, 1, slotSizes[i], file) == slotSizes[i]);
        assert(fclose(file) == 0);
        assert(ReadConfigSlot(3) == (slotSizes[i] >= 2216));
        assert(LoadConfigSlot(3) == (slotSizes[i] >= 2216));
    }
    puts("Save directories, slots, and settings passed");
}
