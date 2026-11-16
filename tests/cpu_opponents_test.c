#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../source/sdl/src/cpu_opponents.c"

int g_saveBlock[554];
static Player players[5];
static SrcVertex vertices[3];
static int randomValue, randomCalls;

int Random(void)
{
    randomCalls++;
    return randomValue;
}

static void Fixture(void)
{
    memset(players, 0, sizeof(players));
    memset(g_saveBlock, 0, sizeof(int) * 554);
    g_playerBase = players;
    g_numPlayers = 5;
    g_numHumans = g_numViewports = 1;
    g_raceType = RACE_GP;
    g_raceSubMode = SUBMODE_NORMAL;
    g_demoMode = DEMO_NONE;
    g_netSessionActive = g_isNetworkGame = 0;
    for (int i = 0; i < 5; i++) players[i].charId = (short)i;
    for (int i = 0; i < 4; i++) g_charUnlockTable[i] = 2;
    g_charUnlockTable[CHAR_EGGMAN] = 1;
    randomValue = randomCalls = 0;
    CpuOpponents_SetOptions((CpuOpponentsOptions){0, 0, {-1, -1, -1, -1}});
    CpuOpponents_RememberStock();
}

static void SetAll(int character, int colours)
{
    CpuOpponents_SetOptions((CpuOpponentsOptions){1, colours,
        {character, character, character, character}});
}

static void StockRoster(void)
{
    for (int i = 0; i < 5; i++) assert(players[i].charId == i);
}

static void DisabledAndRestore(void)
{
    Fixture();
    Player before[5];
    memcpy(before, players, sizeof(players));
    CpuOpponents_Apply();
    assert(memcmp(before, players, sizeof(players)) == 0 && randomCalls == 0);
    SetAll(CHAR_SONIC, 1);
    CpuOpponents_Apply();
    for (int i = 0; i < 5; i++) assert(players[i].charId == CHAR_SONIC);
    g_cpuOpponentsOptions.enabled = 0;
    CpuOpponents_Apply();
    StockRoster();
    assert(!s_active && !s_colours && randomCalls == 0);
    assert(strcmp(s_status, "CPU roster: Original") == 0);
}

static void AllUnlockedCharacters(void)
{
    for (int character = 0; character < CHAR_COUNT; character++) {
        Fixture();
        g_charUnlockTable[character] = 2;
        SetAll(character, 0);
        CpuOpponents_Apply();
        assert(players[0].charId == CHAR_SONIC);
        for (int slot = 1; slot < 5; slot++) assert(players[slot].charId == character);
        assert(randomCalls == 0 && s_active && !s_colours);
    }
    Fixture();
    players[0].charId = CHAR_SUPER_SONIC;
    g_allCharsUnlocked = 2;
    CpuOpponents_RememberStock();
    SetAll(CHAR_SUPER_SONIC, 1);
    CpuOpponents_Apply();
    for (int i = 0; i < 5; i++) assert(players[i].charId == CHAR_SUPER_SONIC);
}

static void UnlockGatesAndRandom(void)
{
    for (int state = 0; state <= 1; state++) {
        Fixture();
        g_allCharsUnlocked = state;
        randomValue = 32767;
        SetAll(CHAR_SUPER_SONIC, 0);
        CpuOpponents_Apply();
        assert(randomCalls == 4);
        for (int slot = 1; slot < 5; slot++) assert(players[slot].charId == CHAR_AMY);
    }
    Fixture();
    SetAll(CHAR_EGGMAN, 0);
    CpuOpponents_Apply();
    for (int slot = 1; slot < 5; slot++) assert(players[slot].charId == CHAR_SONIC);
    assert(randomCalls == 4);
    Fixture();
    memset(g_charUnlockTable, 0, CHAR_COUNT * sizeof(int));
    SetAll(-1, 0);
    CpuOpponents_Apply();
    StockRoster();
    assert(!s_active && randomCalls == 0);
    Fixture();
    CpuOpponents_SetOptions((CpuOpponentsOptions){1, 0, {-20, 999, CHAR_TAILS, CHAR_AMY}});
    CpuOpponents_Apply();
    assert(players[1].charId == 0 && players[2].charId == 0);
    assert(players[3].charId == 1 && players[4].charId == 3 && randomCalls == 2);
}

