local state = assert(tonumber(assert(os.getenv("BLACKOUT_RUN_STATE")), 16))
local offsets = assert(tonumber(assert(os.getenv("BLACKOUT_RUN_OFFSETS")), 16))
local output = assert(os.getenv("BLACKOUT_RUN_OUTPUT"))
local result_path = assert(os.getenv("BLACKOUT_RUN_RESULT"))

local function off(index) return emu:read32(offsets + index * 4) end
local SCREEN = off(0)
local SHIFT = off(1)
local DELIVERED = off(4)
local MISSED = off(5)
local COPIED = off(6)
local MONEY = off(9)
local HEAT = off(11)
local LAST_ROUTE = off(12)
local ROUTE_BYTES = off(13)

local TITLE, BRIEF, PLAY, PAUSE, MEMORY, ENDING = 0, 1, 2, 3, 4, 5

local function frames(count)
    for _ = 1, count do emu:runFrame() end
end

local function tap(key)
    emu:addKey(key)
    frames(4)
    emu:clearKey(key)
    frames(3)
end

local function expect(value, label)
    local actual = emu:read8(state + SCREEN)
    if actual ~= value then
        error(string.format("%s: expected screen %d, got %d", label, value, actual))
    end
end

local function shot(name)
    emu:screenshot(output .. "/" .. name .. ".png")
end

local function seed_learned_city()
    for index = 0, 599 do emu:write8(state + HEAT + index, 0) end
    for index = 0, ROUTE_BYTES - 1 do emu:write8(state + LAST_ROUTE + index, 0) end
    local cells = {}
    for x = 4, 24 do table.insert(cells, 6 * 30 + x) end
    for y = 6, 17 do table.insert(cells, y * 30 + 23) end
    for _, index in ipairs(cells) do
        emu:write8(state + HEAT + index, (index % 5) + 2)
        local address = state + LAST_ROUTE + math.floor(index / 8)
        local bit = 2 ^ (index % 8)
        emu:write8(address, emu:read8(address) + bit)
    end
    emu:write8(state + DELIVERED, 2)
    emu:write8(state + MISSED, 0)
    emu:write8(state + COPIED, 40)
    emu:write16(state + MONEY, 57)
end

local ok, failure = xpcall(function()
    emu:setKeys(0)
    emu:reset()
    frames(120)
    expect(TITLE, "reset")
    if emu:getGameTitle() ~= "BLACKOUT RUN" then error("wrong ROM title") end
    if emu:getGameCode() ~= "AGB-BLKR" then error("wrong ROM game code") end
    shot("01-title")

    tap(0)
    expect(BRIEF, "story selection")
    shot("02-shift-brief")
    tap(0)
    expect(PLAY, "start shift")
    frames(30)
    shot("03-first-run")
    tap(3)
    expect(PAUSE, "pause")
    shot("04-pause")
    tap(3)
    expect(PLAY, "resume")

    seed_learned_city()
    tap(2)
    expect(MEMORY, "route memory")
    shot("05-route-memory")

    emu:write8(state + SHIFT, 2)
    emu:write8(state + SCREEN, PLAY)
    frames(12)
    shot("06-blackout-run")

    emu:write8(state + DELIVERED, 6)
    emu:write16(state + MONEY, 148)
    emu:write8(state + SCREEN, ENDING)
    frames(12)
    shot("07-ending")
end, debug.traceback)

emu:setKeys(0)
local temporary = result_path .. ".tmp"
os.remove(temporary)
local result = assert(io.open(temporary, "w"))
if ok then result:write("PASS\n") else result:write("FAIL\n", failure, "\n") end
assert(result:close())
assert(os.rename(temporary, result_path))
