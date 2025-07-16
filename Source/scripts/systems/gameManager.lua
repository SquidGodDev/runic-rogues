
GAME_DATA = {}

GameManager = {}
function GameManager:init()
    GAME_DATA = {
        currentRoom = 2,
        currentArea = 2,
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
        ROOM_TYPE.CAMPFIRE,
        ROOM_TYPE.BOSS
    }
end