#include "sonicr_types.h"
#include "sonicr_globals.h"
#include "modern_controls.h"
#include "controls_math.h"
#include "pad_bits.h"
#include "platform.h"
#include <math.h>
#include <stdlib.h>

ModernControlsOptions g_modernControlsOptions;
static ControlsVector s_movement;
static int s_cameraYaw;
static int s_cameraHeight;
static int s_initialized;

static int LocalSonic(void)
{
    return g_playerBase && g_playerBase[0].charId == CHAR_SONIC &&
           g_numViewports == 1 && g_netSessionActive == 0 && g_isNetworkGame == 0;
}

int ModernControlsChangesMovement(void)
{
    return g_modernControlsOptions.movement && LocalSonic();
}

void ModernControlsReset(void)
{
    s_movement = (ControlsVector){0, 0};
    s_cameraYaw = 0;
    s_cameraHeight = 0;
    s_initialized = 0;
}

void ModernControlsUpdate(void)
{
    s_movement = (ControlsVector){0, 0};
    if ((!g_modernControlsOptions.movement && !g_modernControlsOptions.camera) ||
        !LocalSonic() || g_demoMode != DEMO_NONE || g_postRaceCameraMode != 0) {
        s_initialized = 0;
        return;
    }
    Player *player = &g_playerBase[0];
    if (!s_initialized || g_introCountdown > 0) {
        s_cameraYaw = player->angleYaw & 4095;
        s_cameraHeight = 0;
        s_initialized = 1;
    }
    if (g_isPaused || g_introCountdown != 0) {
        return;
    }

    float axes[4];
    int recenter;
#ifdef SONICR_DC
    for (int i = 0; i < 4; i++) axes[i] = 0;
    recenter = 0;
#else
    platform_control_axes(axes, &recenter);
#endif
    s_movement = ControlsStick(axes[0], axes[1]);
    int right = g_diKeyboardState[g_keyMap_P1_Right] != 0;
    int left = g_diKeyboardState[g_keyMap_P1_Left] != 0;
    int up = g_diKeyboardState[g_keyMap_P1_Up] != 0;
    int down = g_diKeyboardState[g_keyMap_P1_Down] != 0;
    if (right || left || up || down) {
        s_movement = ControlsStick((float)(right - left), (float)(up - down));
    } else if (s_movement.x == 0 && s_movement.y == 0) {
        unsigned short buttons = g_perPlayerInput[0];
        s_movement = ControlsStick(
            (float)(!!(buttons & PAD_RIGHT) - !!(buttons & PAD_LEFT)),
            (float)(!!(buttons & PAD_UP) - !!(buttons & PAD_DOWN)));
        if (g_mirrorMode) s_movement.x = -s_movement.x;
    }
    if (g_mirrorMode) s_movement.x = -s_movement.x;

    if (g_modernControlsOptions.camera) {
        ControlsVector look = ControlsStick(axes[2], axes[3]);
        if (g_mirrorMode) look.x = -look.x;
        s_cameraYaw = (s_cameraYaw + (int)lroundf(look.x * 40)) & 4095;
        s_cameraHeight += (int)lroundf(look.y * 12);
        if (s_cameraHeight < -48) s_cameraHeight = -48;
        if (s_cameraHeight > 256) s_cameraHeight = 256;
        if (recenter) {
            s_cameraYaw = player->angleYaw & 4095;
            s_cameraHeight = 0;
        }
    }
}

int ModernControlsMovementActive(Player *player)
{
    return ModernControlsChangesMovement() && player == &g_playerBase[0] &&
           g_demoMode == DEMO_NONE && g_introCountdown == 0 && !g_isPaused &&
           g_postRaceCameraMode == 0 && player->loopMode == 0;
}

unsigned short ModernControlsButtons(Player *player, unsigned short buttons)
{
    if (!ModernControlsMovementActive(player)) {
        return buttons;
    }
    buttons &= ~(PAD_DIRECTIONS | PAD_ACCEL | PAD_DRIFTL | PAD_DRIFTR);
    if (ModernControlsStrength() > 0) buttons |= PAD_ACCEL;
    return buttons;
}

float ModernControlsStrength(void)
{
    return hypotf(s_movement.x, s_movement.y);
}

int ModernControlsSteer(Player *player)
{
    if (!ModernControlsMovementActive(player)) {
        return 0;
    }
    if (ModernControlsStrength() > 0) {
        int cameraYaw = g_modernControlsOptions.camera ? s_cameraYaw : player->moveMode;
        int target = ControlsHeading(s_movement, cameraYaw);
        int step = player->groundedFlag ? 96 : 48;
        if (abs(player->forwardSpeed) < 0x8000 && player->groundedFlag) step = 128;
        player->angleYaw = ControlsTurnToward(player->angleYaw, target, step);
    }
    return 1;
}

static int OrbitCameraActive(Player *player)
{
    return g_modernControlsOptions.camera && LocalSonic() &&
           player == &g_playerBase[0] && s_initialized && g_demoMode == DEMO_NONE &&
           g_postRaceCameraMode == 0 && player->loopMode == 0;
}

int ModernControlsCameraYaw(Player *player, int classicYaw)
{
    return OrbitCameraActive(player) ? s_cameraYaw : classicYaw;
}

int ModernControlsCameraHeight(Player *player)
{
    return OrbitCameraActive(player) ? s_cameraHeight : 0;
}
