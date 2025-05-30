local pd <const> = playdate
local gfx <const> = playdate.graphics


local SCREEN_WIDTH <const> = 200
local SCREEN_HEIGHT <const> = 120
local TRANSITION_CENTER_X <const> = 100
local TRANSITION_CENTER_Y <const> = 60
local TRANSITION_TIME <const> = 500 -- milliseconds
local TRANSITION_CIRCLE_BORDER <const> = 3 -- pixels
local MIN_RADIUS <const> = 0
local MID_RADIUS <const> = 40
local MAX_RADIUS <const> = 250

local timerUpdate = pd.timer.updateTimers

local transitionImage = nil

SceneManager = {}
SceneManager.transitioning = false

local function setSceneUpdate(scene)
    local sceneUpdate = scene.update
    pd.update = function()
        sceneUpdate()
        timerUpdate()
        if transitionImage then
            transitionImage:drawIgnoringOffset(0, 0)
        end
    end
end

local function cleanupScene()
    gfx.sprite.removeAll()
    gfx.setDrawOffset(0, 0)
    pd.display.setOffset(0, 0)
    local allTimers = pd.timer.allTimers()
    for _, timer in ipairs(allTimers) do
        timer:remove()
    end
end

function SceneManager.loadNewScene(scene, args)
    cleanupScene()
    scene.init(table.unpack(args))
    setSceneUpdate(scene)
end

function SceneManager.isTransitioning()
    return SceneManager.transitioning
end

function SceneManager.switchScene(scene, ...)
    if SceneManager.transitioning then
        return false
    end

    SceneManager.transitioning = true
    local args = {...}

    SceneManager.startTransition(scene, args)
    return true
end

local function createTransitionImage(value)
    transitionImage = gfx.image.new(SCREEN_WIDTH, SCREEN_HEIGHT, gfx.kColorBlack)
    local borderWidth = value > TRANSITION_CIRCLE_BORDER and TRANSITION_CIRCLE_BORDER or value
    gfx.pushContext(transitionImage)
        gfx.setColor(gfx.kColorWhite)
        gfx.fillCircleAtPoint(TRANSITION_CENTER_X, TRANSITION_CENTER_Y, value + borderWidth)
    gfx.popContext()

    local transitionMask = gfx.image.new(SCREEN_WIDTH, SCREEN_HEIGHT, gfx.kColorWhite)
    gfx.pushContext(transitionMask)
        gfx.setColor(gfx.kColorBlack)
        gfx.fillCircleAtPoint(TRANSITION_CENTER_X, TRANSITION_CENTER_Y, value)
    gfx.popContext()
    transitionImage:setMaskImage(transitionMask)
end

local function createTransitionTimer(startPos, endPos, easingFunction)
    local transitionTimer = pd.timer.new(TRANSITION_TIME, startPos, endPos, easingFunction)
    createTransitionImage(startPos)
    transitionTimer.updateCallback = function()
        createTransitionImage(transitionTimer.value)
    end
    return transitionTimer
end

function SceneManager.startTransition(scene, args)
    local transitionTimer = createTransitionTimer(MAX_RADIUS, MID_RADIUS, pd.easingFunctions.outCubic)
    transitionTimer.timerEndedCallback = function()
        transitionTimer = createTransitionTimer(MID_RADIUS, MIN_RADIUS, pd.easingFunctions.outCubic)
        transitionTimer.timerEndedCallback = function()
            SceneManager.loadNewScene(scene, args)
            SceneManager.transitioning = false

            transitionTimer = createTransitionTimer(MIN_RADIUS, MAX_RADIUS, pd.easingFunctions.inCubic)
            transitionTimer.timerEndedCallback = function()
                transitionImage = nil
            end
        end
    end
end