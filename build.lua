local luabc = require("luabc")
local cmd   = luabc.cmd
local tool  = luabc.tool
local debug = luabc.debug

local CC            = "gcc"
local CFLAGS        = { "-Wall", "-Wextra", "-I src/" }
local CSTD          = "-std=gnu11"

local TARGET        = "dysh"
local DYSH_SRC      = tool.match_file_extension(".c", "src")
local DYSH_OBJ      = tool.replace_files_extension(DYSH_SRC, ".o")
local DYSH_CLEAN    = { TARGET, table.unpack(DYSH_OBJ) }

local CMD_SRC       = tool.match_file_extension(".c", "src/builtin")
local CMD_CLEAN     = "src/bin/"
local CMD_TARGET    = tool.replace_paths_directory(tool.replace_files_extension(CMD_SRC, ""), "src/bin/")

local function build_builtin_cmd()
    local mkdir = cmd:new()
    mkdir:append("mkdir -p", "src/bin/")

    for i = 1, #CMD_SRC do
        local all = cmd:new()
        all:append(CC, CFLAGS, CSTD, "-o", CMD_TARGET[i], CMD_SRC[i])
    end

    local clean = cmd:new("clean")
    clean:append("rm -rf", CMD_CLEAN)
end

local function build_dysh()
    for i = 1, #DYSH_SRC do
        local compile = cmd:new()
        compile:append(CC, CFLAGS, CSTD, "-c", "-o", DYSH_OBJ[i], DYSH_SRC[i])
    end

    local link = cmd:new()
    link:append(CC, CFLAGS, CSTD, "-o", TARGET, DYSH_OBJ)

    local clean = cmd:new("clean")
    clean:append("rm -rf", DYSH_CLEAN)

    build_builtin_cmd()

    luabc.build()
end

build_dysh()
