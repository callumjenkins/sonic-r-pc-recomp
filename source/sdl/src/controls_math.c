#include "controls_math.h"
#include <math.h>

ControlsVector ControlsStick(float x, float y)
{
    float magnitude = hypotf(x, y);
    if (magnitude <= 0.2f) {
        return (ControlsVector){0, 0};
    }
    float strength = (fminf(magnitude, 1.0f) - 0.2f) / 0.8f;
    return (ControlsVector){x * strength / magnitude, y * strength / magnitude};
}

int ControlsHeading(ControlsVector movement, int cameraYaw)
{
    double angle = atan2(movement.x, movement.y);
    return (cameraYaw + (int)lround(angle * (4096.0 / 6.283185307179586))) & 4095;
}

int ControlsTurnToward(int current, int target, int step)
{
    int delta = (target - current) & 4095;
    if (delta > 2048) {
        delta -= 4096;
    }
    if (delta > step) {
        delta = step;
    }
    if (delta < -step) {
        delta = -step;
    }
    return (current + delta) & 4095;
}
