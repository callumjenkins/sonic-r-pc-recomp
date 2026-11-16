#include "wide_island.h"
#include "wide_island_layout.h"
#include "sonicr_types.h"
#include "sonicr_globals.h"
#include "sonicr_functions.h"
#include "ter_types.h"
#include "vertex_struct.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __ANDROID__
#include <SDL.h>
#include <jni.h>
static SDL_SpinLock s_lock;
#define LOCK() SDL_AtomicLock(&s_lock)
#define UNLOCK() SDL_AtomicUnlock(&s_lock)
#else
#define LOCK() ((void)0)
#define UNLOCK() ((void)0)
#endif

extern int g_terCollisionMeshSize;
extern int g_posDataCount3, g_posDataCount5, g_posDataCount6;
extern unsigned char *GetAIDataBuffer(void);

WideIslandOptions g_wideIslandOptions = {0};
int g_playfieldColumns = WIDE_ISLAND_GRID_COLUMNS;
static WideIslandLayout s_layout;
/* The layout's tile count, for the Android menu thread. */
static int s_activeTiles;
static unsigned char s_stretchedSurfaces[4096 / 8];

/* Decorations narrower than this move whole, so props such as trees keep their shape. */
#define RIGID_DECORATION_WIDTH 1000
/* Room beyond the furthest corner, so integer rounding never leaves a face edge outside its circle. */
#define SURFACE_RADIUS_MARGIN 64

WideIslandOptions WideIsland_GetOptions(void)
{
    LOCK();
    WideIslandOptions options = g_wideIslandOptions;
    UNLOCK();
    return options;
}

void WideIsland_SetOptions(WideIslandOptions options)
{
    if (!WideIslandLayout_ValidTiles(options.tiles)) options.tiles = 0;
    LOCK();
    g_wideIslandOptions = options;
    UNLOCK();
}

int WideIsland_Command(const char *text)
{
    char *end = NULL;
    long tiles = text ? strtol(text, &end, 10) : -1;
    if (!text || !*text || *end || tiles < 0 || tiles > 32 || !WideIslandLayout_ValidTiles((int)tiles)) {
        fprintf(stderr, "--wide-island takes 0, 8, 16, or 32 tiles\n");
        return 1;
    }
    WideIsland_SetOptions((WideIslandOptions){(int)tiles});
    return 0;
}

int WideIslandActive(void)
{
    return s_layout.tiles != 0;
}

int WideIslandChangesCourse(void)
{
    return WideIslandActive() ||
        (WideIsland_GetOptions().tiles && g_trackId == TRACK_RESORT_ISLAND && g_demoMode == DEMO_NONE &&
         !g_netSessionActive);
}

int WideIslandShiftX(int x)
{
    return WideIslandLayout_Shift(&s_layout, x);
}

int WideIslandGridX(int x)
{
    return WideIslandLayout_Unshift(&s_layout, x);
}

int WideIslandWidth(void)
{
    return s_layout.tiles ? s_layout.width : 0;
}

const char *WideIsland_Status(void)
{
    static char status[128];
    LOCK();
    int requested = g_wideIslandOptions.tiles;
    int active = s_activeTiles;
    UNLOCK();
    WideIslandLayout_Status(status, sizeof(status), requested, active);
    return status;
}

static void SetLayout(WideIslandLayout layout)
{
    s_layout = layout;
    if (!layout.tiles) memset(s_stretchedSurfaces, 0, sizeof(s_stretchedSurfaces));
    g_playfieldColumns = layout.tiles ? layout.columns : WIDE_ISLAND_GRID_COLUMNS;
    LOCK();
    s_activeTiles = layout.tiles;
    UNLOCK();
}

void WideIsland_Deactivate(void)
{
    SetLayout((WideIslandLayout){0});
}

static int Straddles(int minX, int maxX)
{
    return minX <= s_layout.cut && maxX > s_layout.cut;
}

