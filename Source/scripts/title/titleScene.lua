local pd <const> = playdate
local gfx <const> = pd.graphics

TitleScene = {}
function TitleScene:init()
    local titleImage = gfx.image.new("images/title/titleScreen")
    local titleSprite = gfx.sprite.new(titleImage)
    titleSprite:setCenter(0, 0)
    titleSprite:moveTo(0, 0)
    titleSprite:add()
end

function TitleScene:update()
    gfx.sprite.update()
    if pd.buttonJustPressed(pd.kButtonA) and not SceneManager.isTransitioning() then
        GameManager:init()
        SceneManager.switchScene(RoomSelectScene)
    end
end