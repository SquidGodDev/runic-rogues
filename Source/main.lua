import "CoreLibs/graphics"
import "CoreLibs/timer"
import "CoreLibs/animator"

import "scripts/globals/helpers"
import "scripts/systems/sceneManager"
import "scripts/systems/AudioManager"
import "scripts/roomSelect/roomSelectScene"

AudioManager.init()

SceneManager.loadNewScene(RoomSelectScene, {})

function NextLevel()
	SceneManager.switchScene(RoomSelectScene)
end