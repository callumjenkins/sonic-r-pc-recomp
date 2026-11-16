#include "cpu_opponents.h"
#include "sonicr_types.h"
#include "sonicr_globals.h"
#include "sonicr_functions.h"
#include "vertex_struct.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

CpuOpponentsOptions g_cpuOpponentsOptions = {0, 0, {-1, -1, -1, -1}};
static int s_stock[4], s_stockHuman, s_stockValid;
static int s_active, s_colours;
static int s_available;
static char s_status[180] = "CPU roster: Original";
static const char *s_names[CHAR_COUNT] = {
    "Sonic", "Tails", "Knuckles", "Amy", "Eggman", "Metal Sonic",
    "Tails Doll", "Metal Knuckles", "Egg Robo", "Super Sonic"
};
static const char *s_keys[CHAR_COUNT] = {
    "sonic", "tails", "knuckles", "amy", "eggman", "metal-sonic",
    "tails-doll", "metal-knuckles", "egg-robo", "super-sonic"
};

CpuOpponentsOptions CpuOpponents_GetOptions(void)
{
    LOCK();
    CpuOpponentsOptions options = g_cpuOpponentsOptions;
    UNLOCK();
    return options;
}

void CpuOpponents_SetOptions(CpuOpponentsOptions options)
{
    options.enabled = options.enabled != 0;
    options.colours = options.colours != 0;
    for (int i = 0; i < 4; i++) {
        if (options.characters[i] < -1 || options.characters[i] >= CHAR_COUNT)
            options.characters[i] = -1;
    }
    LOCK();
    g_cpuOpponentsOptions = options;
    UNLOCK();
}

int CpuOpponents_ParseRoster(const char *text)
{
    if (!text) return 0;
    CpuOpponentsOptions options = CpuOpponents_GetOptions();
    for (int slot = 0; slot < 4; slot++) {
        const char *end = strchr(text, ',');
        size_t length = end ? (size_t)(end - text) : strlen(text);
        int character = -2;
        if (length == 6 && strncmp(text, "random", length) == 0) character = -1;
        for (int id = 0; id < CHAR_COUNT; id++) {
            if ((length == 1 && text[0] == '0' + id) ||
                (strlen(s_keys[id]) == length && strncmp(text, s_keys[id], length) == 0))
                character = id;
        }
        if (character == -2 || (slot < 3 && !end) || (slot == 3 && end)) return 0;
        options.characters[slot] = character;
        text = end ? end + 1 : text + length;
    }
    options.enabled = 1;
    CpuOpponents_SetOptions(options);
    return 1;
}

int CpuOpponents_Command(int option, const char *text)
{
    if (option == CPU_OPT_ROSTER) {
        if (!CpuOpponents_ParseRoster(text)) {
            fprintf(stderr, "--cpu-opponents requires four comma-separated characters or random\n");
            return 2;
        }
        return 0;
    }
    CpuOpponentsOptions options = CpuOpponents_GetOptions();
    if (option == CPU_OPT_CLASSIC) options.enabled = 0;
    else if (option == CPU_OPT_COLOURS || option == CPU_OPT_CLASSIC_COLOURS)
        options.colours = option == CPU_OPT_COLOURS;
    else return 0;
    CpuOpponents_SetOptions(options);
    return 0;
}

static int SupportedRace(void)
{
    return g_playerBase && g_numPlayers == 5 && g_numHumans == 1 &&
           g_numViewports == 1 && g_raceType == RACE_GP &&
           g_raceSubMode == SUBMODE_NORMAL && g_demoMode == DEMO_NONE &&
           !g_netSessionActive && !g_isNetworkGame;
}

void CpuOpponents_RememberStock(void)
{
    s_stockValid = SupportedRace();
    if (!s_stockValid) return;
    s_stockHuman = g_playerBase[0].charId;
    for (int i = 0; i < 4; i++) s_stock[i] = g_playerBase[i + 1].charId;
    s_active = s_colours = 0;
}

void CpuOpponents_UpdateAvailability(void)
{
    int available = 0;
    for (int i = 0; i < CHAR_COUNT; i++)
        if (g_charUnlockTable[i] == 2) available |= 1 << i;
    LOCK();
    s_available = available;
    UNLOCK();
}

static void PublishStatus(void)
{
    char status[sizeof(s_status)] = "CPU roster: Original";
    if (s_active) {
        snprintf(status, sizeof(status), "CPU roster: %s, %s, %s, %s%s",
            s_names[g_playerBase[1].charId], s_names[g_playerBase[2].charId],
            s_names[g_playerBase[3].charId], s_names[g_playerBase[4].charId],
            s_colours ? "; clone colours" : "");
    }
    LOCK();
    memcpy(s_status, status, sizeof(status));
    UNLOCK();
}

