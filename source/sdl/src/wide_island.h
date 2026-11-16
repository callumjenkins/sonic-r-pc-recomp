#ifndef WIDE_ISLAND_H
#define WIDE_ISLAND_H

typedef struct {
    int tiles;
} WideIslandOptions;

enum {
    WIDE_ISLAND_OPT = 1010
};

extern WideIslandOptions g_wideIslandOptions;
extern int g_playfieldColumns;

WideIslandOptions WideIsland_GetOptions(void);
void WideIsland_SetOptions(WideIslandOptions options);
int WideIsland_Command(const char *text);
/* Widens the loaded Resort Island when the option is on, after the terrain header is parsed. */
void WideIsland_Apply(void);
void WideIsland_Deactivate(void);
int WideIslandActive(void);
/* True when the current or next race runs on a widened course. */
int WideIslandChangesCourse(void);
int WideIslandShiftX(int x);
/* Maps a world X on the widened course to the stock course, for the stock collision grid. */
int WideIslandGridX(int x);
int WideIslandWidth(void);
/* True for a collision surface that crosses the strip. */
int WideIslandStretchedSurface(int surface);
const char *WideIsland_Status(void);

#endif
