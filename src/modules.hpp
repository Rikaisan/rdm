#pragma once

#include <unordered_set>
#include <unordered_map>
#include <filesystem>
#include <lua.hpp>
#include <vector>
#include <variant>
#include <optional>
#include "utils.hpp"

namespace fs = std::filesystem;


namespace rdm {
    enum class FileDataType {
        RawData,
        Text,
        Directory
    };

    struct FileData {
        FileData(const std::string &content);
        FileData(const fs::path &path, FileDataType dataType);
        FileData(FileData&& other);
        FileData(FileData& other) = delete;
        std::string getContent() const;
        fs::path getPath() const;
        FileDataType getDataType() const;
        bool isExecutable() const;
        std::string getExecutablePattern() const;
        void setExecutable(bool executable);
        void setExecutableRules(const std::string &pattern);

        private:
        std::variant<std::string, fs::path> m_content, m_filePath;
        FileDataType m_dataType;
        std::string m_execPattern;
        bool m_isExecutable = false;
    };

    using FileContentMap = std::unordered_map<std::string, FileData>;
    using FileList = std::vector<fs::path>;

    class Module;
    using ModuleList = std::unordered_map<std::string, Module>;
    using ModulePaths = std::unordered_map<std::string, fs::path>;
    
    class Module {
        public:
        Module(const fs::path &path, const fs::path &destinationRoot);
        Module(Module&& other);
        Module(Module& other) = delete;
        Module& operator=(const Module&) = delete;
        std::optional<FileContentMap> getGeneratedFiles();
        std::unordered_set<std::string> getExtraModules();
        bool runInit();
        bool runDelayed();
        std::string getPath() const;
        int getExitCode() const;
        std::string getErrorString() const;
        std::string getName() const;
        ~Module();

        static std::string getNameFromPath(const fs::path &path);
        static fs::path getCurrentlyExecutingFile();

        
        private:
        int setupLuaState();
        bool callLuaMethod(const std::string &name);
        
        static fs::path s_currentlyExecutingFile;
        
        const fs::path m_modulePath;
        const fs::path m_destinationRoot;
        const std::string m_name;

        lua_State* m_state;
        int m_luaExitCode;
        std::string m_luaErrorString;

        static const char* LUA_FILE_DIR;
    };

    class ModuleManager {
        public:
        ModuleManager(const fs::path &root, const fs::path &destinationRoot);
        ModuleManager(const fs::path &root, const fs::path &destinationRoot, const ModulesAndFlags &maf);
        void refreshModules();
        ModuleList& getModules();
        ModulePaths& getAvailableModules();
        FileContentMap getGeneratedFiles();

        void runInits();
        void runDelayeds();

        static bool shouldProcessAllModules();
        static bool shouldProcessModule(const std::string &module);
        static ModuleList getModules(const fs::path &root, const fs::path &destinationRoot);
        static ModulePaths getAvailableModules(const fs::path &root);
        static bool isFlagSet(const std::string &flag);

        static const std::string MODULE_PREFIX;
        private:
        static bool updateModuleList(const fs::path &root, const fs::path &destinationRoot, ModuleList &moduleList);
        const fs::path m_root;
        const fs::path m_destinationRoot;
        ModuleList m_modules;
        static ModulePaths s_availableModules;
        static std::unordered_set<std::string> s_userModules;
        static std::unordered_set<std::string> s_queuedModules;
        static std::unordered_set<std::string> s_userFlags;
    };
}