static void ShiftObjects(void)
{
    int *obj = (int *)g_objectStructArray;
    for (int i = 0; i < g_totalObjects; i++, obj += 17) {
        short radius = *(short *)((char *)obj + 0x2C);
        short mode = *(short *)((char *)obj + 0x2E);
        SrcVertex *vertex = &g_vertexArrayBase[*(unsigned short *)((char *)obj + 0x38)];
        int count = *(unsigned short *)((char *)obj + 0x3A);
        if (mode == 2 || radius == -1 || count == 0) {
            int east = WideIslandShiftX(mode == 2 ? obj[8] : obj[0]) != (mode == 2 ? obj[8] : obj[0]);
            if (east) {
                obj[0] += s_layout.width;
                obj[8] += s_layout.width;
            }
            continue;
        }
        int minX = vertex[0].posX, maxX = vertex[0].posX;
        for (int v = 1; v < count; v++) {
            if (vertex[v].posX < minX) minX = vertex[v].posX;
            if (vertex[v].posX > maxX) maxX = vertex[v].posX;
        }
        if (!Straddles(minX, maxX) || (mode == 1 && maxX - minX < RIGID_DECORATION_WIDTH)) {
            if (WideIslandShiftX((minX + maxX) / 2) == (minX + maxX) / 2) continue;
            for (int v = 0; v < count; v++) vertex[v].posX += s_layout.width;
            obj[0] += s_layout.width;
            obj[8] += s_layout.width;
            continue;
        }
        double reach = 0;
        for (int v = 0; v < count; v++) {
            vertex[v].posX = WideIslandShiftX(vertex[v].posX);
            double dx = vertex[v].posX - obj[0], dy = vertex[v].posY - obj[1], dz = vertex[v].posZ - obj[2];
            double distance = sqrt(dx * dx + dy * dy + dz * dz);
            if (distance > reach) reach = distance;
        }
        int stretched = (int)ceil(reach);
        if (stretched > 32766) stretched = 32766;
        if (stretched > radius) *(short *)((char *)obj + 0x2C) = (short)stretched;
        DebugLog("WideIsland: stretched %s %d across X %d..%d\n", mode ? "decoration" : "part", i, minX, maxX);
    }
}

static void ShiftPoints(int *points, int count, int stride)
{
    if (!points) return;
    for (int i = 0; i < count; i++) points[i * stride] = WideIslandShiftX(points[i * stride]);
}

static void ShiftShorts(void *base, int count, int stride)
{
    if (!base) return;
    for (int i = 0; i < count; i++) {
        short *x = (short *)((char *)base + i * stride);
        *x = (short)WideIslandShiftX(*x);
    }
}

typedef struct {
    int x, z;
} Point2;

static Point2 s_points[4 * 4096];
int WideIslandStretchedSurface(int surface)
{
    return s_layout.tiles && surface >= 0 && surface < 4096 && (s_stretchedSurfaces[surface / 8] >> (surface % 8)) & 1;
}

/* Re-centres a bounding circle on moved points. It only grows, so a stock check never misses a face.
 * Returns whether the points straddle the strip. */
static int Enclose(int16_t *centerX, int16_t *centerZ, int32_t *radiusSq, const Point2 *points, int count)
{
    int minX = points[0].x, maxX = points[0].x, minZ = points[0].z, maxZ = points[0].z;
    for (int i = 1; i < count; i++) {
        if (points[i].x < minX) minX = points[i].x;
        if (points[i].x > maxX) maxX = points[i].x;
        if (points[i].z < minZ) minZ = points[i].z;
        if (points[i].z > maxZ) maxZ = points[i].z;
    }
    if (!Straddles(minX, maxX)) {
        if (minX > s_layout.cut) *centerX = (int16_t)(*centerX + s_layout.width);
        return 0;
    }
    int cx = (minX + maxX) / 2, cz = (minZ + maxZ) / 2;
    double reach = 0;
    for (int i = 0; i < count; i++) {
        double dx = points[i].x - cx, dz = points[i].z - cz;
        double distance = sqrt(dx * dx + dz * dz);
        if (distance > reach) reach = distance;
    }
    reach += SURFACE_RADIUS_MARGIN;
    *centerX = (int16_t)cx;
    *centerZ = (int16_t)cz;
    if (reach * reach > *radiusSq) *radiusSq = (int32_t)(reach * reach);
    return 1;
}

static TerVertex *FaceVertex(TerFace *face, int corner)
{
    return (TerVertex *)((char *)g_terVertexTable + face->vtxIdx[corner]);
}

/* Normals from the diagonals, length 4096 and on the same side as the stock normal. */
static void Renormal(TerFace *face)
{
    TerVertex *a = FaceVertex(face, 0), *b = FaceVertex(face, 1), *c = FaceVertex(face, 2), *d = FaceVertex(face, 3);
    double ux = c->x - a->x, uy = c->y - a->y, uz = c->z - a->z;
    double vx = d->x - b->x, vy = d->y - b->y, vz = d->z - b->z;
    double nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
    double length = sqrt(nx * nx + ny * ny + nz * nz);
    if (length < 1) return;
    if (nx * face->normalX + ny * face->normalY + nz * face->normalZ < 0) length = -length;
    face->normalX = (int16_t)lround(nx * 4096 / length);
    face->normalY = (int16_t)lround(ny * 4096 / length);
    face->normalZ = (int16_t)lround(nz * 4096 / length);
}

