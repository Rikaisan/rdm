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
    
    class Module {
        public:
        Module(fs::path path, fs::path destinationRoot);
        Module(Module&& other);
        Module(Module& other) = delete;
        Module& operator=(const Module&) = delete;
        FileContentMap getGeneratedFiles() const;
        std::string getPath() const;
        int getExitCode() const;
        std::string getErrorString() const;
        std::string getName() const;
        ~Module();

        static fs::path currentlyExecutingFile;

        private:
        int setupLuaState();

        static std::string getNameFromPath(std::filesystem::path path);

        const fs::path m_path;
        const fs::path m_destinationRoot;
        const std::string m_name;

        lua_State* m_state;
        int m_luaExitCode;
        std::string m_luaErrorString;

        static const char* LUA_FILE_DIR;
    };

    class ModuleManager {
        public:
        ModuleManager(fs::path root, fs::path destinationRoot);
        ModuleManager(fs::path root, fs::path destinationRoot, ModulesAndFlags& maf);
        void refreshModules();
        const ModuleList& getModules();
        FileContentMap getGeneratedFiles();

        static bool shouldProcessAllModules();
        static bool shouldProcessModule(std::string module);
        static ModuleList getModules(fs::path root, fs::path destinationRoot);
        static bool isFlagSet(std::string flag);
        static bool isModuleSet(std::string module);

        static const std::string MODULE_PREFIX;
        private:
        const fs::path m_root;
        const fs::path m_destinationRoot;
        ModuleList m_modules;
        static std::unordered_set<std::string> m_userModules;
        static std::unordered_set<std::string> m_userFlags;
    };
}