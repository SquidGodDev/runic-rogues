
GAME_DATA = {}

GameManager = {}
function GameManager:init()
    GAME_DATA = {
        currentArea = 2,
        currentRoom = 1,
        areaRooms = self:getAreaRooms(),
        visitedRooms = {}
    }
end

function GameManager:getAreaRooms()
    return {
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.BOSS
    }
end