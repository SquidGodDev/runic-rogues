#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"

int map[MAP_WIDTH * MAP_HEIGHT];

void initMap(void) {
    int mapData[] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    };
    memcpy(map, mapData, sizeof(mapData));
}

int isBlocked(int x, int y) {
    return map[y * MAP_WIDTH + x] != 0;
}

// From James Howard's Catacombs of the Damned under MIT license
int isClearLine(int x1, int y1, int x2, int y2) {
    int cellX1 = x1 / CELL_SIZE;
	int cellX2 = x2 / CELL_SIZE;
	int cellY1 = y1 / CELL_SIZE;
	int cellY2 = y2 / CELL_SIZE;

    int xDist = ABS(cellX2 - cellX1);

	int partial, delta;
	int deltaFrac;
	int xFrac, yFrac;
	int xStep, yStep;
	int32_t oppositeAxisStep;
	int x, y;

    if (xDist > 0)
    {
        if (cellX2 > cellX1)
        {
            partial = (CELL_SIZE * (cellX1 + 1) - x1);
            xStep = 1;
        }
        else
        {
            partial = (x1 - CELL_SIZE * (cellX1));
            xStep = -1;
        }

        deltaFrac = ABS(x2 - x1);
        delta = y2 - y1;
        oppositeAxisStep = ((int32_t)delta * CELL_SIZE) / deltaFrac;
        if (oppositeAxisStep > 0x7fffl) {
            yStep = 0x7fff;
        } else if (oppositeAxisStep < -0x7fffl) {
            yStep = -0x7fff;
        } else {
            yStep = oppositeAxisStep;
        }
        yFrac = y1 + (((int32_t)yStep*partial) / CELL_SIZE);

        x = cellX1 + xStep;
        cellX2 += xStep;
        do
        {
            y = (yFrac) / CELL_SIZE;
            yFrac += yStep;

			if (isBlocked(x, y)) {
				return 0;
            }

            x += xStep;

        } while (x != cellX2);
    }

    int yDist = ABS(cellY2 - cellY1);

    if (yDist > 0)
    {
        if (cellY2 > cellY1)
        {
            partial = (CELL_SIZE * (cellY1 + 1) - y1);
            yStep = 1;
        }
        else
        {
            partial = (y1 - CELL_SIZE * (cellY1));
            yStep = -1;
        }

        deltaFrac = ABS(y2 - y1);
        delta = x2 - x1;
        oppositeAxisStep = ((int32_t)delta * CELL_SIZE) / deltaFrac;
        if (oppositeAxisStep > 0x7fffl) {
            xStep = 0x7fff;
        }
        else if (oppositeAxisStep < -0x7fffl) {
            xStep = -0x7fff;
        }
        else {
            xStep = oppositeAxisStep;
        }
        xFrac = x1 + (((int32_t)xStep * partial) / CELL_SIZE);

        y = cellY1 + yStep;
        cellY2 += yStep;
        do
        {
            x = (xFrac) / CELL_SIZE;
            xFrac += xStep;

			if (isBlocked(x, y)) {
				return 0;
            }
            y += yStep;

        } while (y != cellY2);
    }

    return 1;
}

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