static void OtherModes(void)
{
    for (int which = 0; which < 10; which++) {
        Fixture();
        SetAll(CHAR_SONIC, 1);
        switch (which) {
            case 0: g_raceType = RACE_MULTIPLAYER; break;
            case 1: g_raceType = RACE_TIMEATTACK; break;
            case 2: g_raceType = RACE_SPECIAL; break;
            case 3: g_demoMode = DEMO_TITLE; break;
            case 4: g_demoMode = DEMO_REPLAY; break;
            case 5: g_netSessionActive = 1; break;
            case 6: g_isNetworkGame = 1; break;
            case 7: g_numViewports = 2; break;
            case 8: g_numHumans = 2; break;
            case 9: g_numPlayers = 2; break;
        }
        CpuOpponents_Apply();
        StockRoster();
        assert(!s_active && !s_colours && randomCalls == 0);
    }
    Fixture();
    g_playerBase = NULL;
    CpuOpponents_Apply();
    assert(!s_active);
}

static void OptionsDefaultOff(void)
{
    assert(g_cpuOpponentsOptions.enabled == 0 && g_cpuOpponentsOptions.colours == 0);
    for (int slot = 0; slot < 4; slot++) assert(g_cpuOpponentsOptions.characters[slot] == -1);
}

static void ParseKeepsColourSwitch(void)
{
    Fixture();
    CpuOpponents_SetOptions((CpuOpponentsOptions){0, 1, {CHAR_EGGMAN, CHAR_EGGMAN, CHAR_EGGMAN, CHAR_EGGMAN}});
    assert(CpuOpponents_ParseRoster("sonic,tails,knuckles,amy"));
    assert(g_cpuOpponentsOptions.enabled == 1 && g_cpuOpponentsOptions.colours == 1);
    assert(g_cpuOpponentsOptions.characters[0] == CHAR_SONIC && g_cpuOpponentsOptions.characters[3] == CHAR_AMY);
    CpuOpponentsOptions options = CpuOpponents_GetOptions();
    options.enabled = 0;
    CpuOpponents_SetOptions(options);
    assert(!g_cpuOpponentsOptions.enabled && g_cpuOpponentsOptions.colours == 1);
    assert(g_cpuOpponentsOptions.characters[1] == CHAR_TAILS);
    assert(CpuOpponents_ParseRoster("random,random,random,random"));
    assert(g_cpuOpponentsOptions.enabled == 1 && g_cpuOpponentsOptions.colours == 1);
    for (int slot = 0; slot < 4; slot++) assert(g_cpuOpponentsOptions.characters[slot] == -1);
}

static void StatusOmitsColourSuffix(void)
{
    Fixture();
    SetAll(CHAR_AMY, 0);
    CpuOpponents_Apply();
    assert(!s_colours);
    assert(strcmp(s_status, "CPU roster: Amy, Amy, Amy, Amy") == 0);
}

static void InterleavedDuplicatesKeepTheirOwnTint(void)
{
    Fixture();
    players[0].charId = CHAR_KNUCKLES;
    CpuOpponents_RememberStock();
    CpuOpponents_SetOptions((CpuOpponentsOptions){1, 1, {CHAR_SONIC, CHAR_TAILS, CHAR_SONIC, CHAR_TAILS}});
    CpuOpponents_Apply();
    assert(players[1].charId == CHAR_SONIC && players[2].charId == CHAR_TAILS);
    assert(players[3].charId == CHAR_SONIC && players[4].charId == CHAR_TAILS);
    SrcVertex buffer[2];
    int grey = 0x80 * 0x2000 + 0x1000;
    buffer[0].colorR = buffer[0].colorG = buffer[0].colorB = grey;
    buffer[1] = buffer[0];
    g_vertexArrayBase = buffer;
    g_modelMeta[CHAR_SONIC].vertexStart = 0;
    g_modelMeta[CHAR_SONIC].vertexCount = 1;
    g_modelMeta[CHAR_TAILS].vertexStart = 1;
    g_modelMeta[CHAR_TAILS].vertexCount = 1;
    SrcVertex original[2];
    memcpy(original, buffer, sizeof(buffer));
    CpuOpponents_TintLighting(&players[1]);
    CpuOpponents_TintLighting(&players[2]);
    assert(memcmp(buffer, original, sizeof(buffer)) == 0);
    CpuOpponents_TintLighting(&players[3]);
    assert(memcmp(&buffer[1], &original[1], sizeof(buffer[1])) == 0);
    uint32_t sonic = (uint32_t)(buffer[0].colorR >> 13) << 16 |
        (uint32_t)(buffer[0].colorG >> 13) << 8 | (uint32_t)(buffer[0].colorB >> 13);
    assert(sonic == (CpuOpponents_TintColour(1, 0x808080u) & 0xFFFFFFu));
    CpuOpponents_TintLighting(&players[4]);
    assert(sonic == ((uint32_t)(buffer[0].colorR >> 13) << 16 |
        (uint32_t)(buffer[0].colorG >> 13) << 8 | (uint32_t)(buffer[0].colorB >> 13)));
    uint32_t tails = (uint32_t)(buffer[1].colorR >> 13) << 16 |
        (uint32_t)(buffer[1].colorG >> 13) << 8 | (uint32_t)(buffer[1].colorB >> 13);
    assert(tails == (CpuOpponents_TintColour(1, 0x808080u) & 0xFFFFFFu));
}

