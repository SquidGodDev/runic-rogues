#pragma once

#include "globals/globals.h"
#include "game/systems/progressionManager.h"

#define MAP_FLOOR 0
#define MAP_WALL 1
#define MAP_GATE 2
#define MAP_CHASM 3

#define DUNGEON_ROOM_COUNT 6
#define NON_BOSS_ROOM_COUNT (DUNGEON_ROOM_COUNT - 1)

extern int map[MAP_WIDTH * MAP_HEIGHT];

Point getPlayerSpawnMapPosition(void);
Point getExitMapPosition(void);
Point getShopMapPosition(void);
void spawnExit(void);
void getRandomEmptyMapPosition(int *mapX, int *mapY);
void initMap(void);
void setMapCell(int x, int y, int value);
int isBlocked(int x, int y);
int isOpaque(int x, int y);
int isClearLine(int x1, int y1, int x2, int y2);
void castRayToWall(float x1, float y1, float x2, float y2, float *targetX, float *targetY);
Point checkRayCollision(float startX, float startY, float angle, float distance);

int getDungeonRoomCount(void);
Point getRoomCenter(int roomIndex);
Point getRoomWorldCenter(int roomIndex);
Point getRoomInteriorMin(int roomIndex);
Point getRoomInteriorMax(int roomIndex);
Point getRoomSpawnPosition(int roomIndex, int spawnSlot);
void closeEntranceGate(void);
void openRoomGate(int gateIndex);
void closeRoomGate(int gateIndex);
int getPlayerCurrentRoom(void);
