#include "modules.hpp"
#include "Logger.hpp"
#include <fstream>
#include <string>

namespace rdm {
    fs::path currentlyExecutingFile;

    const std::string ModuleManager::MODULE_PREFIX = "rdm-";
    const char* Module::LUA_FILE_DIR = "RDM_FILE_ROOT";
    std::unordered_set<std::string> ModuleManager::m_flags;

    int lapi_Read(lua_State* L) {
        if (lua_gettop(L) != 1) {
            lua_pushstring(L, "");
        } else {
            if (!lua_isstring(L, -1)) {
                lua_pushstring(L, "");
                return 1;
            }

            std::string fileName = lua_tostring(L, -1);

            fs::path fileToRead(currentlyExecutingFile.parent_path());
            fileToRead.append(fileName);

            if (!ModuleManager::isAllowedPath(currentlyExecutingFile.parent_path(), fileToRead)) {
                lua_pushstring(L, "");
                return 1;
            }

            std::string buff;
            std::ifstream file;
            file.open(fileToRead);
            if (file.is_open()) {
                std::string line;
                while (getline(file, line)) {
                    buff.append(line);
                    buff.append("\n");
                }
                buff.pop_back();
                file.close();
            }
            lua_pushstring(L, buff.c_str());
        }
        return 1;
    }

    int lapi_OptionIsSet(lua_State* L) {
        int argc = lua_gettop(L);
        if (argc != 1) {
            lua_pushboolean(L, 0);
        } else {
            std::string flag = lua_tostring(L, -1);
            lua_pushboolean(L, ModuleManager::isFlagSet(flag));
        }
        return 1;
    }

    Module::Module(fs::path modulePath)
    : m_path(modulePath)
    , m_name(Module::getNameFromPath(modulePath)) {
        this->setupLuaState();
    }

    Module::Module(Module&& other)
    : m_path(std::move(other.m_path))
    , m_name(std::move(other.m_name))
    , m_state(other.m_state) {
        other.m_state = nullptr;
    }
    
    Module::~Module() { if (m_state != nullptr) lua_close(m_state); }

    FileContentMap Module::getGeneratedFiles() const {
        FileContentMap files;
        if (!lua_istable(m_state, -1)) return FileContentMap();

        lua_pushnil(m_state);
        while (lua_next(m_state, -2)) {
            if (lua_isstring(m_state, -2)) {
                std::string key = lua_tostring(m_state, -2);
                if (lua_isstring(m_state, -1)) {
                    std::string value = lua_tostring(m_state, -1);
                    files.emplace(key, value); // FIXME: What if the same file is specified twice? (relative paths)
                }
            }
            lua_pop(m_state, 1);
        }

        return files;
    }

    std::string Module::getPath() const { return m_path; }

    std::string Module::getName() const { return m_name; }

    void Module::setupLuaState() {
        currentlyExecutingFile = m_path;
        m_state = luaL_newstate();
        lua_pushstring(m_state, m_path.parent_path().c_str());
        lua_setglobal(m_state, LUA_FILE_DIR);
        lua_pushcfunction(m_state, lapi_Read);
        lua_setglobal(m_state, "Read");
        lua_pushcfunction(m_state, lapi_OptionIsSet);
        lua_setglobal(m_state, "OptionIsSet");
        luaL_openlibs(m_state);
        luaL_dofile(m_state, m_path.c_str());
    }

    std::string Module::getNameFromPath(std::filesystem::path path) {
        return path.stem().string().substr(ModuleManager::MODULE_PREFIX.length());
    }

    ModuleManager::ModuleManager(fs::path root): m_root(root) {
        this->refreshModules();
    }

    ModuleManager::ModuleManager(fs::path root, std::unordered_set<std::string>& flags): m_root(root) {
        ModuleManager::m_flags = std::unordered_set<std::string>();
        m_flags.reserve(m_flags.size());
        for (auto& flag : flags) {
            m_flags.insert(flag);
        }
        this->refreshModules();
    }

    void ModuleManager::refreshModules() {
        m_modules = ModuleManager::getModules(m_root);
    }

    const ModuleList& ModuleManager::getModules() {
        return m_modules;
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

    bool ModuleManager::isAllowedPath(fs::path base, fs::path userPath) {
        LOG_DEBUG("Validating path: " << userPath);
        fs::path absoluteBase = fs::absolute(base);
        fs::path absoluteUser = fs::absolute(userPath);

        LOG_DEBUG("Absolute base: " << absoluteBase);
        LOG_DEBUG("Absolute user: " << absoluteUser);

        if (fs::exists(absoluteBase)) {
            absoluteBase = fs::canonical(absoluteBase);
            LOG_DEBUG("Canonical base: " << absoluteBase);
        }

        if (fs::exists(absoluteUser)) {
            absoluteUser = fs::canonical(absoluteUser);
            LOG_DEBUG("Canonical user: " << absoluteUser);
        } else {
            absoluteUser = "";
            LOG_DEBUG("Canonical user: INVALID");
        }

        bool isValid = absoluteUser.string().starts_with(absoluteBase.c_str());
        if (isValid) { LOG_DEBUG("Pass!"); } else { LOG_DEBUG("Denied!"); }
        return isValid;
    }

    ModuleList ModuleManager::getModules(fs::path root) {
        ModuleList modules;
        modules.reserve(32);

        for (auto& file : fs::directory_iterator(root)) {
            auto filePath = file.path();
            if (file.is_directory()) {
                ModuleList nestedModules = ModuleManager::getModules(filePath);
                for (auto& module : nestedModules) {
                    modules.insert(std::move(module));
                }
            } else {
                std::string fileName = filePath.filename();
                if (fileName.starts_with(MODULE_PREFIX) && fileName.ends_with(".lua")) {
                    Module module = Module(filePath);
                    modules.emplace(module.getName(), std::move(module));
                }
            }
        }


        return modules;
    }

    bool ModuleManager::isFlagSet(std::string flag) {
        return m_flags.contains(flag);
    }
}