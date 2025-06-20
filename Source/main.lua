import "CoreLibs/graphics"
import "CoreLibs/object"
import "CoreLibs/timer"
import "CoreLibs/animator"

import "scripts/globals/helpers"
import "scripts/systems/sceneManager"
import "scripts/systems/AudioManager"
import "scripts/systems/GameManager"
import "scripts/roomSelect/roomSelectScene"
import "scripts/title/titleScene"
import "scripts/title/gameOverScene"

AudioManager.init()

SceneManager.loadNewScene(TitleScene, {})

function NextLevel(playerHealth)
	GAME_DATA.currentRoom += 1
	if GAME_DATA.currentRoom >= 10 then
		GAME_DATA.currentRoom = 1
		GAME_DATA.currentArea += 1
	end
	GAME_DATA.playerHealth = playerHealth
	SceneManager.switchScene(RoomSelectScene)
end

function GameOver()
	SceneManager.switchScene(GameOverScene)
end