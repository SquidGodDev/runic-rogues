import "CoreLibs/graphics"
import "CoreLibs/object"
import "CoreLibs/timer"
import "CoreLibs/animator"

import "scripts/globals/helpers"
import "scripts/systems/sceneManager"
import "scripts/systems/AudioManager"
import "scripts/systems/GameManager"
import "scripts/roomSelect/roomSelectScene"

AudioManager.init()
GameManager:init()

SceneManager.loadNewScene(RoomSelectScene, {})

function NextLevel(playerHealth)
	GAME_DATA.currentRoom += 1
	if GAME_DATA.currentRoom >= 10 then
		GAME_DATA.currentRoom = 1
		GAME_DATA.currentArea += 1
	end
	GAME_DATA.playerHealth = playerHealth
	SceneManager.switchScene(RoomSelectScene)
end