import "CoreLibs/graphics"

local pd = playdate
local gfx = pd.graphics

local gameFont = gfx.font.new("fonts/m5x7-12-outlined")
gfx.setFont(gameFont)

local GAME_STATE = {
	ACTIVE = 0,
	GAME_OVER = 1
}

local gameState = GAME_STATE.ACTIVE
local score

function pd.update()
	if gameState == GAME_STATE.ACTIVE then
		game.update()
	elseif gameState == GAME_STATE.GAME_OVER then
		gfx.drawTextAligned("GAME OVER", 100, 50, kTextAlignment.center)
		gfx.drawTextAligned("Score: " .. score, 100, 70, kTextAlignment.center)

		if pd.buttonJustPressed(pd.kButtonA) then
			gameState = GAME_STATE.ACTIVE
			game.init()
		end
	end
end

function GameOver(enemiesKilled)
	score = enemiesKilled
	gameState = GAME_STATE.GAME_OVER
end