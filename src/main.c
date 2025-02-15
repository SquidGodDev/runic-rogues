#define TARGET_EXTENSION 1
#ifdef _WINDLL
__declspec(dllexport)
#endif

#include "pd_api.h"
static PlaydateAPI* pd = NULL;

void drawCircleAtPoint(int x, int y, int radius)
{
	int drawX = x - radius;
	int drawY = y - radius;
	int width = radius * 2;
	int height = radius * 2;
	pd->graphics->drawEllipse(drawX, drawY, width, height, 1, 0, 0, kColorWhite);
}

int playerX = 100, playerY = 120;
int playerRadius = 8;
int speed = 3;

int cellSize = 20;
int mapWidth = 10;
int mapHeight = 12;
int map[] =
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

// Lua program to create basic map
// ================================
// local width = 20
// local height = 12

// for h = 1, height do
//   local rowString = ""
//   for w = 1, width do
//       local cell = "0, "
//       if w == width then
//         if h == height then
//           cell = "1"
//         else
//           cell = "1,"
//         end
//       elseif h == 1 or h == height or w == 1 then
//         cell = "1, "
//       end
//       rowString = rowString .. cell
//   end
//   print(rowString)
// end

static void gameInit()
{
	playerX = 200;
	playerY = 120;
}

void drawMap()
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

void drawPlayer()
{
	drawCircleAtPoint(playerX, playerY, playerRadius);
}

void handleInput()
{
	PDButtons current;
	pd->system->getButtonState(&current, NULL, NULL);

	if (current & kButtonLeft) { 
		playerX -= speed;
	} else if (current & kButtonRight) { 
		playerX += speed;
	}

	if (current & kButtonUp) {
		playerY -= speed;
	} else if (current & kButtonDown) {
		playerY += speed;
	}
}

static int gameUpdate(lua_State* L)
{
	// pd->sprite->updateAndDrawSprites();
	pd->graphics->clear(kColorBlack);

	handleInput();
	drawMap();
	drawPlayer();

	pd->lua->pushNil();
	return 1;
}

int eventHandler(PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg)
{
	if (event == kEventInitLua)
	{
		pd = playdate;

		const char* err;
		pd->lua->addFunction(gameUpdate, "game.update", &err);
	}

	return 0;
}
