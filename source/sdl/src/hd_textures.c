#include "hd_textures.h"
#include "hd_textures_format.h"
#include "fileio.h"
#include "r_texture.h"
#include "sonicr_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef __ANDROID__
#include <SDL.h>
#include <jni.h>
static SDL_SpinLock s_lock;
#define LOCK() SDL_AtomicLock(&s_lock)
#define UNLOCK() SDL_AtomicUnlock(&s_lock)
#else
#define LOCK() ((void)0)
#define UNLOCK() ((void)0)
#endif

#define TPAGE_COUNT 52

static HdTexturesOptions s_options;
static unsigned char s_replaced[TPAGE_COUNT];
static char s_loadedPack[HD_TEXTURES_PACK_MAX];

HdTexturesOptions HdTextures_GetOptions(void)
{
    LOCK();
    HdTexturesOptions options = s_options;
    UNLOCK();
    return options;
}

void HdTextures_SetOptions(HdTexturesOptions options)
{
    options.pack[HD_TEXTURES_PACK_MAX - 1] = '\0';
    if (!HdTexturesFormat_PackName(options.pack)) options.pack[0] = '\0';
    LOCK();
    s_options = options;
    UNLOCK();
}

static void SetReplaced(int tpage, int replaced)
{
    LOCK();
    s_replaced[tpage] = (unsigned char)replaced;
    UNLOCK();
}

void HdTextures_Drop(int tpage)
{
    if (tpage < 0 || tpage >= TPAGE_COUNT || !s_replaced[tpage]) return;
    SetReplaced(tpage, 0);
    R_SetTpageHD(tpage, NULL, 0);
    R_MarkTextureDirty(tpage);
}

void HdTextures_Load(int tpage, const char *stockPath, int part)
{
    char path[512];
    if (tpage < 0 || tpage >= TPAGE_COUNT) return;
    HdTextures_Drop(tpage);
    HdTexturesOptions options = HdTextures_GetOptions();
    if (!HdTexturesFormat_Path(path, sizeof(path), options.pack, stockPath, part)) return;
    FILE *fp = fOpen(path, "rb");
    if (!fp) return;
    fSeek(fp, 0, SEEK_END);
    long bytes = fTell(fp);
    fSeek(fp, 0, SEEK_SET);
    int side = HdTexturesFormat_Side(bytes);
    unsigned char *rgba = side ? malloc((size_t)bytes) : NULL;
    if (!rgba || fRead(rgba, 1, (size_t)bytes, fp) != (size_t)bytes) {
        DebugLog("HD texture %s skipped: not a readable square RGBA page of 512, 1024, or 2048 pixels\n", path);
        free(rgba);
        fClose(fp);
        return;
    }
    fClose(fp);
    R_SetTpageHD(tpage, rgba, side);
    R_MarkTextureDirty(tpage);
    LOCK();
    s_replaced[tpage] = 1;
    strcpy(s_loadedPack, options.pack);
    UNLOCK();
}

const char *HdTextures_Status(void)
{
    static char status[192];
    char folder[HD_TEXTURES_PACK_MAX + 3];
    char loadedPack[HD_TEXTURES_PACK_MAX];
    struct stat info;
    HdTexturesOptions options = HdTextures_GetOptions();
    snprintf(folder, sizeof(folder), "HD/%s", options.pack);
    int hasPack = options.pack[0] && stat(folder, &info) == 0 && S_ISDIR(info.st_mode);
    LOCK();
    int pages = 0;
    for (int i = 0; i < TPAGE_COUNT; i++) pages += s_replaced[i];
    strcpy(loadedPack, s_loadedPack);
    UNLOCK();
    HdTexturesFormat_Status(status, sizeof(status), options.pack, loadedPack, pages, hasPack);
    return status;
}

#ifdef __ANDROID__
JNIEXPORT void JNICALL Java_org_sonicr_recomp_GameActivity_nativeHdTextures(JNIEnv *env, jclass type, jstring pack)
{
    (void)type;
    HdTexturesOptions options = {{0}};
    const char *chars = pack ? (*env)->GetStringUTFChars(env, pack, NULL) : NULL;
    if (chars && strlen(chars) < sizeof(options.pack)) strcpy(options.pack, chars);
    if (chars) (*env)->ReleaseStringUTFChars(env, pack, chars);
    HdTextures_SetOptions(options);
}

JNIEXPORT jstring JNICALL Java_org_sonicr_recomp_GameActivity_nativeHdTexturesStatus(JNIEnv *env, jclass type)
{
    (void)type;
    return (*env)->NewStringUTF(env, HdTextures_Status());
}
#endif