static void DesktopColourFlags(void)
{
    Fixture();
    assert(CpuOpponents_Command(CPU_OPT_ROSTER, "sonic,tails") == 2);
    assert(!g_cpuOpponentsOptions.enabled && g_cpuOpponentsOptions.characters[0] == -1);
    assert(CpuOpponents_Command(CPU_OPT_ROSTER, "sonic,tails,knuckles,amy") == 0);
    assert(g_cpuOpponentsOptions.enabled == 1 && !g_cpuOpponentsOptions.colours);
    assert(g_cpuOpponentsOptions.characters[0] == CHAR_SONIC && g_cpuOpponentsOptions.characters[3] == CHAR_AMY);
    assert(CpuOpponents_Command(CPU_OPT_COLOURS, NULL) == 0 && g_cpuOpponentsOptions.colours == 1);
    assert(CpuOpponents_Command(CPU_OPT_CLASSIC, NULL) == 0);
    assert(!g_cpuOpponentsOptions.enabled && g_cpuOpponentsOptions.colours == 1);
    assert(g_cpuOpponentsOptions.characters[1] == CHAR_TAILS);
    assert(CpuOpponents_Command(CPU_OPT_CLASSIC_COLOURS, NULL) == 0);
    assert(!g_cpuOpponentsOptions.enabled && !g_cpuOpponentsOptions.colours);
}

static void ParseAndAvailability(void)
{
    Fixture();
    assert(CpuOpponents_ParseRoster("super-sonic,9,random,amy"));
    assert(g_cpuOpponentsOptions.enabled);
    assert(g_cpuOpponentsOptions.characters[0] == 9 && g_cpuOpponentsOptions.characters[1] == 9);
    assert(g_cpuOpponentsOptions.characters[2] == -1 && g_cpuOpponentsOptions.characters[3] == 3);
    CpuOpponentsOptions before = g_cpuOpponentsOptions;
    const char *invalid[] = {NULL, "", "9", "9,9,9", "9,9,9,9,9", "9,9,9,", "9,9,,9", "9,9,10,9", "-1,9,9,9", " 9,9,9,9", "Super Sonic,9,9,9"};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        assert(!CpuOpponents_ParseRoster(invalid[i]));
        assert(memcmp(&before, &g_cpuOpponentsOptions, sizeof(before)) == 0);
    }
    CpuOpponents_UpdateAvailability();
    assert(s_available == 15);
    g_allCharsUnlocked = 2;
    CpuOpponents_UpdateAvailability();
    assert(s_available == (15 | 1 << CHAR_SUPER_SONIC));
    g_allCharsUnlocked = 1;
    CpuOpponents_UpdateAvailability();
    assert(s_available == 15);
}

