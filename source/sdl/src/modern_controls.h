#ifndef MODERN_CONTROLS_H
#define MODERN_CONTROLS_H

typedef struct Player Player;

typedef struct {
    int movement;
    int camera;
} ModernControlsOptions;

extern ModernControlsOptions g_modernControlsOptions;

void ModernControlsReset(void);
void ModernControlsUpdate(void);
int ModernControlsChangesMovement(void);
int ModernControlsMovementActive(Player *player);
unsigned short ModernControlsButtons(Player *player, unsigned short buttons);
int ModernControlsSteer(Player *player);
float ModernControlsStrength(void);
int ModernControlsCameraYaw(Player *player, int classicYaw);
int ModernControlsCameraHeight(Player *player);

#endif
