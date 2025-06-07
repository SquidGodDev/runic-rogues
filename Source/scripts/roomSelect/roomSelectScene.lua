local pd <const> = playdate
local gfx <const> = playdate.graphics

local enemyIcon <const> = gfx.image.new("images/roomSelect/mapIcons/enemyIcon")
local eliteIcon <const> = gfx.image.new("images/roomSelect/mapIcons/eliteIcon")
local shopIcon <const> = gfx.image.new("images/roomSelect/mapIcons/shopIcon")
local campfireIcon <const> = gfx.image.new("images/roomSelect/mapIcons/campfireIcon")
local bossIcon <const> = gfx.image.new("images/roomSelect/mapIcons/bossIcon")

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
local offsetFrame
local backgroundOffset
local roomSelectBackgroundSprite

local cursorTopImage = gfx.image.new("images/roomSelect/mapIcons/mapCursorTop")
local cursorTopSprite = gfx.sprite.new(cursorTopImage)
local cursorBottomImage = gfx.image.new("images/roomSelect/mapIcons/mapCursorBottom")
local cursorBottomSprite = gfx.sprite.new(cursorBottomImage)
local cursorTopTargetY
local cursorBottomTargetY
local cursorTargetX

local currentRoom = 8

RoomSelectScene = {}

function RoomSelectScene.init()
    offsetFrame = 0
    backgroundOffset = 0
    local roomSelectBackground = gfx.image.new("images/roomSelect/roomSelectBackground")
    roomSelectBackgroundSprite = gfx.sprite.new(roomSelectBackground)
    roomSelectBackgroundSprite:setCenter(0, 0)
    roomSelectBackgroundSprite:moveTo(0, 0)
    roomSelectBackgroundSprite:add()

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
                end
            end
        end)
        local delayAmount = delayTime
        if i == #areaRooms - 1 then
            delayAmount += additionalDelay
        end
        currentDelay += isFirstRoom and delayAmount or 0
    end

    cursorTargetX = nil
    cursorTopTargetY = nil
    cursorBottomTargetY = nil
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
        cursorTopSprite:add()
        cursorBottomSprite:add()
        pd.timer.performAfterDelay(cursorMoveDelay, function()
            cursorTargetX = MAP_BASE_X + MAP_ICON_BASE_X + (currentRoom - 1) * MAP_ICON_X_SPACING
            cursorTopTargetY = MAP_ICON_CURSOR_TOP_BASE_TARGET_Y
            cursorBottomTargetY = MAP_ICON_CURSOR_BOTTOM_BASE_TARGET_Y
            if type(areaRooms[currentRoom]) == "table" then
                cursorTopTargetY = MAP_ICON_CURSOR_TOP_BASE_TARGET_Y - MAP_ICON_Y_SPACING
                cursorBottomTargetY = MAP_ICON_CURSOR_BOTTOM_BASE_TARGET_Y + MAP_ICON_Y_SPACING
            else
                cursorTopTargetY = MAP_ICON_CURSOR_TOP_BASE_TARGET_Y
                cursorBottomTargetY = MAP_ICON_CURSOR_BOTTOM_BASE_TARGET_Y
            end
        end)
    end

    -- Draw lines between visited rooms
    local lineXOffset = 7
    local lineYOffset = 5
    gfx.pushContext(pathImage)
        gfx.setColor(gfx.kColorWhite)
        -- gfx.setPattern({0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA})
        gfx.setLineCapStyle(gfx.kLineCapStyleRound)
        gfx.setLineWidth(2)
        for i = 1, currentRoom - 2 do
            local startIsDouble = type(areaRooms[i]) == "table"
            local endIsDouble = type(areaRooms[i + 1]) == "table"
            local startX = MAP_ICON_BASE_X + (i - 1) * MAP_ICON_X_SPACING + lineXOffset
            local endX = MAP_ICON_BASE_X + i * MAP_ICON_X_SPACING - lineXOffset
            local startY = MAP_ICON_BASE_Y
            if startIsDouble then
                if visitedRooms[i] == 1 then
                    startY -= MAP_ICON_Y_SPACING - lineYOffset
                else
                    startY += MAP_ICON_Y_SPACING - lineYOffset
                end
            end
            local endY = MAP_ICON_BASE_Y
            if endIsDouble then
                if visitedRooms[i + 1] == 1 then
                    endY -= MAP_ICON_Y_SPACING - lineYOffset
                else
                    endY += MAP_ICON_Y_SPACING - lineYOffset
                end
            end
            gfx.drawLine(startX, startY, endX, endY)
        end
    gfx.popContext()
end

function RoomSelectScene.update()
    gfx.sprite.update()
    pd.timer.updateTimers()

    offsetFrame = (offsetFrame + 1) % 2
    if offsetFrame == 1 then
        backgroundOffset = (backgroundOffset + 1) % MAX_BACKGROUND_OFFSET
    end
    roomSelectBackgroundSprite:moveTo(-MAX_BACKGROUND_OFFSET + backgroundOffset, -backgroundOffset)

    if cursorTargetX then
        local currentX = cursorTopSprite.x
        local currentTopY = cursorTopSprite.y
        local currentBottomY = cursorBottomSprite.y

        local lerpBuffer = 0.8
        if (math.abs(currentX - cursorTargetX) > lerpBuffer) then
            cursorTopSprite:moveTo(math.lerp(currentX, cursorTargetX, MAP_ICON_CURSOR_LERP_SPEED), currentTopY)
            cursorBottomSprite:moveTo(math.lerp(currentX, cursorTargetX, MAP_ICON_CURSOR_LERP_SPEED), currentBottomY)
        else
            cursorTopSprite:moveTo(cursorTargetX, currentTopY)
            cursorBottomSprite:moveTo(cursorTargetX, currentBottomY)
        end

        if (math.abs(currentTopY - cursorTopTargetY) > lerpBuffer) then
            cursorTopSprite:moveTo(cursorTopSprite.x, math.lerp(currentTopY, cursorTopTargetY, MAP_ICON_CURSOR_LERP_SPEED))
            cursorBottomSprite:moveTo(cursorBottomSprite.x, math.lerp(currentBottomY, cursorBottomTargetY, MAP_ICON_CURSOR_LERP_SPEED))
        else
            cursorTopSprite:moveTo(cursorTopSprite.x, cursorTopTargetY)
            cursorBottomSprite:moveTo(cursorBottomSprite.x, cursorBottomTargetY)
        end
    end

    if pd.buttonJustPressed(pd.kButtonA) then
        currentRoom += 1
        SceneManager.switchScene(RoomSelectScene)
    end
end