static void ColoursAndLighting(void)
{
    uint32_t variants[5];
    for (int i = 0; i < 5; i++) {
        variants[i] = CpuOpponents_TintColour(i, 0x7FFFFF00u);
        assert((variants[i] >> 24) == 0x7F);
        assert(CpuOpponents_TintColour(i, 0x81000000u) == 0x81000000u);
        for (int j = 0; j < i; j++) assert(variants[j] != variants[i]);
    }
    assert(CpuOpponents_TintColour(0, 0x12345678u) == 0x12345678u);
    assert(CpuOpponents_TintColour(5, 0x12345678u) == 0x12345678u);
    assert(CpuOpponents_TintColour(1, 0xFFFFFFFFu) != 0xFFFFFFFFu);
    Fixture();
    memset(vertices, 0, sizeof(vertices));
    g_vertexArrayBase = vertices;
    g_modelMeta[CHAR_SONIC].vertexStart = 0;
    g_modelMeta[CHAR_SONIC].vertexCount = 3;
    SetAll(CHAR_SONIC, 1);
    CpuOpponents_Apply();
    extern void UpdateVertexLightingPhase(Player *player);
    g_trackId = TRACK_RADIANT_EMERALD;
    g_lightingPhaseGlobal = 0;
    for (int i = 0; i < 3 * 32; i++) {
        g_charLightingTable[i * 3] = 255 * 0x2000 + 0x1000;
        g_charLightingTable[i * 3 + 1] = 255 * 0x2000 + 0x1000;
        g_charLightingTable[i * 3 + 2] = 0x1000;
    }
    for (int repeat = 0; repeat < 2; repeat++) {
        for (int slot = 0; slot < 5; slot++) {
            UpdateVertexLightingPhase(&players[slot]);
            CpuOpponents_TintLighting(&players[slot]);
            uint32_t colour = (uint32_t)(vertices[0].colorR >> 13) << 16 |
                (uint32_t)(vertices[0].colorG >> 13) << 8 | (uint32_t)(vertices[0].colorB >> 13);
            assert(colour == (variants[slot] & 0xFFFFFFu));
        }
    }
    SetAll(CHAR_SONIC, 0);
    CpuOpponents_Apply();
    UpdateVertexLightingPhase(&players[4]);
    SrcVertex before[3];
    memcpy(before, vertices, sizeof(vertices));
    CpuOpponents_TintLighting(&players[4]);
    assert(memcmp(before, vertices, sizeof(vertices)) == 0);
}

