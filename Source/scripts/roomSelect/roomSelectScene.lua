local pd <const> = playdate
local gfx <const> = playdate.graphics

local enemyIcon <const> = gfx.image.new("images/roomSelect/mapIcons/enemyIcon")
local eliteIcon <const> = gfx.image.new("images/roomSelect/mapIcons/eliteIcon")
local shopIcon <const> = gfx.image.new("images/roomSelect/mapIcons/shopIcon")
local campfireIcon <const> = gfx.image.new("images/roomSelect/mapIcons/campfireIcon")
local bossIcon <const> = gfx.image.new("images/roomSelect/mapIcons/bossIcon")

local enemyTile <const> = gfx.image.new("images/roomSelect/tiles/enemyTile")
local eliteTile <const> = gfx.image.new("images/roomSelect/tiles/eliteTile")
local shopTile <const> = gfx.image.new("images/roomSelect/tiles/shopTile")
local campfireTile <const> = gfx.image.new("images/roomSelect/tiles/campfireTile")
local bossTile <const> = gfx.image.new("images/roomSelect/tiles/bossTile")

local ROOM_TYPE <const> = {
    ENEMY = 0,
    ELITE = 1,
    SHOP = 2,
    CAMPFIRE = 3,
    BOSS = 4
}

local roomTypeMapIcons = {
    [ROOM_TYPE.ENEMY] = enemyIcon,
    [ROOM_TYPE.ELITE] = eliteIcon,
    [ROOM_TYPE.SHOP] = shopIcon,
    [ROOM_TYPE.CAMPFIRE] = campfireIcon,
    [ROOM_TYPE.BOSS] = bossIcon
}

local roomTypeTiles = {
    [ROOM_TYPE.ENEMY] = enemyTile,
    [ROOM_TYPE.ELITE] = eliteTile,
    [ROOM_TYPE.SHOP] = shopTile,
    [ROOM_TYPE.CAMPFIRE] = campfireTile,
    [ROOM_TYPE.BOSS] = bossTile
}

local MAP_BASE_X <const> = 5
local MAP_BASE_Y <const> = 19
local MAP_BORDER_WIDTH <const> = 190
local MAP_BORDER_HEIGHT <const> = 41
local MAP_ICON_BASE_X <const> = 14
local MAP_ICON_BASE_Y <const> = 21
local MAP_ICON_X_SPACING <const> = 18
local MAP_ICON_Y_SPACING <const> = 8
local MAP_ICON_CURSOR_OFFSET <const> = 6
local MAP_ICON_CURSOR_TOP_BASE_TARGET_Y <const> = MAP_BASE_Y + MAP_ICON_BASE_Y - MAP_ICON_CURSOR_OFFSET - 1
local MAP_ICON_CURSOR_BOTTOM_BASE_TARGET_Y <const> = MAP_BASE_Y + MAP_ICON_BASE_Y + MAP_ICON_CURSOR_OFFSET
local MAP_ICON_CURSOR_LERP_SPEED <const> = 0.12

local MAX_BACKGROUND_OFFSET <const> = 16

RoomSelectScene = {}

local cursorTopImage = gfx.image.new("images/roomSelect/mapIcons/mapCursorTop")
local cursorTopSprite = gfx.sprite.new(cursorTopImage)
local cursorBottomImage = gfx.image.new("images/roomSelect/mapIcons/mapCursorBottom")
local cursorBottomSprite = gfx.sprite.new(cursorBottomImage)

local areaRooms = {
    ROOM_TYPE.ENEMY,
    {ROOM_TYPE.CAMPFIRE, ROOM_TYPE.ENEMY},
    {ROOM_TYPE.SHOP, ROOM_TYPE.ENEMY},
    ROOM_TYPE.ENEMY,
    ROOM_TYPE.ENEMY,
    {ROOM_TYPE.ELITE, ROOM_TYPE.ENEMY},
    ROOM_TYPE.ENEMY,
    ROOM_TYPE.ENEMY,
    ROOM_TYPE.CAMPFIRE,
    ROOM_TYPE.BOSS
}

local visitedRooms = {
    nil,
    1,
    2,
    nil,
    nil,
    1,
    nil,
    nil,
    nil,
    nil
}

