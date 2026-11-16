#include "../source/sdl/src/platform/platform_sdl.c"
#include <assert.h>

static void TestTouchAxesAndKeyboardCamera(void)
{
    float axes[4];
    int recenter;
    assert(platform_control_axes(axes, &recenter) == 0);
    for (int i = 0; i < 4; i++) assert(axes[i] == 0);
    assert(recenter == 0);

    Java_org_sonicr_recomp_GameActivity_nativeTouchAxes(NULL, NULL, 0.19f, 0, 0.19f, 0);
    assert(platform_control_axes(axes, &recenter) == 0);
    assert(axes[0] == 0 && axes[2] == 0);
    Java_org_sonicr_recomp_GameActivity_nativeTouchAxes(NULL, NULL, 0.4f, 0.7f, -0.5f, 0.5f);
    assert(platform_control_axes(axes, &recenter) == 1);
    assert(axes[0] == 0.4f && axes[1] == 0.7f && axes[2] == -0.5f && axes[3] == 0.5f);

    s_keystate[DIK_Q] = s_keystate[0xD1] = s_keystate[DIK_R] = 1;
    platform_control_axes(axes, &recenter);
    assert(axes[2] == -1 && axes[3] == -1 && recenter == 1);
    s_keystate[DIK_E] = s_keystate[0xC9] = 1;
    platform_control_axes(axes, &recenter);
    assert(axes[2] == 0 && axes[3] == 0);
    memset(s_keystate, 0, sizeof(s_keystate));
    Java_org_sonicr_recomp_GameActivity_nativeTouchAxes(NULL, NULL, 0, 0, 0, 0);
    assert(platform_control_axes(axes, &recenter) == 0);
    for (int i = 0; i < 4; i++) assert(axes[i] == 0);
}

static void TestControllerAndTouchTakeover(void)
{
    assert(SDL_Init(SDL_INIT_GAMECONTROLLER) == 0);
    int device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,
        SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
    assert(device >= 0);
    SDL_GameController *controller = SDL_GameControllerOpen(device);
    assert(controller != NULL);
    SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controller);
    s_pads[0].ctrl = controller;
    s_padCount = 1;
    SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_LEFTX, -20000);
    SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_RIGHTY, 15000);
    SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_RIGHTSTICK, 1);
    SDL_GameControllerUpdate();
    float axes[4];
    int recenter;
    assert(platform_control_axes(axes, &recenter) == 0);
    assert(axes[0] < -0.6f && axes[3] < -0.45f && recenter == 1);

    Java_org_sonicr_recomp_GameActivity_nativeTouchAxes(NULL, NULL, 0.19f, 0, 0, 0.8f);
    assert(platform_control_axes(axes, &recenter) == 0);
    assert(axes[0] < -0.6f && axes[3] == 0.8f);
    Java_org_sonicr_recomp_GameActivity_nativeTouchAxes(NULL, NULL, 0.8f, 0, 0, 0.19f);
    assert(platform_control_axes(axes, &recenter) == 1);
    assert(axes[0] == 0.8f && axes[3] < -0.45f);
    Java_org_sonicr_recomp_GameActivity_nativeTouchAxes(NULL, NULL, 0, 0, 0, 0);
    assert(platform_control_axes(axes, &recenter) == 0);
    assert(axes[0] < -0.6f && axes[3] < -0.45f && recenter == 1);

    SDL_GameControllerClose(controller);
    s_pads[0].ctrl = NULL;
    s_padCount = 0;
    SDL_JoystickDetachVirtual(device);
    SDL_Quit();
}

int main(void)
{
    TestTouchAxesAndKeyboardCamera();
    TestControllerAndTouchTakeover();
    puts("Android touch axes passed");
}
