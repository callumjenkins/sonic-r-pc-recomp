#ifndef CPU_OPPONENTS_H
#define CPU_OPPONENTS_H

#include <stdint.h>

typedef struct Player Player;

typedef struct {
    int enabled;
    int colours;
    int characters[4];
} CpuOpponentsOptions;

extern CpuOpponentsOptions g_cpuOpponentsOptions;

enum {
    CPU_OPT_ROSTER = 1006,
    CPU_OPT_CLASSIC = 1007,
    CPU_OPT_COLOURS = 1008,
    CPU_OPT_CLASSIC_COLOURS = 1009
};

CpuOpponentsOptions CpuOpponents_GetOptions(void);
void CpuOpponents_SetOptions(CpuOpponentsOptions options);
int CpuOpponents_ParseRoster(const char *text);
int CpuOpponents_Command(int option, const char *text);
void CpuOpponents_RememberStock(void);
void CpuOpponents_Apply(void);
void CpuOpponents_UpdateAvailability(void);
void CpuOpponents_TintLighting(Player *player);
uint32_t CpuOpponents_TintColour(int variant, uint32_t colour);

#endif