local currentRoom = 2

function RoomSelectScene:init()
    self.offsetFrame = 0
    self.backgroundOffset = 0
    local roomSelectBackground = gfx.image.new("images/roomSelect/roomSelectBackground")
    local roomSelectBackgroundSprite = gfx.sprite.new(roomSelectBackground)
    roomSelectBackgroundSprite:setCenter(0, 0)
    roomSelectBackgroundSprite:moveTo(0, 0)
    roomSelectBackgroundSprite:add()
    self.roomSelectBackgroundSprite = roomSelectBackgroundSprite

    local gradient = gfx.image.new("images/roomSelect/gradient")
    local gradientSprite = gfx.sprite.new(gradient)
    gradientSprite:setCenter(0, 0)
    gradientSprite:moveTo(0, 0)
    gradientSprite:add()

    local pathImage = gfx.image.new(MAP_BORDER_WIDTH, MAP_BORDER_HEIGHT)
    local pathSprite = gfx.sprite.new(pathImage)
    pathSprite:setCenter(0, 0)
    pathSprite:moveTo(MAP_BASE_X, MAP_BASE_Y)
    pathSprite:add()

    self.selectedPath = 1
    local prospectivePath1Image = pathImage:copy()
    local prospectivePath1 = gfx.sprite.new(prospectivePath1Image)
    prospectivePath1:setCenter(0, 0)
    prospectivePath1:moveTo(MAP_BASE_X, MAP_BASE_Y)
    prospectivePath1:add()
    prospectivePath1:setVisible(false)
    self.prospectivePath1 = prospectivePath1

    local prospectivePath2Image = pathImage:copy()
    local prospectivePath2 = gfx.sprite.new(prospectivePath2Image)
    prospectivePath2:setCenter(0, 0)
    prospectivePath2:moveTo(MAP_BASE_X, MAP_BASE_Y)
    prospectivePath2:add()
    prospectivePath2:setVisible(false)
    self.prospectivePath2 = prospectivePath2

    local mapImage = gfx.image.new(MAP_BORDER_WIDTH, MAP_BORDER_HEIGHT)
    gfx.pushContext(mapImage)
        gfx.setColor(gfx.kColorWhite)
        gfx.setLineWidth(2)
        gfx.drawRoundRect(0, 0, MAP_BORDER_WIDTH, MAP_BORDER_HEIGHT, 4)
    gfx.popContext()
    local mapSprite = gfx.sprite.new(mapImage)
    mapSprite:setCenter(0, 0)
    mapSprite:moveTo(MAP_BASE_X, MAP_BASE_Y)
    mapSprite:add()

    local isFirstRoom = currentRoom == 1

    local delayTime = 450
    local additionalDelay = 500
    local currentDelay = isFirstRoom and delayTime or 0
    local synth = pd.sound.synth.new(pd.sound.kWaveSawtooth)
    synth:setADSR(0, 0.1, 0, 0.4, 0.6)
    local noteOffset = 0

    local bossSynth = pd.sound.synth.new(pd.sound.kWaveNoise)
    bossSynth:setADSR(0, 1.0, 0.0, 0.0, 1.0)
    local notePlayTime = 1.0

    local playNote = function(isBoss)
        if not isFirstRoom then
            return
        end
        if isBoss then
            bossSynth:playMIDINote(30, 100, 2.0)
        else
            synth:playMIDINote(40 + noteOffset, 100, notePlayTime)
            noteOffset += 0.5
        end
    end

    for i, roomType in ipairs(areaRooms) do
        pd.timer.performAfterDelay(currentDelay, function()
            local iconX = MAP_ICON_BASE_X + (i - 1) * MAP_ICON_X_SPACING
            if type(roomType) == "table" then
                playNote()
                gfx.pushContext(mapImage)
                    roomTypeMapIcons[roomType[1]]:drawAnchored(iconX, MAP_ICON_BASE_Y - MAP_ICON_Y_SPACING, 0.5, 0.5, gfx.kImageUnflipped)
                    roomTypeMapIcons[roomType[2]]:drawAnchored(iconX, MAP_ICON_BASE_Y + MAP_ICON_Y_SPACING, 0.5, 0.5, gfx.kImageUnflipped)
                gfx.popContext()
            else
                playNote(roomType == ROOM_TYPE.BOSS)
                gfx.pushContext(mapImage)
                    roomTypeMapIcons[roomType]:drawAnchored(iconX, MAP_ICON_BASE_Y, 0.5, 0.5, gfx.kImageUnflipped)
                gfx.popContext()
                if isFirstRoom and roomType == ROOM_TYPE.BOSS then
                    local cursorX = MAP_BASE_X + MAP_ICON_BASE_X + (currentRoom - 1) * MAP_ICON_X_SPACING
                    cursorTopSprite:moveTo(cursorX, MAP_ICON_CURSOR_TOP_BASE_TARGET_Y)
                    cursorBottomSprite:moveTo(cursorX, MAP_ICON_CURSOR_BOTTOM_BASE_TARGET_Y)
                    cursorTopSprite:add()
                    cursorBottomSprite:add()
                    prospectivePath1:setVisible(true)
                end
            end
        end)
        local delayAmount = delayTime
        if i == #areaRooms - 1 then
            delayAmount += additionalDelay
        end
        currentDelay += isFirstRoom and delayAmount or 0
    end

    self.cursorTargetX = nil
    self.cursorTopTargetY = nil
    self.cursorBottomTargetY = nil
    local cursorMoveDelay = 500
    if not isFirstRoom then
        local previousCursorX = MAP_BASE_X + MAP_ICON_BASE_X + (currentRoom - 2) * MAP_ICON_X_SPACING
        if type(areaRooms[currentRoom - 1]) == "table" then
            cursorTopSprite:moveTo(previousCursorX, MAP_ICON_CURSOR_TOP_BASE_TARGET_Y - MAP_ICON_Y_SPACING)
            cursorBottomSprite:moveTo(previousCursorX, MAP_ICON_CURSOR_BOTTOM_BASE_TARGET_Y + MAP_ICON_Y_SPACING)
        else
            cursorTopSprite:moveTo(previousCursorX, MAP_ICON_CURSOR_TOP_BASE_TARGET_Y)
            cursorBottomSprite:moveTo(previousCursorX, MAP_ICON_CURSOR_BOTTOM_BASE_TARGET_Y)
        end
        prospectivePath1:setVisible(true)
        cursorTopSprite:add()
        cursorBottomSprite:add()
        pd.timer.performAfterDelay(cursorMoveDelay, function()
            self.cursorTargetX = MAP_BASE_X + MAP_ICON_BASE_X + (currentRoom - 1) * MAP_ICON_X_SPACING
            self.cursorTopTargetY = MAP_ICON_CURSOR_TOP_BASE_TARGET_Y
            self.cursorBottomTargetY = MAP_ICON_CURSOR_BOTTOM_BASE_TARGET_Y
            if type(areaRooms[currentRoom]) == "table" then
                self.cursorTopTargetY = MAP_ICON_CURSOR_TOP_BASE_TARGET_Y - MAP_ICON_Y_SPACING
                self.cursorBottomTargetY = MAP_ICON_CURSOR_BOTTOM_BASE_TARGET_Y + MAP_ICON_Y_SPACING
            else
                self.cursorTopTargetY = MAP_ICON_CURSOR_TOP_BASE_TARGET_Y
                self.cursorBottomTargetY = MAP_ICON_CURSOR_BOTTOM_BASE_TARGET_Y
            end
        end)
    end

    -- Draw lines between visited rooms
    local lineXOffset = 0
    local lineYOffset = 0
    gfx.pushContext(pathImage)
        gfx.setColor(gfx.kColorWhite)
        -- gfx.setPattern({0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA})
        gfx.setLineCapStyle(gfx.kLineCapStyleRound)
        gfx.setLineWidth(3)
        for i = 1, currentRoom - 1 do
            local prospectivePath = i == currentRoom - 1
            local startIsDouble = type(areaRooms[i]) == "table"
            local endIsDouble = type(areaRooms[i + 1]) == "table"
            local startX = MAP_ICON_BASE_X + (i - 1) * MAP_ICON_X_SPACING + lineXOffset
            local endX = MAP_ICON_BASE_X + i * MAP_ICON_X_SPACING - lineXOffset
            local startY = MAP_ICON_BASE_Y
            local endY = MAP_ICON_BASE_Y
            local prospective1EndY = endY
            local prospective2EndY = endY

            if startIsDouble then
                if visitedRooms[i] == 1 then
                    startY -= MAP_ICON_Y_SPACING - lineYOffset
                else
                    startY += MAP_ICON_Y_SPACING - lineYOffset
                end
            end

            if endIsDouble then
                prospective1EndY -= MAP_ICON_Y_SPACING - lineYOffset
                prospective2EndY += MAP_ICON_Y_SPACING - lineYOffset
                if visitedRooms[i + 1] == 1 then
                    endY = prospective1EndY
                else
                    endY = prospective2EndY
                end
            end

            if prospectivePath then
                gfx.setPattern({0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA})
                gfx.pushContext(prospectivePath1Image)
                    gfx.drawLine(startX, startY, endX, prospective1EndY)
                gfx.popContext()
                gfx.pushContext(prospectivePath2Image)
                    gfx.drawLine(startX, startY, endX, prospective2EndY)
                gfx.popContext()
            else
                gfx.drawLine(startX, startY, endX, endY)
            end
        end
    gfx.popContext()
