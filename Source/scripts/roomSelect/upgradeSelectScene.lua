local pd <const> = playdate
local gfx <const> = pd.graphics

local maxDescWidth <const> = 170
local maxDescHeight <const> = 35

local enFontTitle = gfx.font.new("fonts/m6x11-12")
local enFontSub = gfx.font.new("fonts/m5x7-12")
local jpFontTitle = gfx.font.new("fonts/KH-Dot-Kabutochou-16")
local jpFontSub = gfx.font.new("fonts/KH-Dot-Kodenmachou-12")

local cardLerpSpeed <const> = 0.3
local cardGap <const> = 60
local cardSelectedY <const> = 27
local cardY <const> = 33
local cardImage = gfx.image.new("images/upgrades/card")
local cardWidth, cardHeight = cardImage:getSize()
local cardCursorImage = gfx.image.new("images/upgrades/cardCursor")

UpgradeSelectScene = {}
function UpgradeSelectScene:init()
    gfx.setBackgroundColor(gfx.kColorBlack)
    gfx.clear(gfx.kColorBlack)
    local languageEN = SETTINGS.language == "en"
    local titleFont = languageEN and enFontTitle or jpFontTitle
    local subtitleFont = languageEN and enFontSub or jpFontSub
    self.upgradeOptionNames = {}
    self.upgradeOptionDescriptions = {}
    self.upgradeCards = {}
    self.selectedOption = 2
    for i=1, 3 do
        if not languageEN then
            gfx.setImageDrawMode(gfx.kDrawModeFillWhite)
        end
        local isSelectedOption = i == self.selectedOption
        local name, description, image = Upgrades.getUpgradeOption(i - 1) -- C function from upgradeManager.c
        local nameSprite = Utilities.spriteWithText(name, titleFont)
        nameSprite:moveTo(100, 70)
        nameSprite:setVisible(isSelectedOption)
        nameSprite:add()
        self.upgradeOptionNames[i] = nameSprite

        local descriptionSprite = gfx.sprite.spriteWithText(description, maxDescWidth, maxDescHeight, gfx.kColorClear, nil, nil, kTextAlignment.center, subtitleFont)
        descriptionSprite:moveTo(100, 98)
        descriptionSprite:setVisible(isSelectedOption)
        descriptionSprite:add()
        self.upgradeOptionDescriptions[i] = descriptionSprite

        gfx.setImageDrawMode(gfx.kDrawModeCopy)

        local upgradeCardImage = cardImage:copy()
        gfx.pushContext(upgradeCardImage)
            image:drawCentered(cardWidth / 2, cardHeight / 2)
        gfx.popContext()
        local cardSprite = gfx.sprite.new(upgradeCardImage)
        cardSprite:moveTo(100 + cardGap * (i - 2), isSelectedOption and cardSelectedY or cardY)
        cardSprite:add()
        self.upgradeCards[i] = cardSprite
    end
    local cursor = gfx.sprite.new(cardCursorImage)
    cursor:moveTo(100, cardSelectedY)
    cursor:add()
    self.cursor = cursor
end

function UpgradeSelectScene:displayOptionInfo()
    for i=1, 3 do
        local nameSprite = self.upgradeOptionNames[i]
        local descriptionSprite = self.upgradeOptionDescriptions[i]
        nameSprite:setVisible(i == self.selectedOption)
        descriptionSprite:setVisible(i == self.selectedOption)
    end
end

function UpgradeSelectScene:update()
    gfx.sprite.update()

    if pd.buttonJustPressed(pd.kButtonLeft) and self.selectedOption > 1 then
        self.selectedOption -= 1
        self:displayOptionInfo()
    elseif pd.buttonJustPressed(pd.kButtonRight) and self.selectedOption < 3 then
        self.selectedOption += 1
        self:displayOptionInfo()
    end

    for i=1, 3 do
        local curCard = self.upgradeCards[i]
        local targetY = (i == self.selectedOption) and cardSelectedY or cardY
        curCard:moveTo(curCard.x, math.lerp(curCard.y, targetY, cardLerpSpeed))
    end

    local cursorTargetX = math.lerp(self.cursor.x, 100 + cardGap * (self.selectedOption - 2), cardLerpSpeed)
    self.cursor:moveTo(cursorTargetX, self.cursor.y)
end
