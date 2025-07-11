local pd <const> = playdate
local gfx <const> = pd.graphics

GameOverScene = {}
function GameOverScene:init(win)
    local gameOverImage = win and gfx.image.new("images/title/gameWonScreen") or gfx.image.new("images/title/gameEndScreen")
    local gameOverSprite = gfx.sprite.new(gameOverImage)
    gameOverSprite:setCenter(0, 0)
    gameOverSprite:moveTo(0, 0)
    gameOverSprite:add()
end

function GameOverScene:update()
    gfx.sprite.update()
    if pd.buttonJustPressed(pd.kButtonA) and not SceneManager.isTransitioning() then
        SceneManager.switchScene(TitleScene)
    end
end