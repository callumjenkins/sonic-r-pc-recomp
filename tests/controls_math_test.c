#include "controls_math.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static int close_to(float a, float b)
{
    return fabsf(a - b) < 0.0001f;
}

int main(void)
{
    ControlsVector centre = ControlsStick(0.1f, -0.1f);
    assert(centre.x == 0 && centre.y == 0);
    assert(ControlsStick(0.2f, 0).x == 0);
    assert(ControlsStick(0.201f, 0).x > 0);
    ControlsVector half = ControlsStick(0.6f, 0);
    assert(close_to(half.x, 0.5f));
    ControlsVector diagonal = ControlsStick(1, 1);
    assert(close_to(hypotf(diagonal.x, diagonal.y), 1));
    assert(close_to(ControlsStick(-1, 0).x, -1));
    assert(ControlsHeading((ControlsVector){0, 1}, 0) == 0);
    assert(ControlsHeading((ControlsVector){0, 1}, 1024) == 1024);
    assert(ControlsHeading((ControlsVector){0, 1}, 2048) == 2048);
    assert(ControlsHeading((ControlsVector){0, 1}, 3072) == 3072);
    assert(ControlsHeading((ControlsVector){1, 0}, 0) == 1024);
    assert(ControlsHeading((ControlsVector){-1, 0}, 0) == 3072);
    assert(ControlsHeading((ControlsVector){0, -1}, 0) == 2048);
    assert(ControlsHeading((ControlsVector){1, 1}, 0) == 512);
    assert(ControlsTurnToward(4090, 6, 20) == 6);
    assert(ControlsTurnToward(6, 4090, 20) == 4090);
    assert(ControlsTurnToward(4090, 100, 10) == 4);
    assert(ControlsTurnToward(0, 2048, 96) == 96);
    assert(ControlsTurnToward(0, 1024, 0) == 0);
    puts("controls math passed");
}
