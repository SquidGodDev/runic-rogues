
GAME_DATA = {}

GameManager = {}
function GameManager:init()
    GAME_DATA = {
        currentArea = 1,
        currentRoom = 2,
        areaRooms = self:getAreaRooms(),
        visitedRooms = {}
    }
end

function GameManager:getAreaRooms()
    return {
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.SHOP,
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.SHOP,
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.SHOP,
        ROOM_TYPE.BOSS
    }
end