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
import "scripts/roomSelect/upgradeSelectScene"
import "scripts/title/titleScene"
import "scripts/title/gameOverScene"

AudioManager.init()

SceneManager.loadFirstScene(TitleScene)

function NextLevel(playerHealth)
	GAME_DATA.currentRoom += 1
	if GAME_DATA.currentRoom > 10 and GAME_DATA.currentArea >= 2 then
		SceneManager.switchScene(GameOverScene, true)
		return
	end
	if GAME_DATA.currentRoom > 10 then
		GAME_DATA.currentRoom = 1
		GAME_DATA.currentArea += 1
	end
	GAME_DATA.playerHealth = playerHealth
	SceneManager.switchScene(RoomSelectScene)
end

function GameOver()
	SceneManager.switchScene(GameOverScene, false)
end