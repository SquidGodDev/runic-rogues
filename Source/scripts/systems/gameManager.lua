
GAME_DATA = {}

GameManager = {}
function GameManager:init()
    GAME_DATA = {
        currentRoom = 2,
        currentArea = 1,
        areaRooms = self:getAreaRooms(),
        visitedRooms = {}
    }
end

function GameManager:getAreaRooms()
    return {
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.ENEMY,
        {ROOM_TYPE.CAMPFIRE, ROOM_TYPE.ENEMY},
        {ROOM_TYPE.ENEMY, ROOM_TYPE.ELITE},
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.ENEMY,
        ROOM_TYPE.ENEMY,
        {ROOM_TYPE.CAMPFIRE, ROOM_TYPE.ENEMY},
        ROOM_TYPE.BOSS
    }
end