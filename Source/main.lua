import "CoreLibs/graphics"
import "CoreLibs/timer"

import "scripts/systems/sceneManager"

local pd = playdate
local gfx = pd.graphics

SceneManager.loadNewScene(GameScene, {})

function NextLevel()
	SceneManager.switchScene(GameScene)
end