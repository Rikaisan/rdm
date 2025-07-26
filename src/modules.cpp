#include "modules.hpp"
#include <string>
#include "Logger.hpp"
#include "utils.hpp"
#include "api.hpp"

namespace rdm {
    const std::string ModuleManager::MODULE_PREFIX = "rdm-";
    const char* Module::LUA_FILE_DIR = "MODULE_ROOT";
    std::unordered_set<std::string> ModuleManager::s_userModules;
    std::unordered_set<std::string> ModuleManager::s_userFlags;
    fs::path Module::s_currentlyExecutingFile;

    FileData::FileData(std::string content) {
        m_dataType = FileDataType::Text;
        m_content = content;
    }

    FileData::FileData(fs::path path, FileDataType dataType) : m_dataType(dataType) {
        if (dataType == FileDataType::Text) {
            m_content = std::string(path);
        } else {
            m_filePath = path;
        }
    }

    FileData::FileData(FileData&& other) {
        m_dataType = other.m_dataType;
        if (other.m_dataType == FileDataType::Text) {
            m_content = other.m_content;
            other.m_content = std::string();
        } else {
            m_filePath = other.m_filePath;
            other.m_filePath = fs::path();
        }
    }

    std::string FileData::getContent() {
    return m_dataType == FileDataType::Text ? std::get<std::string>(m_content) : std::string();
    }

    fs::path FileData::getPath() {
        return m_dataType != FileDataType::Text ? std::get<fs::path>(m_filePath) : fs::path();
    }

    FileDataType FileData::getDataType() {
        return m_dataType;
    }

    Module::Module(fs::path modulePath, fs::path destinationRoot)
    : m_modulePath(modulePath)
    , m_destinationRoot(destinationRoot)
    , m_name(Module::getNameFromPath(modulePath)) {
        this->setupLuaState();
    }

    Module::Module(Module&& other)
    : m_modulePath(std::move(other.m_modulePath))
    , m_destinationRoot(std::move(other.m_destinationRoot))
    , m_name(std::move(other.m_name))
    , m_state(other.m_state)
    , m_luaExitCode(other.m_luaExitCode)
    , m_luaErrorString(std::move(other.m_luaErrorString)) {
        other.m_state = nullptr;
    }
    
    Module::~Module() { if (m_state != nullptr) lua_close(m_state); }

    int Module::getExitCode() const {
        return m_luaExitCode;
    }

    std::string Module::getErrorString() const {
        return m_luaErrorString;
    }

    FileContentMap Module::getGeneratedFiles() {
        if (m_luaExitCode != LUA_OK) return FileContentMap();
        s_currentlyExecutingFile = m_modulePath;
        LOG_CUSTOM_DEBUG(m_name, "Started generating files");
        FileContentMap files;
        lua_State* L = m_state;
        if (lua_getglobal(L, "RDM_GetFiles") != LUA_TFUNCTION) {
            lua_pop(L, 1);
            return FileContentMap();
        }

        m_luaExitCode = lua_pcall(L, 0, 1, 0);
        if (m_luaExitCode != LUA_OK) {
            m_luaErrorString = lua_tostring(m_state, -1);
            return FileContentMap();
        }

        if (!lua_istable(L, -1)) return FileContentMap();

        lua_pushnil(L);
        while (lua_next(L, -2)) {
            if (lua_isstring(L, -2)) {
                std::string key = lua_tostring(L, -2);
                if (lua_isstring(L, -1)) {
                    FileData value(lua_tostring(L, -1));
                    fs::path userPath = m_destinationRoot / key;
                    if (isAllowedPath(m_destinationRoot, userPath, false)) {
                        LOG_CUSTOM_DEBUG(m_name, "Added text file");
                        files.emplace(userPath, std::move(value)); // FIXME: What if the same file is specified twice? (relative paths)
                    }
                }
                else if (lua_istable(L, -1)) {
                    LOG_CUSTOM_DEBUG(m_name, "Found FileDescriptor");
                    int valueType = lua_getfield(L, -1, "type");
                    if (valueType == LUA_TSTRING) {
                        std::string dataType = lua_tostring(L, -1);
                        LOG_CUSTOM_DEBUG(m_name, "FileDescriptor type: " << dataType);
                        if (dataType == "bytes" || dataType == "directory") {
                            int path = lua_getfield(L, -2, "path");
                            if (path == LUA_TSTRING) {
                                FileDataType fileDataType = FileDataType::RawData;
                                if (dataType == "directory") fileDataType = FileDataType::Directory;
                                FileData data(fs::path(lua_tostring(L, -1)), fileDataType);
                                LOG_CUSTOM_DEBUG(m_name, "Added file with type " << dataType);
                                files.emplace(m_destinationRoot / key, std::move(data)); // FIXME: What if the same file is specified twice? (relative paths)
                            } else {
                                LOG_CUSTOM_ERR(m_name, "Invalid value for file " << key << ": Invalid or non-present path");
                            }
                            lua_pop(L, 1);
                        } else {
                            LOG_CUSTOM_ERR(m_name, "Invalid value for file " << key << ": Unknown type");
                        }
                    } else {
                        LOG_CUSTOM_ERR(m_name, "Invalid value for file " << key << ": Invalid or non-present type");
                    }
                    lua_pop(L, 1);
                } else {
                    LOG_CUSTOM_ERR(m_name, "Invalid value for file " << key << ": Not a FileDescriptor or a string");
                }
            }
            lua_pop(L, 1);
        }

        LOG_CUSTOM_DEBUG(m_name, "Finished generating files");
        return files;
    }

