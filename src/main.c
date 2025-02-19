#define TARGET_EXTENSION 1

#include <stdio.h>
#include <math.h>
#define PI 3.141592f
#define TARGET_FPS 30

#include "pd_api.h"
static PlaydateAPI* pd = NULL;

static void drawCircleAtPoint(float x, float y, int radius)
{
	int drawX = (int)x - radius;
	int drawY = (int)y - radius;
	int width = radius * 2;
	int height = radius * 2;
	pd->graphics->drawEllipse(drawX, drawY, width, height, 1, 0, 0, kColorWhite);
}

static int cellSize = 20;
static int mapWidth = 10;
static int mapHeight = 12;
static int map[] =
{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 0, 0, 1, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 1, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 1, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 1, 1, 1, 1, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 1, 1, 1, 0, 0, 1, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 1, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 1, 0, 0, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

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

static float playerX, playerY;
static int playerRadius;
static float playerAngle;
static float playerTurnSpeed;
static float playerMaxSpeed;
static float playerXVelocity, playerYVelocity;

static void gameInit(void)
{
	playerX = 200;
	playerY = 120;
	playerRadius = 6;
	playerAngle = 0;
	playerTurnSpeed = 0.1f * TARGET_FPS;
	playerMaxSpeed = 2.0f * TARGET_FPS;
	playerXVelocity = cosf(playerAngle) * playerMaxSpeed;
	playerYVelocity = sinf(playerAngle) * playerMaxSpeed;
}

static void drawMap(void)
{
	pd->graphics->fillRect(0, 0, mapWidth * cellSize, mapHeight * cellSize, kColorWhite);
	for(int y = 0; y < mapHeight; y++)
	{
		for(int x = 0; x < mapWidth; x++)
		{
			int cellValue = map[y * mapWidth + x];
			int drawX = x * cellSize;
			int drawY = y * cellSize;
			if (cellValue == 1)
			{
				pd->graphics->drawRect(drawX, drawY, cellSize, cellSize, kColorBlack);
			}
			else
			{
				pd->graphics->fillRect(drawX, drawY, cellSize, cellSize, kColorBlack);
			}
		}
	}
}

static void drawPlayer(void)
{
	drawCircleAtPoint(playerX, playerY, playerRadius);
	
	int angleIndicatorX = (int)(playerX + playerXVelocity/4);
	int angleIndicatorY = (int)(playerY + playerYVelocity/4);
	pd->graphics->drawLine((int)playerX, (int)playerY, angleIndicatorX, angleIndicatorY, 2, kColorWhite);
}

void drawRays(void)
{
	int maxFieldDepth = 8;
	float rayAngle = playerAngle;
	int rayCount = 1;
	for (int ray = 0; ray < rayCount; ray++)
	{
		// === Check for horizontal lines ===
		float rayX = playerX;
		float rayY = playerY;
		float xOffset = 0;
		float yOffset = 0;
		int depthOfField = 0;
		float raySlope = -1 / tanf(rayAngle);

		if (rayAngle > PI)
		{
			// Looking up
			rayY = floorf(playerY / cellSize) * cellSize - 0.01f;
			rayX = (playerY - rayY) * raySlope + playerX;
			yOffset = -cellSize;
			xOffset = -yOffset * raySlope;
		}
		else if (rayAngle < PI && rayAngle > 0)
		{
			// Looking down
			rayY = floorf(playerY / cellSize) * cellSize + cellSize;
			rayX = (playerY - rayY) * raySlope + playerX;
			yOffset = cellSize;
			xOffset = -yOffset * raySlope;
		}
		else
		{
			// Looking straight left or right
			depthOfField = maxFieldDepth;
		}

		while (depthOfField < maxFieldDepth)
		{
			int mapX = (int)(rayX / cellSize);
			int mapY = (int)(rayY / cellSize);
			int mapIndex = mapY * mapWidth + mapX;
			if (mapIndex < mapWidth * mapHeight && map[mapIndex] == 1)
			{
				depthOfField = maxFieldDepth; // Hit wall
			}
			else
			{
				rayX += xOffset;
				rayY += yOffset;
				depthOfField += 1;
			}
		}

		pd->graphics->drawLine((int)playerX, (int)playerY, (int)rayX, (int)rayY, 1, kColorWhite);
	}
}

static void handleInput(float deltaTime)
{
	// char str_num[50];
	// sprintf(str_num, "%f", deltaTime);
	// pd->system->logToConsole(str_num);

	PDButtons current;
	pd->system->getButtonState(&current, NULL, NULL);

	if (current & kButtonLeft)
	{ 
		playerAngle -= playerTurnSpeed * deltaTime;
		if (playerAngle < 0)
		{
			playerAngle += 2 * PI;
		}
		playerXVelocity = cosf(playerAngle) * playerMaxSpeed;
		playerYVelocity = sinf(playerAngle) * playerMaxSpeed;
	}
	else if (current & kButtonRight)
	{ 
		playerAngle += playerTurnSpeed * deltaTime;
		if (playerAngle > 2 * PI)
		{
			playerAngle -= 2 * PI;
		}
		playerXVelocity = cosf(playerAngle) * playerMaxSpeed;
		playerYVelocity = sinf(playerAngle) * playerMaxSpeed;
	}

	if (current & kButtonUp)
	{
		playerX += playerXVelocity * deltaTime;
		playerY += playerYVelocity * deltaTime;
	}
	else if (current & kButtonDown)
	{
		playerX -= playerXVelocity * deltaTime;
		playerY -= playerYVelocity * deltaTime;
	}
}

static int gameUpdate(lua_State* L)
{
	float deltaTime = pd->system->getElapsedTime();
	pd->system->resetElapsedTime();

	// pd->sprite->updateAndDrawSprites();
	pd->graphics->clear(kColorBlack);

	handleInput(deltaTime);
	drawMap();
	drawPlayer();
	drawRays();

	pd->lua->pushNil();
	return 1;
}

int eventHandler(PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg)
{
	if (event == kEventInitLua)
	{
		pd = playdate;
		pd->system->resetElapsedTime();

		gameInit();

		pd->lua->addFunction(gameUpdate, "game.update", NULL);
	}

	return 0;
}
