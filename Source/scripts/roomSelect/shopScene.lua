local pd <const> = playdate
local gfx <const> = pd.graphics

local maxDescWidth <const> = 190
local maxDescHeight <const> = 45

local enFontTitle = gfx.font.new("fonts/m6x11-12")
local enFontSub = gfx.font.new("fonts/m5x7-12")
local jpFontTitle = gfx.font.new("fonts/KH-Dot-Kabutochou-16")
local jpFontSub = gfx.font.new("fonts/KH-Dot-Kodenmachou-12")

local cardLerpSpeed <const> = 0.3
local cardGap <const> = 60
local cardSelectedY <const> = 27
local cardY <const> = 33
local cardBackgroundImage = gfx.image.new("images/shop/cardBackground")
local cardWidth, cardHeight = cardBackgroundImage:getSize()
local cardCursorImage = gfx.image.new("images/shop/cardCursor")

ShopScene = {}
function ShopScene:init()
    gfx.setBackgroundColor(gfx.kColorBlack)
    gfx.clear(gfx.kColorBlack)
    local languageEN = SETTINGS.language == "en"
    local titleFont = languageEN and enFontTitle or jpFontTitle
    local subtitleFont = languageEN and enFontSub or jpFontSub
    self.upgradeOptionNames = {}
    self.upgradeOptionDescriptions = {}
    self.upgradeCards = {}
    self.selectedOption = 2
    Shop.populateOptions(); -- C function from shopManager.c
    for i=1, 3 do
        if not languageEN then
            gfx.setImageDrawMode(gfx.kDrawModeFillWhite)
        end
        local isSelectedOption = i == self.selectedOption
        local name, description, image = Shop.getSpellOption(i - 1) -- C function from shopManager.c
        local nameSprite = Utilities.spriteWithText(name, titleFont)
        nameSprite:moveTo(100, 65)
        nameSprite:setVisible(isSelectedOption)
        nameSprite:add()
        self.upgradeOptionNames[i] = nameSprite

        local descriptionSprite = gfx.sprite.spriteWithText(description, maxDescWidth, maxDescHeight, gfx.kColorClear, nil, nil, kTextAlignment.center, subtitleFont)
        descriptionSprite:moveTo(100, 95)
        descriptionSprite:setVisible(isSelectedOption)
        descriptionSprite:add()
        self.upgradeOptionDescriptions[i] = descriptionSprite

        gfx.setImageDrawMode(gfx.kDrawModeCopy)

        local upgradeCardImage = cardBackgroundImage:copy()
        gfx.pushContext(upgradeCardImage)
            image:drawCentered(cardWidth / 2, cardHeight / 2)
        gfx.popContext()
        local cardSprite = gfx.sprite.new(upgradeCardImage)
        cardSprite:setScale(2.0)
        cardSprite:moveTo(100 + cardGap * (i - 2), isSelectedOption and cardSelectedY or cardY)
        cardSprite:add()
        self.upgradeCards[i] = cardSprite
    end
    local cursor = gfx.sprite.new(cardCursorImage)
    cursor:moveTo(100, cardSelectedY)
    cursor:add()
    self.cursor = cursor
end

function ShopScene:displayOptionInfo()
    for i=1, 3 do
        local nameSprite = self.upgradeOptionNames[i]
        local descriptionSprite = self.upgradeOptionDescriptions[i]
        nameSprite:setVisible(i == self.selectedOption)
        descriptionSprite:setVisible(i == self.selectedOption)
    end
end

function ShopScene:update()
    gfx.sprite.update()

    if SceneManager.isTransitioning() then
        return
    end

    if pd.buttonJustPressed(pd.kButtonLeft) and self.selectedOption > 1 then
        AudioManager.playSFX(SFX.CursorNavigate)
        self.selectedOption -= 1
        self:displayOptionInfo()
    elseif pd.buttonJustPressed(pd.kButtonRight) and self.selectedOption < 3 then
        AudioManager.playSFX(SFX.CursorNavigate)
        self.selectedOption += 1
        self:displayOptionInfo()
    end

    if pd.buttonJustPressed(pd.kButtonA) then
        local upgradeIndex = self.selectedOption - 1
        Shop.selectSpellOption(upgradeIndex) -- C function from shopManager.c
        NextLevel()
    end

    for i=1, 3 do
        local curCard = self.upgradeCards[i]
        local targetY = (i == self.selectedOption) and cardSelectedY or cardY
        curCard:moveTo(curCard.x, math.lerp(curCard.y, targetY, cardLerpSpeed))
    end

    local cursorTargetX = math.lerp(self.cursor.x, 100 + cardGap * (self.selectedOption - 2), cardLerpSpeed)
    self.cursor:moveTo(cursorTargetX, self.cursor.y)
end