    std::string Module::getPath() const { return m_modulePath; }

    std::string Module::getName() const { return m_name; }

    int Module::setupLuaState() {
        s_currentlyExecutingFile = m_modulePath;
        m_state = luaL_newstate();
        lua_pushstring(m_state, m_modulePath.parent_path().c_str());
        lua_setglobal(m_state, LUA_FILE_DIR);
        lua_register(m_state, "Read", lapi_Read);
        lua_register(m_state, "FlagIsSet", lapi_FlagIsSet);
        lua_register(m_state, "ModuleIsSet", lapi_ModuleIsSet);
        lua_register(m_state, "ForceSpawn", lapi_ForceSpawn);
        lua_register(m_state, "Spawn", lapi_Spawn);
        lua_register(m_state, "File", lapi_File);
        lua_register(m_state, "Directory", lapi_Directory);
        luaL_openlibs(m_state); // FIXME: Change to only load safe libs (?) maybe allow an --allow-unsafe flag?
        m_luaExitCode = luaL_dofile(m_state, m_modulePath.c_str());
        if (m_luaExitCode != LUA_OK) {
            m_luaErrorString = lua_tostring(m_state, -1);
        } else {
            callLuaMethod("RDM_Init");
        }
        return m_luaExitCode;
    }

    std::string Module::getNameFromPath(std::filesystem::path path) {
        return path.stem().string().substr(ModuleManager::MODULE_PREFIX.length());
    }

    bool Module::runDelayed() {
        s_currentlyExecutingFile = m_modulePath;
        return callLuaMethod("RDM_Delayed");
    }

    bool Module::callLuaMethod(std::string name) {
        if (m_luaExitCode != LUA_OK) return false;
        if (lua_getglobal(m_state, name.c_str()) == LUA_TFUNCTION) {
            m_luaExitCode = lua_pcall(m_state, 0, 0, 0);
            if (m_luaExitCode != LUA_OK) {
                m_luaErrorString = lua_tostring(m_state, -1);
                return false;
            }
            return true;
        } else {
            lua_pop(m_state, 1);
            return false;
        }
    }


    ModuleManager::ModuleManager(fs::path root, fs::path destinationRoot)
    : m_root(root)
    , m_destinationRoot(destinationRoot)
    {
        this->refreshModules();
    }

    ModuleManager::ModuleManager(fs::path root, fs::path destinationRoot, ModulesAndFlags& maf)
    : m_root(root)
    , m_destinationRoot(destinationRoot)
    {
        ModuleManager::s_userModules = std::unordered_set<std::string>();
        ModuleManager::s_userFlags = std::unordered_set<std::string>();

        s_userFlags.reserve(s_userFlags.size());
        for (auto& flag : maf.flags) {
            s_userFlags.insert(flag);
        }

        s_userModules.reserve(s_userModules.size());
        for (auto& module : maf.modules) {
            s_userModules.insert(module);
        }
        this->refreshModules();
    }

    void ModuleManager::refreshModules() {
        m_modules = ModuleManager::getModules(m_root, m_destinationRoot);
    }

    ModuleList& ModuleManager::getModules() {
        return m_modules;
    }

    ModuleList ModuleManager::getModules(fs::path root, fs::path destinationRoot) {
        ModuleList modules;
        modules.reserve(32);

        for (auto& file : fs::directory_iterator(root)) {
            auto filePath = file.path();
            if (file.is_directory()) {
                ModuleList nestedModules = ModuleManager::getModules(filePath, destinationRoot);
                for (auto& module : nestedModules) {
                    modules.insert(std::move(module));
                }
            } else {
                std::string fileName = filePath.filename();
                if (fileName.starts_with(MODULE_PREFIX) && fileName.ends_with(".lua") && shouldProcessModule(Module::getNameFromPath(fileName))) {
                    Module module = Module(filePath, destinationRoot);
                    modules.emplace(module.getName(), std::move(module));
                }
            }
        }

        return modules;
    }

    FileContentMap ModuleManager::getGeneratedFiles() {
        FileContentMap files;

        for (auto& pair : m_modules) {
            for (auto& fileKV : pair.second.getGeneratedFiles()) {
                files.emplace(fileKV.first, std::move(fileKV.second)); // FIXME: What if the same file is specified twice?
            }
        }
        
        return files;
    }

    bool ModuleManager::isFlagSet(std::string flag) {
        return s_userFlags.contains(flag);
    }

    bool ModuleManager::shouldProcessAllModules() {
        return s_userModules.empty();
    }

    bool ModuleManager::shouldProcessModule(std::string module) {
        return shouldProcessAllModules() || s_userModules.contains(module);
    }
}