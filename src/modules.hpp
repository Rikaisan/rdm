#pragma once

#include <unordered_set>
#include <unordered_map>
#include <filesystem>
#include <lua.hpp>
#include <vector>

namespace fs = std::filesystem;

using FileContentMap = std::unordered_map<std::string, std::string>;
using FileList = std::vector<fs::path>;


namespace rdm {
    struct ModulesAndFlags {
        std::unordered_set<std::string> modules;
        std::unordered_set<std::string> flags;
    };

    class Module;
    using ModuleList = std::unordered_map<std::string, Module>;

    int lapi_Read(lua_State* L);
    int lapi_OptionIsSet(lua_State* L);
    
    class Module {
        public:
        Module(fs::path path);
        Module(Module&& other);
        Module(Module& other) = delete;
        Module& operator=(const Module&) = delete;
        FileContentMap getGeneratedFiles() const;
        std::string getPath() const;
        int getExitCode() const;
        std::string getErrorString() const;
        std::string getName() const;
        ~Module();

        private:
        int setupLuaState();

        static std::string getNameFromPath(std::filesystem::path path);

        const fs::path m_path;
        const std::string m_name;

        lua_State* m_state;
        int m_luaExitCode;
        std::string m_luaErrorString;

        static const char* LUA_FILE_DIR;
    };

    class ModuleManager {
        public:
        ModuleManager(fs::path root);
        ModuleManager(fs::path root, ModulesAndFlags& maf);
        void refreshModules();
        const ModuleList& getModules();
        FileContentMap getGeneratedFiles();

        static bool isAllowedPath(fs::path base, fs::path userPath);
        static ModuleList getModules(fs::path root);
        static bool isFlagSet(std::string flag);
        static bool isModuleSet(std::string module);

        static const std::string MODULE_PREFIX;
        private:
        const fs::path m_root;
        ModuleList m_modules;
        static std::unordered_set<std::string> m_userModules;
        static std::unordered_set<std::string> m_userFlags;
    };
}