static void RosterLifecycle(void)
{
    for (int mode = 1; mode <= 3; mode++) {
        Fixture(); SetAll(CHAR_SONIC, 1); g_raceSubMode = mode;
        CpuOpponents_Apply(); StockRoster(); assert(!s_active && randomCalls == 0);
    }
    Fixture();
    CpuOpponents_SetOptions((CpuOpponentsOptions){-1,42,{-1,0,9,10}});
    assert(g_cpuOpponentsOptions.enabled == 1 && g_cpuOpponentsOptions.colours == 1);
    assert(g_cpuOpponentsOptions.characters[0] == -1 && g_cpuOpponentsOptions.characters[3] == -1);
    for (int id = 0; id < CHAR_COUNT; id++) {
        Fixture(); memset(g_charUnlockTable, 0, CHAR_COUNT * sizeof(int));
        g_charUnlockTable[id] = 2; SetAll(-1, 1); randomValue = 32767;
        CpuOpponents_Apply(); for (int p = 1; p < 5; p++) assert(players[p].charId == id);
    }
    Fixture(); SetAll(CHAR_TAILS, 1); CpuOpponents_Apply();
    assert(strcmp(s_status, "CPU roster: Tails, Tails, Tails, Tails; clone colours") == 0);
    SetAll(CHAR_AMY, 0);
    for (int p = 1; p < 5; p++) assert(players[p].charId == CHAR_TAILS);
    assert(s_colours == 1);
    CpuOpponents_Apply(); for (int p = 1; p < 5; p++) assert(players[p].charId == CHAR_AMY);
    assert(s_colours == 0);
    CpuOpponents_SetOptions((CpuOpponentsOptions){0,0,{-1,-1,-1,-1}});
    CpuOpponents_Apply(); StockRoster();
    Fixture(); SetAll(CHAR_SONIC, 1); CpuOpponents_Apply();
    g_demoMode = DEMO_REPLAY; CpuOpponents_Apply();
    assert(!s_active && !s_colours && strcmp(s_status,"CPU roster: Original") == 0);
    for (int p = 1; p < 5; p++) assert(players[p].charId == CHAR_SONIC);
    g_demoMode = DEMO_NONE;
    CpuOpponents_SetOptions((CpuOpponentsOptions){0,0,{-1,-1,-1,-1}});
    CpuOpponents_Apply(); StockRoster();
    Fixture(); players[0].charId = 3;
    for (int p = 1; p < 5; p++) players[p].charId = 9-p;
    CpuOpponents_Apply();
    for (int p = 1; p < 5; p++) assert(players[p].charId == 9-p);
    Fixture(); CpuOpponents_UpdateAvailability();
    for (int id = 0; id < CHAR_COUNT; id++) g_charUnlockTable[id] = id % 2;
    CpuOpponents_UpdateAvailability(); assert(s_available == 0);
    for (int id = 0; id < CHAR_COUNT; id++) g_charUnlockTable[id] = 2;
    CpuOpponents_UpdateAvailability(); assert(s_available == 1023);

}
static void LightingBounds(void)
{
    Fixture(); SetAll(CHAR_TAILS, 1); CpuOpponents_Apply();
    SrcVertex buffer[7]; memset(buffer, 0, sizeof(buffer));
    g_vertexArrayBase = buffer;
    g_modelMeta[CHAR_TAILS].vertexStart = 2;
    g_modelMeta[CHAR_TAILS].vertexCount = 3;
    g_trackId = TRACK_RESORT_ISLAND; g_colorTintEnable = 1;
    g_colorTintR = -0x1000; g_colorTintG = 0x1000; g_colorTintB = 0x4000;
    g_lightingPhaseGlobal = 7;
    for (int i = 0; i < 3*32; i++) {
        g_charLightingTable[2*32*3+i*3] = 0x60000;
        g_charLightingTable[2*32*3+i*3+1] = 0x20000;
        g_charLightingTable[2*32*3+i*3+2] = 0x10000;
    }
    extern void UpdateVertexLightingPhase(Player *player);
    for (int repeat = 0; repeat < 2; repeat++) {
        for (int slot = 1; slot < 5; slot++) {
            players[slot].angleYaw = slot * 87;
            UpdateVertexLightingPhase(&players[slot]);
            SrcVertex original[7]; memcpy(original,buffer,sizeof(buffer));
            CpuOpponents_TintLighting(&players[slot]);
            assert(memcmp(buffer,original,2*sizeof(SrcVertex)) == 0);
            assert(memcmp(buffer+5,original+5,2*sizeof(SrcVertex)) == 0);
            if (slot == 1) assert(memcmp(buffer, original, sizeof(buffer)) == 0);
            else {
                uint32_t packed = (original[2].colorR>>13)<<16 | (original[2].colorG>>13)<<8 | (original[2].colorB>>13);
                uint32_t want = CpuOpponents_TintColour(slot-1,packed);
                uint32_t actual=(buffer[2].colorR>>13)<<16|(buffer[2].colorG>>13)<<8|(buffer[2].colorB>>13);
                assert(actual == want);
            }
        }
    }
    Player outsider = players[1]; SrcVertex original[7]; memcpy(original,buffer,sizeof(buffer));
    CpuOpponents_TintLighting(&outsider); assert(memcmp(buffer,original,sizeof(buffer)) == 0);
    for (int i = 0; i < 3; i++) {buffer[2+i].colorR=-0x2000;buffer[2+i].colorG=0x400000;buffer[2+i].colorB=0;}
    CpuOpponents_TintLighting(&players[4]);
    uint32_t want = CpuOpponents_TintColour(3,0x00ff00);
    assert((buffer[2].colorR>>13) == ((want>>16)&255));
    assert((buffer[2].colorG>>13) == ((want>>8)&255));
    assert((buffer[2].colorB>>13) == (want&255));
    for (int channel = 0; channel < 3; channel++) {
        buffer[2].colorR = channel == 0 ? -0x2000 : 0;
        buffer[2].colorG = channel == 1 ? -0x2000 : 0;
        buffer[2].colorB = channel == 2 ? -0x2000 : 0;
        CpuOpponents_TintLighting(&players[4]);
        assert((buffer[2].colorR >> 13) == 0);
        assert((buffer[2].colorG >> 13) == 0);
        assert((buffer[2].colorB >> 13) == 0);
    }
    for (int gate=0;gate<3;gate++) {
        Fixture(); SetAll(CHAR_SONIC,1); CpuOpponents_Apply();
        memset(buffer,0x42,sizeof(buffer)); memcpy(original,buffer,sizeof(buffer));
        if(gate==0)g_demoMode=DEMO_REPLAY;
        if(gate==1)s_active=0;
        if(gate==2)s_colours=0;
        CpuOpponents_TintLighting(&players[4]); assert(memcmp(buffer,original,sizeof(buffer)) == 0);
    }
    assert(CpuOpponents_TintColour(-1,0x12345678)==0x12345678);
    const uint32_t palette[4] = {0xff0000,0x00ff00,0xff00ff,0x00ffff};
    const uint32_t inputs[] = {0x00112233,0x00332211,0x00113322,0x00808080,0x00000001,0x00010000,0x00000100,0x00ffffff};
    const int brightness[] = {0x33,0x33,0x33,0x80,1,1,1,255};
    for(int input=0;input<8;input++)for(int variant=1;variant<=4;variant++) {
        uint32_t want = 0x81000000;
        for(int channel=0;channel<3;channel++)
            if((palette[variant-1]>>(channel*8))&255) want |= brightness[input] << (channel*8);
        assert(CpuOpponents_TintColour(variant,0x81000000|inputs[input]) == want);
    }
    Fixture();g_demoMode=DEMO_REPLAY;CpuOpponents_RememberStock();assert(!s_stockValid);
    g_demoMode=DEMO_NONE;CpuOpponents_Apply();StockRoster();assert(s_stockValid);

}

