-- 'luabc' are some apis implemented by lua to 
-- build c at a easily way, like writing shell script

local color = {
    RESET  = "\x1b[00m",
    BLACK  = "\x1b[30m",
    RED    = "\x1b[31m",
    GREEN  = "\x1b[32m",
    YELLOW = "\x1b[33m",
    BLUE   = "\x1b[34m",
    PURPLE = "\x1b[35m",
    CYAN   = "\x1b[36m",
    WHITE  = "\x1b[37m",
}

local log = {
    err = function (msg)
        print("[" .. color.RED .. "ERROR" .. color.RESET .. "] " .. msg)
    end,

    ok = function (msg)
        print("[" .. color.GREEN .. "OK" .. color.RESET .. "] " .. msg)
    end,

    info = function (msg)
        print("[" .. color.CYAN .. "INFO" .. color.RESET .. "] " .. msg)
    end,
}

local luabc = { tool = {}, cmd = {} }

function luabc.cmd:new()
    local object = { args = {} }
    setmetatable(object, { __index = luabc.cmd })
    return object
end

function luabc.cmd:append(...)
    local args = {...}
    if #args == 0 then
        log.err("there has no args")
        return
    end
    for _, arg in ipairs(args) do
        if type(arg) == "table" then
            arg = table.concat(arg, " ")
        end
        table.insert(self.args, arg)
    end
end

function luabc.cmd:print(is_cmd_line_form)
    if not is_cmd_line_form then
        for key, val in ipairs(self.args) do print(key, val) end
    else
        print(table.concat(self.args, " "))
    end
end

function luabc.cmd:clear()
    self.args = {}
end

function luabc.cmd:run()
    local cmd = table.concat(self.args, " ")
    local status, msg = os.execute(cmd)
    if status then
        log.ok("run the command successfully")
    else
        log.err("something wrong happends: " .. msg)
    end
    self:clear()
end

function luabc.tool.match_file_extension(extension, dir, maxdepth)
    dir = dir or "."
    extension = extension and ("*." .. extension) or "*"
    maxdepth = tonumber(maxdepth) or 1
    local res = {}

    local cmd = "find " .. dir .. " " ..
                "-maxdepth " .. maxdepth .. " " ..
                "-type f " ..
                "-name " .. "'" .. extension .. "'"
    local handle = io.popen(cmd)
    for file in handle:lines() do
        table.insert(res, file)
    end
    handle:close()

    if #res == 0 then
        log.info("no matched files with extension [" .. extension .. "]")
    end

    return res, #res
end

function luabc.tool.replace_file_extension(filename, new_extension)
    if not new_extension or not filename then
        log.err("parameter <filename> and <new_extension> must be valid")
        return
    end
    new_extension = "." .. new_extension
    local new_filename, _ = string.gsub(filename, "%.[^%.]+$", new_extension)
    return new_filename
end

function luabc.tool.clean(file)
    if type(file) == "table" then
        file = table.concat(file, " ")
    end
    local cmd = "rm -r " .. file
    local status, msg = os.execute("rm -r " .. file)
    if status then
        log.ok("run the command successfully")
    else
        log.err("something wrong happends: " .. msg)
    end
end

return luabc
