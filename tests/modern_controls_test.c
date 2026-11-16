#include "sonicr_types.h"
#include "sonicr_globals.h"
#include "modern_controls.h"
#include "pad_bits.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static Player players[2];
Player *g_playerBase = players;
int g_numViewports = 1;
int g_netSessionActive;
int g_isNetworkGame;
int g_demoMode;
int g_postRaceCameraMode;
int g_introCountdown;
int g_isPaused;
int g_mirrorMode;
int g_keyMappingData[20] = {0, 1, 2, 3, 4};
unsigned char g_diKeyboardState[256];
unsigned short g_perPlayerInput[4];
static float axes[4];
static int recenter;
static int touchMovement;

int platform_control_axes(float out[4], int *reset)
{
    memcpy(out, axes, sizeof(axes));
    *reset = recenter;
    return touchMovement;
}

static void ResetFixture(void)
{
    memset(players, 0, sizeof(players));
    memset(axes, 0, sizeof(axes));
    memset(g_diKeyboardState, 0, sizeof(g_diKeyboardState));
    memset(g_perPlayerInput, 0, sizeof(g_perPlayerInput));
    g_playerBase = players;
    players[0].groundedFlag = 1;
    g_numViewports = 1;
    g_netSessionActive = g_isNetworkGame = g_demoMode = 0;
    g_postRaceCameraMode = g_introCountdown = g_isPaused = g_mirrorMode = 0;
    recenter = 0;
    touchMovement = 0;
    g_modernControlsOptions = (ModernControlsOptions){1, 1};
    ModernControlsReset();
}

static void TestTouchPreservesAnalogueMovement(void)
{
    ResetFixture();
    axes[0] = 0.4f;
    axes[1] = 0.7f;
    g_diKeyboardState[g_keyMap_P1_Right] = 1;
    g_diKeyboardState[g_keyMap_P1_Up] = 1;
    touchMovement = 1;
    ModernControlsUpdate();
    float strength = ModernControlsStrength();
    assert(strength > 0.75f && strength < 0.77f);
    ModernControlsSteer(players);
    assert(players[0].angleYaw == 128);
    for (int i = 0; i < 5; i++) {
        ModernControlsUpdate();
        ModernControlsSteer(players);
    }
    assert(players[0].angleYaw > 330 && players[0].angleYaw < 345);
    axes[0] = axes[1] = 0;
    touchMovement = 0;
    g_diKeyboardState[g_keyMap_P1_Right] = 0;
    ModernControlsUpdate();
    assert(ModernControlsStrength() == 1);
    g_diKeyboardState[g_keyMap_P1_Up] = 0;
    ModernControlsUpdate();
    assert(ModernControlsStrength() == 0);
}

static void TestKeyboardAndDigitalDirections(void)
{
    ResetFixture();
    axes[0] = -1;
    g_diKeyboardState[g_keyMap_P1_Right] = 1;
    ModernControlsUpdate();
    ModernControlsSteer(players);
    assert(players[0].angleYaw == 128);
    ResetFixture();
    g_mirrorMode = 1;
    g_diKeyboardState[g_keyMap_P1_Right] = 1;
    ModernControlsUpdate();
    ModernControlsSteer(players);
    assert(players[0].angleYaw == 3968);
    ResetFixture();
    g_mirrorMode = 1;
    g_perPlayerInput[0] = PAD_LEFT;
    ModernControlsUpdate();
    ModernControlsSteer(players);
    assert(players[0].angleYaw == 3968);
    g_perPlayerInput[0] = 0;
    ModernControlsUpdate();
    assert(ModernControlsStrength() == 0);
}

static void TestTurnLimitsAndCameraBasis(void)
{
    ResetFixture();
    axes[0] = 1;
    players[0].forwardSpeed = 0x10000;
    ModernControlsUpdate();
    ModernControlsSteer(players);
    assert(players[0].angleYaw == 96);
    players[0].angleYaw = 0;
    players[0].groundedFlag = 0;
    ModernControlsSteer(players);
    assert(players[0].angleYaw == 48);
    ResetFixture();
    g_modernControlsOptions.camera = 0;
    players[0].moveMode = 1024;
    axes[1] = 1;
    ModernControlsUpdate();
    ModernControlsSteer(players);
    assert(players[0].angleYaw == 128);
    assert(ModernControlsCameraYaw(players, 700) == 700);
    ResetFixture();
    g_modernControlsOptions.camera = 0;
    axes[2] = 1;
    ModernControlsUpdate();
    axes[2] = 0;
    g_modernControlsOptions.camera = 1;
    ModernControlsUpdate();
    assert(ModernControlsCameraYaw(players, 700) == 0);
}

static void TestCameraHeightAndMirror(void)
{
    ResetFixture();
    axes[3] = 1;
    ModernControlsUpdate();
    assert(ModernControlsCameraHeight(players) == 12);
    for (int i = 0; i < 30; i++) ModernControlsUpdate();
    assert(ModernControlsCameraHeight(players) == 256);
    axes[3] = -1;
    for (int i = 0; i < 40; i++) ModernControlsUpdate();
    assert(ModernControlsCameraHeight(players) == -48);
    recenter = 1;
    ModernControlsUpdate();
    assert(ModernControlsCameraHeight(players) == 0);
    ResetFixture();
    g_mirrorMode = 1;
    axes[2] = 1;
    ModernControlsUpdate();
    assert(ModernControlsCameraYaw(players, 700) == 4056);
}

