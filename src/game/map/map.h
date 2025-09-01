#pragma once

#include "globals.h"
#include "game/systems/progressionManager.h"

#define MAP_FLOOR 0
#define MAP_WALL 1
#define MAP_GATE 2

extern int map[MAP_WIDTH * MAP_HEIGHT];

Point getPlayerSpawnMapPosition(void);
Point getExitMapPosition(void);
Point getRandomEnemySpawnPosition(void);
Point getBossSpawnPosition(void);
Point getShopMapPosition(void);
void carveOutShopArea(void);
void spawnExit(void);
void getRandomEmptyMapPosition(int *mapX, int *mapY);
void initMap(RoomType roomType);
int isBlocked(int x, int y);
int isClearLine(int x1, int y1, int x2, int y2);
void castRayToWall(float x1, float y1, float x2, float y2, float *targetX, float *targetY);

/* Lua program to create basic map
================================
local width = 20
local height = 12

for h = 1, height do
  local rowString = ""
  for w = 1, width do
	  local cell = "0, "
	  if w == width then
		if h == height then
		  cell = "1"
		else
		  cell = "1,"
		end
	  elseif h == 1 or h == height or w == 1 then
		cell = "1, "
	  end
	  rowString = rowString .. cell
  end
  print(rowString)
end
*/