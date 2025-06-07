import "CoreLibs/graphics"
import "CoreLibs/timer"
import "CoreLibs/animator"

import "scripts/globals/helpers"
import "scripts/systems/sceneManager"
import "scripts/roomSelect/roomSelectScene"

local pd = playdate
local gfx = pd.graphics

SceneManager.loadNewScene(RoomSelectScene, {})

function NextLevel()
	SceneManager.switchScene(RoomSelectScene)
end