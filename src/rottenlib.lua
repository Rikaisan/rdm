--- @meta

--- Get the content of a file
--- @param filename string
--- @return string
function Read(filename) end

--- Get a boolean representing if a specific flag was specified by the user
--- @param option string
--- @return boolean
function OptionIsSet(option) end

--- Get a boolean representing if a specific flag was specified by the user
--- @param flag string
--- @return boolean
function FlagIsSet(flag) end

--- Get a boolean representing if a specific module was specified by the user
--- @param module string
--- @return boolean
function ModuleIsSet(module) end

--- Run a script or binaary
--- @param filename string
function Spawn(filename) end

--- Run a script or binaary, add exec permission if not present
--- @param filename string
function ForceSpawn(filename) end