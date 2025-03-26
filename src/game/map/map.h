#pragma once

#include "globals.h"

extern int map[MAP_WIDTH * MAP_HEIGHT];

void initMap(void);
int isBlocked(int x, int y);
int isClearLine(int x1, int y1, int x2, int y2);

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