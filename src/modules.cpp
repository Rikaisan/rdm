#include "modules.hpp"
#include <string>
#include "Logger.hpp"
#include "utils.hpp"
#include "api.hpp"

namespace rdm {
    const std::string ModuleManager::MODULE_PREFIX = "rdm-";
    const char* Module::LUA_FILE_DIR = "MODULE_ROOT";
    std::unordered_set<std::string> ModuleManager::m_userModules;
    std::unordered_set<std::string> ModuleManager::m_userFlags;
    fs::path Module::currentlyExecutingFile;

    Module::Module(fs::path modulePath, fs::path destinationRoot)
    : m_path(modulePath)
    , m_destinationRoot(destinationRoot)
    , m_name(Module::getNameFromPath(modulePath)) {
        this->setupLuaState();
    }

    Module::Module(Module&& other)
    : m_path(std::move(other.m_path))
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

    FileContentMap Module::getGeneratedFiles() const {
        FileContentMap files;
        if (!lua_istable(m_state, -1)) return FileContentMap();

        lua_pushnil(m_state);
        while (lua_next(m_state, -2)) {
            if (lua_isstring(m_state, -2)) {
                std::string key = lua_tostring(m_state, -2);
                if (lua_isstring(m_state, -1)) {
                    std::string value = lua_tostring(m_state, -1);
                    if (isAllowedPath(m_destinationRoot, m_destinationRoot / key, false))
                        files.emplace(m_destinationRoot / key, value); // FIXME: What if the same file is specified twice? (relative paths)
                }
            }
            lua_pop(m_state, 1);
        }

        return files;
    }

    std::string Module::getPath() const { return m_path; }

    std::string Module::getName() const { return m_name; }

    int Module::setupLuaState() {
        currentlyExecutingFile = m_path;
        m_state = luaL_newstate();
        lua_pushstring(m_state, m_path.parent_path().c_str());
        lua_setglobal(m_state, LUA_FILE_DIR);
        lua_register(m_state, "Read", lapi_Read);
        lua_register(m_state, "FlagIsSet", lapi_FlagIsSet);
        lua_register(m_state, "OptionIsSet", lapi_FlagIsSet);
        lua_register(m_state, "ModuleIsSet", lapi_ModuleIsSet);
        lua_register(m_state, "ForceSpawn", lapi_ForceSpawn);
        lua_register(m_state, "Spawn", lapi_Spawn);
        luaL_openlibs(m_state); // FIXME: Change to only load safe libs (?) maybe allow an --allow-unsafe flag?
        m_luaExitCode = luaL_dofile(m_state, m_path.c_str());
        if (m_luaExitCode != LUA_OK) {
            m_luaErrorString = lua_tostring(m_state, -1);
        }
        return m_luaExitCode;
    }

    std::string Module::getNameFromPath(std::filesystem::path path) {
        return path.stem().string().substr(ModuleManager::MODULE_PREFIX.length());
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
        ModuleManager::m_userModules = std::unordered_set<std::string>();
        ModuleManager::m_userFlags = std::unordered_set<std::string>();

        m_userFlags.reserve(m_userFlags.size());
        for (auto& flag : maf.flags) {
            m_userFlags.insert(flag);
        }

        m_userModules.reserve(m_userModules.size());
        for (auto& module : maf.modules) {
            m_userModules.insert(module);
        }
        this->refreshModules();
    }

    void ModuleManager::refreshModules() {
        m_modules = ModuleManager::getModules(m_root, m_destinationRoot);
    }

    const ModuleList& ModuleManager::getModules() {
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
                if (fileName.starts_with(MODULE_PREFIX) && fileName.ends_with(".lua") && m_userModules.contains(Module::getNameFromPath(fileName))) {
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
                files.emplace(fileKV.first, fileKV.second); // FIXME: What if the same file is specified twice?
            }
        }
        
        return files;
    }

    bool ModuleManager::isFlagSet(std::string flag) {
        return m_userFlags.contains(flag);
    }

    bool ModuleManager::isModuleSet(std::string module) {
        return m_userModules.contains(module);
    }

    bool ModuleManager::shouldProcessAllModules() {
        return m_userModules.empty();
    }

    bool ModuleManager::shouldProcessModule(std::string module) {
        return m_userModules.contains(module);
    }
}