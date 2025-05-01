local luabc = require("luabc")
local cmd   = luabc.cmd:new()
local tool  = luabc.tool

-- gcc -Wall -Wextra -std=gnu11 -I src/ -o dysh src/main.c src/parse.c

local CC     = "gcc"
local CFLAGS = { "-Wall", "-Wextra", "-I src/" }
local CSTD   = "-std=gnu11"
local TARGET = "dysh"
local SRC    = { "src/parse.c", "src/main.c" }
local OBJ    = (function () 
    local res = {}
    for _, file in ipairs(SRC) do
        table.insert(res, tool.replace_file_extension(file, "o"))
    end
    return res
end)()
local CLEAR_FILES = { TARGET, table.unpack(OBJ) }

local function build_obj()
    table.insert(CFLAGS, "-c")
    for i = 1, #SRC do
        cmd:append(CC, CFLAGS, CSTD, "-o", OBJ[i], SRC[i])
        cmd:print(true)
        cmd:run()
    end
    table.remove(CFLAGS)
end

local function build_dysh()
    build_obj()
    cmd:append(CC, CFLAGS, CSTD, "-o", TARGET, OBJ)
    cmd:run()
    tool.clean(CLEAR_FILES)
end

build_dysh()