#include "r_draw.h"
#include "r_state.h"
float g_uvLUT256[256];
int g_animFrameData[1];
void R_SetTexture(int x) {(void)x;}
void R_SetTexEnv(R_TexEnvMode x) {(void)x;}
void R_DrawTri(const RenderVertex *v) {(void)v;}
void R_DrawQuad(const RenderVertex *v) {(void)v;}
void R_DrawTriFan(const RenderVertex *v, int n) {(void)v;(void)n;}
void RelocateModelVertices(int *a,int b,int c,int d,int e) {(void)a;(void)b;(void)c;(void)d;(void)e;}
extern void RenderCharacterD3D(Player *player);
void DispatchFaceUVAnimation(Player *player) {(void)player;}
static void RendererLightingHook(void) {
 Fixture(); SetAll(CHAR_SONIC, 1); CpuOpponents_Apply();
 g_vertexArrayBase = vertices;
 g_modelMeta[0].vertexStart=0; g_modelMeta[0].vertexCount=3;
 g_modelMeta[0].limbCount=0; g_modelMeta[0].boundRadius=1;
 g_viewMtx00=g_viewMtx11=g_viewMtx22=4096;
 g_viewMtx01=g_viewMtx02=g_viewMtx10=g_viewMtx12=g_viewMtx20=g_viewMtx21=0;
 g_camOrientX=g_camOrientY=g_camOrientZ=0;
 g_screenCenterX=320;g_screenCenterY=240;
 g_projScaleXCurrent=g_projScaleY=256;
 g_clipLeft=g_clipTop=0;g_clipRight=639;g_clipBottom=479;
 g_farClipDepth=10000;g_trackId=TRACK_RADIANT_EMERALD;g_lightingPhaseGlobal=0;
 for(int i=0;i<3*32;i++) {
   g_charLightingTable[i*3]=255*0x2000+0x1000;
   g_charLightingTable[i*3+1]=255*0x2000+0x1000;
   g_charLightingTable[i*3+2]=0x1000;
 }
 for(int repeat=0;repeat<2;repeat++)for(int slot=0;slot<5;slot++) {
   players[slot]._unk_0x1E0=0;players[slot].posZ=100*4096;
   RenderCharacterD3D(&players[slot]);
   uint32_t actual=(vertices[0].colorR>>13)<<16|(vertices[0].colorG>>13)<<8|(vertices[0].colorB>>13);
   assert(actual==(CpuOpponents_TintColour(slot,0xffff00)&0xffffff));
 }

}

#ifdef __ANDROID__
typedef struct {
    jsize length;
    jint values[5];
} TestArray;

static int arrayException;

static jsize JNICALL ArrayLength(JNIEnv *env, jarray array)
{
    (void)env;
    return ((TestArray *)array)->length;
}

static void JNICALL ArrayRegion(JNIEnv *env, jintArray array, jsize start, jsize length, jint *output)
{
    (void)env;
    memcpy(output, ((TestArray *)array)->values + start, length * sizeof(jint));
}

