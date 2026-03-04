#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;

namespace rdm {
    enum class Flag {
        VERBOSE
    };

    struct ModulesAndFlags {
        std::unordered_set<std::string> modules;
        std::unordered_set<std::string> flags;
        std::unordered_set<Flag> programFlags;
    };

    extern const std::unordered_map<std::string, Flag> FLAG_MAP;

    inline void ltrim(std::string &str);
    inline void rtrim(std::string &str);
    void trim(std::string &str);
    
    bool isAllowedPath(fs::path base, fs::path userPath, bool mustExist);
    std::vector<fs::path> getDirectoryFilesRecursive(fs::path root);
    bool fileMatchesPattern(std::string fileName, std::string pattern);

    fs::path getDataDir();
    fs::path getBackupDir();
    fs::path getBackupDir(std::string group);
    fs::path getUserHome();
    void ensureDataDirExists(bool populate);
    bool copyRDMLib();
    void setupBackupDir();
    void setupBackupDir(std::string group);
    bool backupEntry(std::string group, fs::path entry);
    void copyFileOrSym(fs::path source, fs::path dest);

    ModulesAndFlags parseModulesAndFlags(char* argv[], int count);
    bool parseAndInsertFlag(ModulesAndFlags& maf, std::string flag);
}
