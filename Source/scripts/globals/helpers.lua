local pd <const> = playdate
local gfx <const> = pd.graphics

function math.lerp(a, b, t)
    if (math.abs(a - b) < 0.5) then
        return b
    end
	return a + (b - a) * t
end

function ScreenShake(shakeTime, shakeMagnitude)
    -- Creating a value timer that goes from shakeMagnitude to 0, over
    -- the course of 'shakeTime' milliseconds
    local shakeTimer = playdate.timer.new(shakeTime, shakeMagnitude, 0)
    -- Every frame when the timer is active, we shake the screen
    shakeTimer.updateCallback = function(timer)
        -- Using the timer value, so the shaking magnitude
        -- gradually decreases over time
        local magnitude = math.floor(timer.value)
        local shakeX = math.random(-magnitude, magnitude)
        local shakeY = math.random(-magnitude, magnitude)
        playdate.display.setOffset(shakeX, shakeY)
    end
    -- Resetting the display offset at the end of the screen shake
    shakeTimer.timerEndedCallback = function()
        playdate.display.setOffset(0, 0)
    end
end

Utilities = {}

function Utilities.imageWithText(string, font)
	local textImage = gfx.image.new(font:getTextWidth(string), font:getHeight())
	gfx.pushContext(textImage)
        font:drawText(string, 0, 0)
    gfx.popContext()
    return textImage
end

function Utilities.spriteWithText(string, font)
   local textImage = Utilities.imageWithText(string, font)
   return gfx.sprite.new(textImage)
end