static void TestLifecycleAndFeatureCombinations(void)
{
    for (int movement = 0; movement <= 1; movement++) {
        for (int camera = 0; camera <= 1; camera++) {
            ResetFixture();
            g_modernControlsOptions = (ModernControlsOptions){movement, camera};
            axes[0] = axes[2] = 1;
            ModernControlsUpdate();
            assert(ModernControlsChangesMovement() == movement);
            assert(ModernControlsSteer(players) == movement);
            assert(ModernControlsCameraYaw(players, 700) == (camera ? 40 : 700));
        }
    }
    ResetFixture();
    ModernControlsUpdate();
    axes[2] = 1;
    ModernControlsUpdate();
    assert(ModernControlsCameraYaw(players, 700) == 40);
    g_introCountdown = 30;
    players[0].angleYaw = 1024;
    axes[0] = axes[2] = 1;
    ModernControlsUpdate();
    assert(ModernControlsCameraYaw(players, 700) == 1024);
    assert(ModernControlsStrength() == 0);
    g_introCountdown = 0;
    g_isPaused = 1;
    ModernControlsUpdate();
    assert(ModernControlsCameraYaw(players, 700) == 1024);
    ResetFixture();
    axes[0] = 1;
    ModernControlsUpdate();
    assert(ModernControlsStrength() == 1);
    g_postRaceCameraMode = 1;
    ModernControlsUpdate();
    assert(ModernControlsStrength() == 0);
    assert(!ModernControlsSteer(players));
    assert(ModernControlsCameraYaw(players, 700) == 700);
    ResetFixture();
    g_netSessionActive = 1;
    assert(!ModernControlsChangesMovement());
    assert(ModernControlsCameraYaw(players, 700) == 700);
    ResetFixture();
    g_playerBase = NULL;
    ModernControlsUpdate();
    assert(!ModernControlsChangesMovement());
    ResetFixture();
    axes[0] = axes[2] = 1;
    ModernControlsUpdate();
    ModernControlsReset();
    assert(ModernControlsStrength() == 0);
    assert(ModernControlsCameraYaw(players, 700) == 700);
}

int main(void)
{
    players[0].groundedFlag = 1;
    players[0].angleYaw = 0;
    players[0].moveMode = 0;
    axes[0] = -1;
    ModernControlsReset();
    ModernControlsUpdate();
    assert(ModernControlsButtons(players, 0xFFFF) == 0xFFFF);
    assert(!ModernControlsSteer(players));
    assert(ModernControlsCameraYaw(players, 123) == 123);

    g_modernControlsOptions.camera = 1;
    ModernControlsUpdate();
    axes[2] = 1;
    ModernControlsUpdate();
    assert(ModernControlsCameraYaw(players, 123) == 40);
    assert(!ModernControlsSteer(players));
    axes[2] = 0;
    recenter = 1;
    ModernControlsUpdate();
    assert(ModernControlsCameraYaw(players, 123) == 0);
    recenter = 0;

    g_modernControlsOptions.movement = 1;
    ModernControlsUpdate();
    assert(ModernControlsSteer(players));
    assert(players[0].angleYaw == 3968);
    for (int i = 0; i < 16; i++) {
        ModernControlsUpdate();
        ModernControlsSteer(players);
    }
    assert(players[0].angleYaw == 3072);
    assert(ModernControlsCameraYaw(players, 123) == 0);
    unsigned short buttons = ModernControlsButtons(players, PAD_JUMP | PAD_DOWN);
    assert(buttons == (PAD_JUMP | PAD_ACCEL));

    axes[0] = 0;
    ModernControlsUpdate();
    assert(ModernControlsStrength() == 0);
    assert(!(ModernControlsButtons(players, PAD_ACCEL) & PAD_ACCEL));

    axes[0] = 1;
    g_isPaused = 1;
    ModernControlsUpdate();
    assert(ModernControlsStrength() == 0);
    assert(!ModernControlsSteer(players));
    g_isPaused = 0;
    g_introCountdown = 30;
    ModernControlsUpdate();
    assert(ModernControlsStrength() == 0);
    assert(!ModernControlsSteer(players));
    g_introCountdown = 0;

    ModernControlsReset();
    players[0].angleYaw = 0;
    g_mirrorMode = 1;
    ModernControlsUpdate();
    ModernControlsSteer(players);
    assert(players[0].angleYaw == 3968);
    g_mirrorMode = 0;

    players[0].loopMode = 1;
    assert(!ModernControlsSteer(players));
    assert(ModernControlsCameraYaw(players, 123) == 123);
    players[0].loopMode = 0;
    players[0].charId = CHAR_TAILS;
    assert(!ModernControlsChangesMovement());
    players[0].charId = CHAR_SONIC;
    g_numViewports = 2;
    assert(!ModernControlsChangesMovement());
    g_numViewports = 1;
    g_isNetworkGame = 1;
    assert(!ModernControlsChangesMovement());
    g_isNetworkGame = 0;
    g_demoMode = DEMO_REPLAY;
    assert(!ModernControlsSteer(players));
    g_demoMode = DEMO_NONE;
    assert(!ModernControlsSteer(&players[1]));

    g_modernControlsOptions.movement = 0;
    assert(!ModernControlsChangesMovement());
    assert(ModernControlsButtons(players, PAD_LEFT | PAD_ACCEL) == (PAD_LEFT | PAD_ACCEL));
    g_modernControlsOptions.camera = 0;
    assert(ModernControlsCameraYaw(players, 123) == 123);
    TestKeyboardAndDigitalDirections();
    TestTouchPreservesAnalogueMovement();
    TestTurnLimitsAndCameraBasis();
    TestCameraHeightAndMirror();
    TestLifecycleAndFeatureCombinations();
    puts("modern controls state passed");
}
