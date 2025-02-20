#define TARGET_EXTENSION 1

#include <stdio.h>
#include <math.h>
#include <float.h>

#define PI 3.141592f
#define P2 PI/2
#define P3 3*PI/2
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

static float distance(float x1, float y1, float x2, float y2)
{
	return sqrtf((y2 - y1) * (y2 - y1) + (x2 - x1) * (x2 - x1));
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
	playerX = 100;
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

static void drawRays(void)
{
	int maxFieldDepth = 8;
	int rayCount = 50;
	float rayInterval = 2*PI/360;
	float rayAngle = playerAngle - rayInterval * rayCount / 2;

	for (int ray = 0; ray < rayCount; ray++)
	{
		if (rayAngle < 0)
		{
			rayAngle += 2*PI;
		}
		else if (rayAngle > 2*PI)
		{
			rayAngle -= 2*PI;
		}

		float rayX = playerX;
		float rayY = playerY;
		float xOffset = 0;
		float yOffset = 0;
		int depthOfField = 0;

		// === Check for horizontal lines ===
		float horizontalRayX, horizontalRayY;
		float horizontalDistance = 1000000;
		float rayPerpendicularSlope = -1 / tanf(rayAngle);

		if (rayAngle == 0 || rayAngle == PI)
		{
			// Looking straight left or right
			depthOfField = maxFieldDepth;
			rayX = playerX;
			rayY = playerY;
		}
		else if (rayAngle > PI)
		{
			// Looking up
			rayY = floorf(playerY / cellSize) * cellSize - 0.01f;
			rayX = (playerY - rayY) * rayPerpendicularSlope + playerX;
			yOffset = -cellSize;
			xOffset = -yOffset * rayPerpendicularSlope;
		}
		else if (rayAngle < PI && rayAngle > 0)
		{
			// Looking down
			rayY = floorf(playerY / cellSize) * cellSize + cellSize;
			rayX = (playerY - rayY) * rayPerpendicularSlope + playerX;
			yOffset = cellSize;
			xOffset = -yOffset * rayPerpendicularSlope;
		}

		while (depthOfField < maxFieldDepth)
		{
			int mapX = (int)(rayX / cellSize);
			int mapY = (int)(rayY / cellSize);
			int mapIndex = mapY * mapWidth + mapX;
			if (mapIndex >= 0 && mapIndex < mapWidth * mapHeight && map[mapIndex] == 1)
			{
				depthOfField = maxFieldDepth; // Hit wall
				horizontalDistance = distance(playerX, playerY, rayX, rayY);
				horizontalRayX = rayX;
				horizontalRayY = rayY;
			}
			else
			{
				rayX += xOffset;
				rayY += yOffset;
				depthOfField += 1;
			}
		}

		// === Check for vertical lines ===
		depthOfField = 0;
		float verticalRayX, verticalRayY;
		float verticalDistance = 1000000;
		float rayNegSlope = -tanf(rayAngle);

		if (rayAngle == P2 || rayAngle == P3)
		{
			// Looking straight up or down
			depthOfField = maxFieldDepth;
			rayX = playerX;
			rayY = playerY;
		}
		else if (rayAngle > P2 && rayAngle < P3)
		{
			// Looking left
			rayX = floorf(playerX / cellSize) * cellSize - 0.01f;
			rayY = (playerX - rayX) * rayNegSlope + playerY;
			xOffset = -cellSize;
			yOffset = -xOffset * rayNegSlope;
		}
		else if (rayAngle < P2 || rayAngle > P3)
		{
			// Looking right
			rayX = floorf(playerX / cellSize) * cellSize + cellSize;
			rayY = (playerX - rayX) * rayNegSlope + playerY;
			xOffset = cellSize;
			yOffset = -xOffset * rayNegSlope;
		}

		while (depthOfField < maxFieldDepth)
		{
			int mapX = (int)(rayX / cellSize);
			int mapY = (int)(rayY / cellSize);
			int mapIndex = mapY * mapWidth + mapX;
			if (mapIndex >= 0 && mapIndex < mapWidth * mapHeight && map[mapIndex] == 1)
			{
				depthOfField = maxFieldDepth; // Hit wall
				verticalDistance = distance(playerX, playerY, rayX, rayY);
				verticalRayX = rayX;
				verticalRayY = rayY;
			}
			else
			{
				rayX += xOffset;
				rayY += yOffset;
				depthOfField += 1;
			}
		}

		float rayDistance = 0;
		if (verticalDistance < horizontalDistance)
		{
			rayX = verticalRayX;
			rayY = verticalRayY;
			rayDistance = verticalDistance;
		}
		else if (horizontalDistance < verticalDistance)
		{
			rayX = horizontalRayX;
			rayY = horizontalRayY;
			rayDistance = horizontalDistance;
		}

		pd->graphics->drawLine((int)playerX, (int)playerY, (int)rayX, (int)rayY, 1, kColorWhite);

		// === Draw 3D Walls ===
		int screenWidth = 200;
		int screenHeight = 240;
		float lineHeight = (cellSize * screenHeight) / rayDistance;
		if (lineHeight > screenHeight)
		{
			lineHeight = screenHeight;
		}
		float lineOffset = screenHeight / 2 - lineHeight / 2;

		int rayWidth = screenWidth / rayCount;
		int renderStartX = 200;
		int wallX = ray*rayWidth + renderStartX;
		pd->graphics->drawLine(wallX, lineOffset, wallX, lineHeight + lineOffset, rayWidth, kColorWhite);

		rayAngle += rayInterval;
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
