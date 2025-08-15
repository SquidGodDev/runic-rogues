import "CoreLibs/graphics"
import "CoreLibs/object"
import "CoreLibs/timer"
import "CoreLibs/animator"
import "CoreLibs/sprites"

import "scripts/globals/helpers"
import "scripts/systems/sceneManager"
import "scripts/systems/audioManager"
import "scripts/systems/gameManager"
import "scripts/systems/settingsManager"
import "scripts/roomSelect/roomSelectScene"
import "scripts/title/titleScene"
import "scripts/title/gameOverScene"

AudioManager.init()

SceneManager.loadFirstScene(TitleScene)

function NextLevel()
	GAME_DATA.currentRoom += 1
	if GAME_DATA.currentRoom > 10 and GAME_DATA.currentArea >= 2 then
		SceneManager.switchScene(GameOverScene, true)
		return
	end
	if GAME_DATA.currentRoom > 10 then
		GAME_DATA.currentRoom = 1
		GAME_DATA.currentArea += 1
	end
	local roomType = GAME_DATA.areaRooms[GAME_DATA.currentRoom]
	SceneManager.switchScene(GameScene, GAME_DATA.currentArea, GAME_DATA.currentRoom, roomType)
end

function GameOver()
	SceneManager.switchScene(GameOverScene, false)
end