static jboolean JNICALL ArrayException(JNIEnv *env)
{
    (void)env;
    return arrayException;
}

static char statusText[180];

static jstring JNICALL NewStringUTF(JNIEnv *env, const char *text)
{
    (void)env;
    snprintf(statusText, sizeof(statusText), "%s", text);
    return (jstring)statusText;
}

static void AndroidBoundaries(void)
{
    const struct JNINativeInterface_ functions = {
        .GetArrayLength = ArrayLength,
        .GetIntArrayRegion = ArrayRegion,
        .ExceptionCheck = ArrayException,
        .NewStringUTF = NewStringUTF
    };
    JNIEnv environment = &functions;
    Fixture();
    CpuOpponentsOptions before = g_cpuOpponentsOptions;
    Java_org_sonicr_recomp_GameActivity_nativeCpuOpponents(&environment, NULL, 1, 1, NULL);
    assert(memcmp(&before, &g_cpuOpponentsOptions, sizeof(before)) == 0);
    for (int length = 0; length <= 5; length++) {
        if (length == 4) continue;
        TestArray array = {length, {0, 1, 2, 3, 4}};
        Java_org_sonicr_recomp_GameActivity_nativeCpuOpponents(&environment, NULL, 1, 1, (jintArray)&array);
        assert(memcmp(&before, &g_cpuOpponentsOptions, sizeof(before)) == 0);
    }
    TestArray array = {4, {-2, 10, 1, 3}};
    arrayException = 1;
    Java_org_sonicr_recomp_GameActivity_nativeCpuOpponents(&environment, NULL, 1, 1, (jintArray)&array);
    assert(memcmp(&before, &g_cpuOpponentsOptions, sizeof(before)) == 0);
    arrayException = 0;
    TestArray named = {4, {CHAR_TAILS, CHAR_KNUCKLES, CHAR_AMY, CHAR_EGGMAN}};
    Java_org_sonicr_recomp_GameActivity_nativeCpuOpponents(&environment, NULL, 0, 0, (jintArray)&named);
    assert(!g_cpuOpponentsOptions.enabled && !g_cpuOpponentsOptions.colours);
    assert(g_cpuOpponentsOptions.characters[0] == CHAR_TAILS && g_cpuOpponentsOptions.characters[3] == CHAR_EGGMAN);
    Java_org_sonicr_recomp_GameActivity_nativeCpuOpponents(&environment, NULL, 1, 0, (jintArray)&named);
    assert(g_cpuOpponentsOptions.enabled == 1 && !g_cpuOpponentsOptions.colours);
    g_charUnlockTable[CHAR_EGGMAN] = 2;
    CpuOpponents_Apply();
    jstring status = Java_org_sonicr_recomp_GameActivity_nativeCpuStatus(&environment, NULL);
    assert(strcmp((const char *)status, "CPU roster: Tails, Knuckles, Amy, Eggman") == 0);
    g_charUnlockTable[CHAR_EGGMAN] = 1;
    Java_org_sonicr_recomp_GameActivity_nativeCpuOpponents(&environment, NULL, 42, 255, (jintArray)&array);
    assert(g_cpuOpponentsOptions.enabled == 1 && g_cpuOpponentsOptions.colours == 1);
    assert(g_cpuOpponentsOptions.characters[0] == -1 && g_cpuOpponentsOptions.characters[1] == -1);
    assert(g_cpuOpponentsOptions.characters[2] == 1 && g_cpuOpponentsOptions.characters[3] == 3);
    CpuOpponents_UpdateAvailability();
    assert(Java_org_sonicr_recomp_GameActivity_nativeCpuAvailability(&environment, NULL) == 15);
    puts("Android CPU option boundary checks passed");
}
#endif

int main(void)
{
#ifdef __ANDROID__
    OptionsDefaultOff();
    AndroidBoundaries();
#else
    OptionsDefaultOff();
    DisabledAndRestore();
    AllUnlockedCharacters();
    UnlockGatesAndRandom();
    OtherModes();
    ParseKeepsColourSwitch();
    StatusOmitsColourSuffix();
    InterleavedDuplicatesKeepTheirOwnTint();
    DesktopColourFlags();
    ParseAndAvailability();
    ColoursAndLighting();
    RosterLifecycle();
    LightingBounds();
    RendererLightingHook();
    puts("CPU opponent checks passed");
#endif
}