static void ShiftTerrain(void)
{
    TerVertex *vertices = (TerVertex *)g_terVertexTable;
    TerFace *faces = (TerFace *)g_terFaceTable;
    TerSurface *surfaces = (TerSurface *)g_trackSurfaceData;
    TerCollisionMesh *walls = (TerCollisionMesh *)g_terCollisionMesh;
    TerVertex *edges = (TerVertex *)g_terEdgeList;
    if (!vertices || !faces || !surfaces || !walls || !edges) return;
    int vertexCount = (int)(((char *)g_terFaceTable - (char *)g_terVertexTable) / sizeof(TerVertex));
    int faceCount = (int)(((char *)g_trackSurfaceData - (char *)g_terFaceTable) / sizeof(TerFace));
    int surfaceCount = (int)(((char *)g_terCollisionMesh - (char *)g_trackSurfaceData) / sizeof(TerSurface));
    int wallCount = g_terCollisionMeshSize / (int)sizeof(TerCollisionMesh);
    int edgeCount = (int)(((char *)g_terLoopTable - (char *)g_terEdgeList) / sizeof(TerVertex));
    int loopRows = (int)(((char *)g_terGridIndex - (char *)g_terUnknown74) / 34);

    ShiftShorts(&vertices[0].x, vertexCount, sizeof(TerVertex));
    ShiftShorts(&edges[0].x, edgeCount, sizeof(TerVertex));
    for (int corner = 0; corner < 4; corner++)
        ShiftShorts((char *)g_terUnknown74 + corner * 6, loopRows, 34);

    for (int f = 0; f < faceCount; f++) {
        int minX = FaceVertex(&faces[f], 0)->x, maxX = minX;
        for (int k = 1; k < 4; k++) {
            int x = FaceVertex(&faces[f], k)->x;
            if (x < minX) minX = x;
            if (x > maxX) maxX = x;
        }
        if (Straddles(minX, maxX)) Renormal(&faces[f]);
    }
    for (int s = 0; s < surfaceCount && s < 4096; s++) {
        int count = 0;
        for (int f = 0; f < surfaces[s].faceCount && count + 4 <= (int)(sizeof(s_points) / sizeof(s_points[0])); f++) {
            for (int k = 0; k < 4; k++, count++) {
                TerVertex *vertex = FaceVertex(&faces[surfaces[s].faceBase + f], k);
                s_points[count] = (Point2){vertex->x, vertex->z};
            }
        }
        if (count && Enclose(&surfaces[s].centerX, &surfaces[s].centerZ, &surfaces[s].radiusSq, s_points, count))
            s_stretchedSurfaces[s / 8] |= (unsigned char)(1 << (s % 8));
    }
    for (int w = 0; w < wallCount; w++) {
        int count = 0;
        for (int k = 0; k < walls[w].vtxCount; k++, count++)
            s_points[count] = (Point2){edges[walls[w].vtxBase + k].x, edges[walls[w].vtxBase + k].z};
        if (count) Enclose(&walls[w].centerX, &walls[w].centerZ, &walls[w].radiusSq, s_points, count);
    }
}

void WideIsland_Apply(void)
{
    WideIsland_Deactivate();
    int tiles = WideIsland_GetOptions().tiles;
    if (!tiles || g_trackId != TRACK_RESORT_ISLAND || g_demoMode != DEMO_NONE || g_netSessionActive) return;
    WideIslandLayout layout = WideIslandLayout_Make(tiles, g_worldBoundsA, g_worldBoundsB, g_worldBoundsD);
    if (!layout.tiles) return;
    SetLayout(layout);

    ShiftObjects();
    ShiftPoints(g_ringSpawnArray, g_ringCount, 4);
    ShiftTerrain();
    unsigned char *waypoints = GetAIDataBuffer();
    if (waypoints) {
        ShiftShorts(waypoints, 1024, 16);
        ShiftShorts(waypoints + 4, 1024, 16);
    }
    ShiftPoints(g_trackBoundaryWaypoints, g_posDataCount3, 3);
    ShiftPoints((int *)g_splineWaypoints, g_posDataCount4, 3);
    ShiftPoints((int *)g_podiumCenter, g_posDataCount5, 3);
    ShiftPoints(g_waypointTablePtr, g_posDataCount6, 3);

    WideIslandLayout_InsertColumns(&s_layout, g_tileMap);
    g_worldBoundsD = WideIslandLayout_Right(&s_layout, g_worldBoundsA, g_worldBoundsB, g_worldBoundsD);
    DebugLog("WideIsland: %d tiles, cut X %d, width %d\n", s_layout.tiles, s_layout.cut, s_layout.width);
}

#ifdef __ANDROID__
JNIEXPORT void JNICALL Java_org_sonicr_recomp_GameActivity_nativeWideIsland(JNIEnv *env, jclass type, jint tiles)
{
    (void)env;
    (void)type;
    WideIsland_SetOptions((WideIslandOptions){tiles});
}

JNIEXPORT jstring JNICALL Java_org_sonicr_recomp_GameActivity_nativeWideIslandStatus(JNIEnv *env, jclass type)
{
    (void)type;
    return (*env)->NewStringUTF(env, WideIsland_Status());
}
#endif