void CpuOpponents_Apply(void)
{
    s_active = s_colours = 0;
    if (!SupportedRace()) {
        PublishStatus();
        return;
    }
    if (!s_stockValid || s_stockHuman != g_playerBase[0].charId)
        CpuOpponents_RememberStock();
    for (int i = 0; i < 4; i++) g_playerBase[i + 1].charId = (short)s_stock[i];
    CpuOpponentsOptions options = CpuOpponents_GetOptions();
    if (options.enabled) {
        int available[CHAR_COUNT], count = 0;
        for (int i = 0; i < CHAR_COUNT; i++)
            if (g_charUnlockTable[i] == 2) available[count++] = i;
        if (count) {
            for (int slot = 0; slot < 4; slot++) {
                int character = options.characters[slot];
                if (character < 0 || character >= CHAR_COUNT || g_charUnlockTable[character] != 2)
                    character = available[Random() % count];
                g_playerBase[slot + 1].charId = (short)character;
            }
            s_active = 1;
            s_colours = options.colours;
        }
    }
    PublishStatus();
}

uint32_t CpuOpponents_TintColour(int variant, uint32_t colour)
{
    if (variant < 1 || variant > 4) return colour;
    int r = (colour >> 16) & 255, g = (colour >> 8) & 255, b = colour & 255;
    int maximum = r > g ? r : g;
    if (b > maximum) maximum = b;
    static const unsigned char palette[4][3] = {
        {255, 0, 0}, {0, 255, 0}, {255, 0, 255}, {0, 255, 255}
    };
    r = maximum * palette[variant - 1][0] / 255;
    g = maximum * palette[variant - 1][1] / 255;
    b = maximum * palette[variant - 1][2] / 255;
    return (colour & 0xFF000000u) | (uint32_t)(r << 16) | (uint32_t)(g << 8) | (uint32_t)b;
}

void CpuOpponents_TintLighting(Player *player)
{
    if (!s_active || !s_colours || !SupportedRace()) return;
    int slot = -1;
    for (int i = 0; i < 5; i++) if (player == &g_playerBase[i]) slot = i;
    if (slot < 1) return;
    int variant = 0;
    for (int i = 0; i < slot; i++)
        if (g_playerBase[i].charId == player->charId) variant++;
    if (!variant) return;
    const ModelMeta *model = &g_modelMeta[player->charId];
    SrcVertex *vertex = &g_vertexArrayBase[model->vertexStart];
    for (int i = 0; i < model->vertexCount; i++, vertex++) {
        int r = vertex->colorR >> 13, g = vertex->colorG >> 13, b = vertex->colorB >> 13;
        if (r < 0) r = 0; else if (r > 255) r = 255;
        if (g < 0) g = 0; else if (g > 255) g = 255;
        if (b < 0) b = 0; else if (b > 255) b = 255;
        uint32_t tinted = CpuOpponents_TintColour(variant, (uint32_t)(r << 16 | g << 8 | b));
        vertex->colorR = ((tinted >> 16) & 255) * 0x2000 + 0x1000;
        vertex->colorG = ((tinted >> 8) & 255) * 0x2000 + 0x1000;
        vertex->colorB = (tinted & 255) * 0x2000 + 0x1000;
    }
}

#ifdef __ANDROID__
JNIEXPORT void JNICALL Java_org_sonicr_recomp_GameActivity_nativeCpuOpponents(
    JNIEnv *env, jclass type, jboolean enabled, jboolean colours, jintArray characters)
{
    (void)type;
    if (!characters || (*env)->GetArrayLength(env, characters) != 4) return;
    jint values[4];
    (*env)->GetIntArrayRegion(env, characters, 0, 4, values);
    if ((*env)->ExceptionCheck(env)) return;
    CpuOpponentsOptions options = {enabled, colours, {values[0], values[1], values[2], values[3]}};
    CpuOpponents_SetOptions(options);
}

JNIEXPORT jint JNICALL Java_org_sonicr_recomp_GameActivity_nativeCpuAvailability(JNIEnv *env, jclass type)
{
    (void)env;
    (void)type;
    LOCK();
    int available = s_available;
    UNLOCK();
    return available;
}

JNIEXPORT jstring JNICALL Java_org_sonicr_recomp_GameActivity_nativeCpuStatus(JNIEnv *env, jclass type)
{
    (void)type;
    char status[sizeof(s_status)];
    LOCK();
    memcpy(status, s_status, sizeof(status));
    UNLOCK();
    return (*env)->NewStringUTF(env, status);
}
#endif
