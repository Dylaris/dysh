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

local luabc = { 
    bind  = { ["__default"] = {} },     -- stores labels and their associated commands
    tool  = {},                         -- contains useful utility functions
    cmd   = {},                         -- defines functions related to command operations
    debug = { log = {} }                -- contains functions and lgos for debugging
}

-- @brief Build the entire project according the label (based on the cmd line input
-- and recoreds in 'luabc.bind')
-- @param is_clear Clear the old records after build or not (for debugging)
function luabc.build(is_clear)
    if is_clear == nil then is_clear = false else is_clear = is_clear and true end

    if #arg <= 1 then
        local label = arg[1] or "-__default"
        if string.sub(label, 1, 1) ~= "-" then
            luabc.debug.log.err("you should use label with '-': -" .. arg[1])
            return
        end
        label = string.sub(label, 2, -1)
        if not luabc.bind[label] then
            luabc.debug.log.err("label "  .. label .. " is not binded")
        else
            for _, cmd in ipairs(luabc.bind[label]) do cmd:run(is_clear) end
        end
    else 
        luabc.debug.log.err("usage: lua build.lua [-label]")
    end

    if is_clear then luabc.bind = { ["__default"] = {} } end
end

--===================================================================================
--================================================================= luabc.cmd
--===================================================================================