end

function RoomSelectScene:update()
    gfx.sprite.update()
    pd.timer.updateTimers()

    self.offsetFrame = (self.offsetFrame + 1) % 2
    if self.offsetFrame == 1 then
        self.backgroundOffset = (self.backgroundOffset + 1) % MAX_BACKGROUND_OFFSET
    end
    self.roomSelectBackgroundSprite:moveTo(-MAX_BACKGROUND_OFFSET + self.backgroundOffset, -self.backgroundOffset)

    if self.cursorTargetX then
        local currentX = cursorTopSprite.x
        local currentTopY = cursorTopSprite.y
        local currentBottomY = cursorBottomSprite.y

        local lerpBuffer = 0.8
        if (math.abs(currentX - self.cursorTargetX) > lerpBuffer) then
            cursorTopSprite:moveTo(math.lerp(currentX, self.cursorTargetX, MAP_ICON_CURSOR_LERP_SPEED), currentTopY)
            cursorBottomSprite:moveTo(math.lerp(currentX, self.cursorTargetX, MAP_ICON_CURSOR_LERP_SPEED), currentBottomY)
        else
            cursorTopSprite:moveTo(self.cursorTargetX, currentTopY)
            cursorBottomSprite:moveTo(self.cursorTargetX, currentBottomY)
        end

        if (math.abs(currentTopY - self.cursorTopTargetY) > lerpBuffer) then
            cursorTopSprite:moveTo(cursorTopSprite.x, math.lerp(currentTopY, self.cursorTopTargetY, MAP_ICON_CURSOR_LERP_SPEED))
            cursorBottomSprite:moveTo(cursorBottomSprite.x, math.lerp(currentBottomY, self.cursorBottomTargetY, MAP_ICON_CURSOR_LERP_SPEED))
        else
            cursorTopSprite:moveTo(cursorTopSprite.x, self.cursorTopTargetY)
            cursorBottomSprite:moveTo(cursorBottomSprite.x, self.cursorBottomTargetY)
        end
    end

    if self.prospectivePath1:isVisible() or self.prospectivePath2:isVisible() then
        if pd.buttonJustPressed(pd.kButtonLeft) or pd.buttonJustPressed(pd.kButtonUp) then
            self.selectedPath = 1
            self.prospectivePath1:setVisible(true)
            self.prospectivePath2:setVisible(false)
        elseif pd.buttonJustPressed(pd.kButtonRight) or pd.buttonJustPressed(pd.kButtonDown) then
            if type(areaRooms[currentRoom]) == "table" then
                self.selectedPath = 2
                self.prospectivePath1:setVisible(false)
                self.prospectivePath2:setVisible(true)
            end
        end
    end

    if pd.buttonJustPressed(pd.kButtonA) then
        visitedRooms[currentRoom] = self.selectedPath
        currentRoom += 1
        SceneManager.switchScene(RoomSelectScene)
    end
end