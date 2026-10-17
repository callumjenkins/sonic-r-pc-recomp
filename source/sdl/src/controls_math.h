#ifndef CONTROLS_MATH_H
#define CONTROLS_MATH_H

typedef struct {
    float x;
    float y;
} ControlsVector;

ControlsVector ControlsStick(float x, float y);
int ControlsHeading(ControlsVector movement, int cameraYaw);
int ControlsTurnToward(int current, int target, int step);

#endif
