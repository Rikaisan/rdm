--- @meta

--- @alias FileDescriptor table Describes a file and how it should be parsed by rdm

--- Get the content of a file
--- @param filename string
--- @return string|nil
function Read(filename) end

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

--- Run a script or binary, add exec permission if not present
--- @param filename string
function ForceSpawn(filename) end

--- Describes that the file must be copied as is (in bytes), useful for non-text files
--- @param filename string
--- @return FileDescriptor|nil
function File(filename) end

--- Describes that rdm should copy the entire directory contents
--- @param filename string
--- @return FileDescriptor|nil
function Directory(filename) end