-- @brief Create a command object
-- @param label Which label the cmd belongs to
-- @param order Defines the execution order (ascending) among all the commands in the same label
-- @param code_first Execute the code first or not
-- @note The command must be associated with only one label (default label: '__default')
function luabc.cmd:new(label, order, code_first)
    label = label or "__default"
    if not luabc.bind[label] then
        order = order or 1
    else
        order = order or (#luabc.bind[label] + 1)
    end
    code_first = code_first or false
    local object = { 
        _args       = {},           -- contains the parameters for the command line
        _code       = {},           -- contains a function and its parameters
        _link       = {},           -- contains a group of labels to be executed after this command (executed in order)
        _label      = label,        -- the label associated with this command
        _order      = order,        -- the execution order of this command within its label (executed in descending order)
        _code_first = code_first    -- indicates whether the function should be executed first
    }
    setmetatable(object, { __index = luabc.cmd })
    object:bind(label)
    return object
end

-- @brief Append the arg to 'cmd.args'
-- @param ... Variable args
-- @note There are two types of append: static and dynamic.
-- In static append, the argument passed is constant and cannot be modified
-- In dynamic append, the argument is a table, allowing modifications before execution
-- as luabc will replace it with a string during runtime
function luabc.cmd:append(...)
    local args = {...}
    if #args == 0 then
        luabc.debug.log.err("there has no args")
        return
    end
    for _, arg in ipairs(args) do
        table.insert(self._args, arg)
    end
end

-- @brief Appends the function to 'cmd.code' (this is why 'code_first' is required in cmd:new())
-- @param func The function to be assigned to 'cmd'
-- @param ... The parameters for the function
-- @note Only one function can be assigned to 'cmd.code' at a time
function luabc.cmd:call(func, ...)
    local code = {func, ...}
    if #code == 0 or type(func) ~= "function" then
        luabc.debug.log.err("you should pass the function and its parameter")
        return
    end
    self._code = code
end

-- @brief Remove the label from cmd.link
-- @param label The removed one
function luabc.cmd:unlink(label)
    for idx, link_label in ipairs(self._link) do
        if link_label == label then
            table.remove(self._link, idx)
            break
        end
    end
end

-- @brief Add the label to cmd.link
-- @param label The add one
function luabc.cmd:link(label)
    table.insert(self._link, label)
end

-- @brief Clear the cmd object
function luabc.cmd:clear()
    self._args       = {}
    self._code       = {}
    self._link       = {}
    self._label      = "__default"
    self._order      = 1
    self._code_first = false
end

local function exec_cmd(args)
    if #(args) == 0 then return end
    -- replace the variable to real value
    local real_args = {}
    for idx, arg in ipairs(args) do
        arg = (type(arg) == "table") and table.concat(arg, " ") or arg
        table.insert(real_args, arg)
    end
    local cmd = table.concat(real_args, " ")
    local status, msg = os.execute(cmd)
    if status then
        luabc.debug.log.ok("run successfully [ " .. cmd .. " ]")
    else
        luabc.debug.log.err("run failed [ " .. cmd .. " ] : " .. msg)
    end
end

local function exec_code(code)
    if #(code) < 1 then return end
    local status, result = pcall(code[1], select(2, table.unpack(code)))
    if status then
        luabc.debug.log.ok("call successfully")
    else
        luabc.debug.log.err("call failed: " .. result) 
    end
end

local function exec_link(link)
    for _, label in ipairs(link) do
        for _, cmd in ipairs(luabc.bind[label]) do cmd:run() end
    end
end

-- @brief Run the command
-- @param is_clear Clear the old records after build or not (for debugging)
function luabc.cmd:run(is_clear)
    if self._code_first then
        exec_code(self._code)
        exec_cmd(self._args)
        exec_link(self._link)
    else
        exec_cmd(self._args)
        exec_code(self._code)
        exec_link(self._link)
    end

    if is_clear == nil then is_clear = false else is_clear = is_clear and true end
    if is_clear then self:clear() end
end

-- @brief Bind the cmd to a label
-- @param label The binded one
function luabc.cmd:bind(label)
    if not label then
        luabc.debug.log.info("you have to bind the command to an label")
        return
    end

    local free_flag, add_flag = false, false

    -- add the label if it does not exist before
    -- and we do not need to free 
    if not luabc.bind[label] then
        luabc.bind[label] = {}
        free_flag = true
    end

    for key, val in pairs(luabc.bind) do
        -- free the binding of command and old label
        if not free_flag and key == self._label then
            for idx, cmd in ipairs(val) do 
                if cmd == self then
                    table.remove(val, idx) 
                    break
                end
            end
            free_flag = true
        end
        -- add the binding of command and new label
        if not add_flag and key == label then
            table.insert(val, self)
            table.sort(val, function (a, b) return a._order < b._order end)
            add_flag = true
        end

        if free_flag and add_flag then break end
    end
    self._label = label
end

--===================================================================================
--================================================================= luabc.tool
--===================================================================================

-- @brief Searches for files with the specified extension in the given directory
-- @param extension The file extension to match (e.g., '.txt')
-- @param dir The directory to search in
-- @param recursive A boolean value indicating whether the search should be recursive
-- @return A list of files that match the extension and the total number of matched files
function luabc.tool.match_file_extension(extension, dir, recursive)
    dir = dir or "."
    extension = extension and ("*" .. extension) or "*"
    recursive = recursive or false
    local res = {}

    local cmd = "find " .. dir .. " " ..
                (recursive and "" or "-maxdepth 1 ") ..
                "-type f " ..
                "-name " .. "'" .. extension .. "'"
    local handle = io.popen(cmd)
    for file in handle:lines() do
        table.insert(res, file)
    end
    handle:close()

    if #res == 0 then
        luabc.debug.log.info("no matched files with extension [" .. extension .. "]")
    end

    return res, #res
end

-- @brief Replaces the file extension of the given filename with a new extension
-- @param filename The original filename whose extension is to be replaced
-- @param new_extension The new extension to replace the old one (e.g., '.jpg')
-- @return The filename with the new extension
function luabc.tool.replace_file_extension(filename, new_extension)
    if not new_extension or not filename then
        luabc.debug.log.err("parameter <filename> and <new_extension> must be valid")
        return
    end
    if new_extension ~= "" and string.sub(new_extension, 1, 1) ~= "." then
        luabc.debug.log.err("you can't ignore dot")
        return
    end
    local new_filename, _ = string.gsub(filename, "%.[^%.]+$", new_extension)
    return new_filename
end

-- @brief Replaces the file extension of each file in the given list with a new extension
-- @param files A list of filenames whose extensions are to be replaced
-- @param new_extension The new extension to replace the old ones (e.g., '.jpg')
-- @return A list of filenames with the new extension applied to each
function luabc.tool.replace_files_extension(files, new_extension)
    if type(files) ~= "table" then
        luabc.debug.log.err("the files should be a table")
    end
    local res = {}
    for _, file in ipairs(files) do
        table.insert(res, luabc.tool.replace_file_extension(file, new_extension))
    end
    return res
end

-- @brief Deletes the specified file or files
-- @param file A single filename or a list of filenames to be deleted
function luabc.tool.clean(file)
    if type(file) == "table" then
        file = table.concat(file, " ")
    end
    local cmd = "rm -rf " .. file
    local status, msg = os.execute(cmd)
    if status then
        luabc.debug.log.ok("run successfully [ " .. cmd .. " ]")
    else
        luabc.debug.log.err("run failed [ " .. cmd .. " ] : " .. msg)
    end
end

-- @brief Get the directory and filename from path
-- @param path The original file path (string)
-- @param new_filename The new filename to replace the old one (string)
-- @param separator The path separator (optional, default is "/").
-- @return The directory and filename
function luabc.tool.get_dir_file(path, separator)
    separator = separator or "/"

    local dir, file = path:match("^(.-)" .. separator .. "([^" .. separator .. "]+)$")
    if not dir then dir = "./" end
    if string.sub(dir, -#separator) ~= separator then dir = dir .. separator end

    return dir, file
end

-- @brief Replaces the filename in the path with the provided new filename
-- @param path The original file path (string)
-- @param new_filename The new filename to replace the old one (string)
-- @param separator The path separator (optional, default is "/").
-- @return The new path with the replaced filename (string)
function luabc.tool.replace_path_filename(path, new_filename, separator)
    if type(path) ~= "string" then
        luabc.debug.log.err("path should be string type")
        return
    end
    if not new_filename or type(new_filename) ~= "string" then
        luabc.debug.log.err("parameter new_filename must be valid")
        return
    end
    separator = separator or "/"
    local old_dir, _ = luabc.tool.get_dir_file(path, separator)
    return old_dir .. new_filename
end

-- @brief Replaces the directory in the path with the provided new directory
-- @param path The original file path (string)
-- @param new_dir The new directory to replace the old one (string)
-- @param separator The path separator (optional, default is "/")
-- @return The new path with the replaced directory (string)
function luabc.tool.replace_path_directory(path, new_dir, separator)
    if type(path) ~= "string" then
        luabc.debug.log.err("path should be string type")
        return
    end
    separator = separator or "/"
    local _, old_file = luabc.tool.get_dir_file(path, separator)
    if string.sub(new_dir, -#separator) ~= separator then new_dir = new_dir .. separator end
    return new_dir .. old_file
end

-- @brief Replaces the directory in each path in the given list of paths with the provided new directory
-- @param paths A table containing paths (table)
-- @param new_dir The new directory to replace the old one (string)
-- @param separator The path separator (optional, default is "/")
-- @return A table containing the new paths with the replaced directories (table)
function luabc.tool.replace_paths_directory(paths, new_dir, separator)
    local res = {}
    if type(paths) == "table" then
        for _, path in ipairs(paths) do
            table.insert(res, luabc.tool.replace_path_directory(path, new_dir, separator))
        end
    else
        luabc.debug.log.err("paths should be a table")
        return
    end
    return res
end

--===================================================================================
--================================================================= luabc.debug
--===================================================================================

function luabc.debug.log.err(msg)
    error("[" .. color.RED .. "ERROR" .. color.RESET .. "] " .. msg)
end

function luabc.debug.log.ok(msg)
    print("[" .. color.GREEN .. "OK" .. color.RESET .. "] " .. msg)
end

function luabc.debug.log.info(msg)
    print("[" .. color.CYAN .. "INFO" .. color.RESET .. "] " .. msg)
end

-- @brief Dumps the contents of an object, for debugging purposes
-- @param obj The object to be dumped, which can be a simple or nested table
-- @param level The depth level to which the object should be dumped (for nested tables)
-- @note Allow nested tables
function luabc.debug.dump(obj, level)
    if not obj then 
        luabc.debug.log.err("invalid obj to be dumpped")
        return
    end

    level = level or 1
    local indent = string.rep("    ", level)
    if type(obj) ~= "table" then
        print(indent .. tostring(obj))
        return
    end
    
    for key, value in pairs(obj) do
        local formatted_key = tostring(key)
        if type(value) == "table" then
            print(indent ..  formatted_key .. " = {")
            luabc.debug.dump(value, level + 1)
            print(indent .. "}")
        else
            print(indent .. formatted_key .. " = " .. tostring(value))
        end
    end
end

-- @brief Prints the contents of an object. If the object is a table, it concatenates the table elements into a string
-- @param obj The object to be printed. If it’s a table, its elements are concatenated
-- @param is_concat A boolean that determines whether the table elements should be concatenated (true) or printed individually (false)
-- @note Only table elements that can be converted to strings are allowed, and only one level of nesting is supported
function luabc.debug.print(obj, is_concat)
    if not obj then 
        luabc.debug.log.err("invalid obj to be print")
        return
    end

    if is_concat == nil then is_concat = false else is_concat = is_concat and true end
    if type(obj) == "table" then
        if not is_concat then
            for key, val in ipairs(obj) do 
                print(key, (type(val) == "table") and table.unpack(val) or val)
            end
        else
            local res = ""
            for key, val in ipairs(obj) do 
                if type(val) == "table" then
                    res = res .. table.concat(val)
                else
                    res = res .. val
                end
            end
            print(res)
        end
    else
        print(obj)
    end
end

return luabc
