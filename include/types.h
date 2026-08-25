#pragma once

#include <3ds.h>
#include <stdbool.h>
#include "config.h"

typedef struct { float x, y; } Vec2;
typedef struct { u8 r, g, b; } Color;

typedef struct {
    Vec2 a, b;
    float minx, maxx, miny, maxy;
    u8 kind;
    u8 accent;
    u32 geometryId;
} Segment;

typedef struct {
    int sx, sy;
    int segCount;
    Segment seg[MAX_SEG_PER_SECTOR];
} Sector;

typedef struct { u32 seed; } World;

typedef struct {
    int bloomMode;
    int zoomMode;
    int particleMode;
    int trailMode;
    bool screenShake;
    bool speedFx;
    bool minimalHud;
    int musicVolume;
} Settings;

typedef enum {
    STATE_TITLE = 0,
    STATE_PLAYING,
    STATE_DEAD
} GameState;

typedef struct {
    World world;
    GameState state;
    Vec2 pos;
    Vec2 vel;
    float aimAngle;
    bool aiming;
    bool prevAction;
    float launchFlash;
    float impactFlash;
    float cameraKick;
    float launchSlowTimer;
    int shots;
    u32 touchedGeometry[TOUCHED_GEOMETRY_SLOTS];
    Settings settings;
    bool settingsOpen;
    int settingsIndex;
    bool darknessActive;
    Vec2 darknessPos;
    float darknessSpeed;
    float darknessDanger;
    float shield;
    float maxSpeedSeen;
    float cameraX;
    float cameraY;
    float cameraScale;
    float distance;
    int score;
    int bestScore;
    int launches;
    u32 seed;
    Vec2 trail[TRAIL_POINTS];
    int trailHead;
    int trailCount;
    u64 lastTrailMs;
} Game;
