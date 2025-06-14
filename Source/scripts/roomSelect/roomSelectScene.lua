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

local echoingCavernsTitleEN <const> = gfx.image.new("images/roomSelect/titles/echoingCavernsTitle_EN")
local theLostArchivesTitleEN <const> = gfx.image.new("images/roomSelect/titles/theLostArchivesTitle_EN")

ROOM_TYPE = {
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

local TITLE_X <const> = 100
local TITLE_Y <const> = 8

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

local TILE_OFFSCREEN_Y <const> = 150
local TILE_IDLE_TARGET_Y <const> = 94
local TILE_SELECTED_TARGET_Y <const> = TILE_IDLE_TARGET_Y - 5
local TILE_GAP <const> = 40
local TILE_LERP_SPEED <const> = 0.1
local TILE_CURSOR_LERP_SPEED <const> = 0.2

local MAX_BACKGROUND_OFFSET <const> = 16

RoomSelectScene = {}

local cursorNavigateSynth = pd.sound.synth.new(pd.sound.kWaveTriangle)
cursorNavigateSynth:setADSR(0.0, 0.1, 0.0, 0.0, 1.0)

local cursorTopImage = gfx.image.new("images/roomSelect/mapIcons/mapCursorTop")
local cursorTopSprite = gfx.sprite.new(cursorTopImage)
local cursorBottomImage = gfx.image.new("images/roomSelect/mapIcons/mapCursorBottom")
local cursorBottomSprite = gfx.sprite.new(cursorBottomImage)

function RoomSelectScene:init()
    gfx.setBackgroundColor(gfx.kColorBlack)

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

    local titleImage
    if GAME_DATA.currentArea == 1 then
        titleImage = echoingCavernsTitleEN
    elseif GAME_DATA.currentArea == 2 then
        titleImage = theLostArchivesTitleEN
    end
    local titleSprite = gfx.sprite.new(titleImage)
    titleSprite:moveTo(TITLE_X, TITLE_Y)
    titleSprite:add()

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

    local isFirstRoom = GAME_DATA.currentRoom == 1

    local delayTime = 450
    local additionalDelay = 500
    local currentDelay = isFirstRoom and delayTime or 0
    local noteOffset = 0

    local playNote = function(isBoss)
        if not isFirstRoom then
            return
        end
        if isBoss then
            AudioManager.playSFX(SFX.BossRoomPopulate)
        else
            AudioManager.playSFX(SFX.RoomPopulate, 40 + noteOffset)
            noteOffset += 0.5
        end
    end

    for i, roomType in ipairs(GAME_DATA.areaRooms) do
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
                    local cursorX = MAP_BASE_X + MAP_ICON_BASE_X + (GAME_DATA.currentRoom - 1) * MAP_ICON_X_SPACING
                    cursorTopSprite:moveTo(cursorX, MAP_ICON_CURSOR_TOP_BASE_TARGET_Y)
                    cursorBottomSprite:moveTo(cursorX, MAP_ICON_CURSOR_BOTTOM_BASE_TARGET_Y)
                    cursorTopSprite:add()
                    cursorBottomSprite:add()
                    self.cursorTargetX = cursorX
                    self.cursorTopTargetY = cursorTopSprite.y
                    self.cursorBottomTargetY = cursorBottomSprite.y
                    prospectivePath1:setVisible(true)

                    ScreenShake(500, 5)
                end
            end
        end)
        local delayAmount = delayTime
        if i == #GAME_DATA.areaRooms - 1 then
            delayAmount += additionalDelay
        end
        currentDelay += isFirstRoom and delayAmount or 0
    end

    self.cursorTargetX = nil
    self.cursorTopTargetY = nil
    self.cursorBottomTargetY = nil
    local cursorMoveDelay = 500
    if not isFirstRoom then
        local previousCursorX = MAP_BASE_X + MAP_ICON_BASE_X + (GAME_DATA.currentRoom - 2) * MAP_ICON_X_SPACING
        if type(GAME_DATA.areaRooms[GAME_DATA.currentRoom - 1]) == "table" then
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
            AudioManager.playSFX(SFX.CursorSlide)
            self.cursorTargetX = MAP_BASE_X + MAP_ICON_BASE_X + (GAME_DATA.currentRoom - 1) * MAP_ICON_X_SPACING
            self.cursorTopTargetY = MAP_ICON_CURSOR_TOP_BASE_TARGET_Y
            self.cursorBottomTargetY = MAP_ICON_CURSOR_BOTTOM_BASE_TARGET_Y
            if type(GAME_DATA.areaRooms[GAME_DATA.currentRoom]) == "table" then
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
        gfx.setLineCapStyle(gfx.kLineCapStyleRound)
        gfx.setLineWidth(3)
        for i = 1, GAME_DATA.currentRoom - 1 do
            local prospectivePath = i == GAME_DATA.currentRoom - 1
            local startIsDouble = type(GAME_DATA.areaRooms[i]) == "table"
            local endIsDouble = type(GAME_DATA.areaRooms[i + 1]) == "table"
            local startX = MAP_ICON_BASE_X + (i - 1) * MAP_ICON_X_SPACING + lineXOffset
            local endX = MAP_ICON_BASE_X + i * MAP_ICON_X_SPACING - lineXOffset
            local startY = MAP_ICON_BASE_Y
            local endY = MAP_ICON_BASE_Y
            local prospective1EndY = endY
            local prospective2EndY = endY

            if startIsDouble then
                if GAME_DATA.visitedRooms[i] == 1 then
                    startY -= MAP_ICON_Y_SPACING - lineYOffset
                else
                    startY += MAP_ICON_Y_SPACING - lineYOffset
                end
            end

            if endIsDouble then
                prospective1EndY -= MAP_ICON_Y_SPACING - lineYOffset
                prospective2EndY += MAP_ICON_Y_SPACING - lineYOffset
                if GAME_DATA.visitedRooms[i + 1] == 1 then
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

    if type(GAME_DATA.areaRooms[GAME_DATA.currentRoom]) == "table" then
        self.tile1 = gfx.sprite.new(roomTypeTiles[GAME_DATA.areaRooms[GAME_DATA.currentRoom][1]])
        self.tile1:moveTo(100 - TILE_GAP, TILE_OFFSCREEN_Y)
        self.tile1:add()
        self.tile2 = gfx.sprite.new(roomTypeTiles[GAME_DATA.areaRooms[GAME_DATA.currentRoom][2]])
        self.tile2:moveTo(100 + TILE_GAP, TILE_OFFSCREEN_Y)
        self.tile2:add()
    else
        self.tile1 = gfx.sprite.new(roomTypeTiles[GAME_DATA.areaRooms[GAME_DATA.currentRoom]])
        self.tile1:moveTo(100, TILE_OFFSCREEN_Y)
        self.tile1:add()
        self.tile2 = nil
    end

    local tileCursor = gfx.sprite.new(gfx.image.new("images/roomSelect/tiles/tileCursor"))
    tileCursor:moveTo(self.tile1:getPosition())
    tileCursor:add()
    self.tileCursor = tileCursor
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
        if pd.buttonJustPressed(pd.kButtonA) and not SceneManager.isTransitioning() then
            GAME_DATA.visitedRooms[GAME_DATA.currentRoom] = self.selectedPath
            SceneManager.switchScene(GameScene, GAME_DATA.playerMaxHealth, GAME_DATA.playerHealth, GAME_DATA.currentArea, GAME_DATA.currentRoom)
        end

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

        if self.tile1 then
            local targetY = self.selectedPath == 1 and TILE_SELECTED_TARGET_Y or TILE_IDLE_TARGET_Y
            self.tile1:moveTo(self.tile1.x, math.lerp(self.tile1.y, targetY, TILE_LERP_SPEED))

            if self.selectedPath == 1 then
                local cursorX = math.lerp(self.tileCursor.x, self.tile1.x, TILE_CURSOR_LERP_SPEED)
                local cursorY = math.lerp(self.tileCursor.y, self.tile1.y, TILE_CURSOR_LERP_SPEED)
                self.tileCursor:moveTo(cursorX, cursorY)
            end
        end

        if self.tile2 then
            local targetY = self.selectedPath == 2 and TILE_SELECTED_TARGET_Y or TILE_IDLE_TARGET_Y
            self.tile2:moveTo(self.tile2.x, math.lerp(self.tile2.y, targetY, TILE_LERP_SPEED))

            if self.selectedPath == 2 then
                local cursorX = math.lerp(self.tileCursor.x, self.tile2.x, TILE_CURSOR_LERP_SPEED)
                local cursorY = math.lerp(self.tileCursor.y, self.tile2.y, TILE_CURSOR_LERP_SPEED)
                self.tileCursor:moveTo(cursorX, cursorY)
            end
        end
    end

    if not SceneManager.isTransitioning() and (self.prospectivePath1:isVisible() or self.prospectivePath2:isVisible()) then
        if self.selectedPath ~= 1 and pd.buttonJustPressed(pd.kButtonLeft) or pd.buttonJustPressed(pd.kButtonUp) then
            AudioManager.playSFX(SFX.CursorNavigate)
            self.selectedPath = 1
            self.prospectivePath1:setVisible(true)
            self.prospectivePath2:setVisible(false)
        elseif self.selectedPath ~= 2 and pd.buttonJustPressed(pd.kButtonRight) or pd.buttonJustPressed(pd.kButtonDown) then
            if type(GAME_DATA.areaRooms[GAME_DATA.currentRoom]) == "table" then
                AudioManager.playSFX(SFX.CursorNavigate)
                self.selectedPath = 2
                self.prospectivePath1:setVisible(false)
                self.prospectivePath2:setVisible(true)
            end
        end
    end
end