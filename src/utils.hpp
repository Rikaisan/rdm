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
    
    bool isAllowedPath(const fs::path &base, const fs::path &userPath, bool mustExist);
    std::vector<fs::path> getDirectoryFilesRecursive(const fs::path &root);
    bool fileMatchesPattern(const std::string &fileName, const std::string &pattern);

    fs::path getDataDir();
    fs::path getBackupDir();
    fs::path getBackupDir(const std::string &group);
    fs::path getUserHome();
    void ensureDataDirExists(bool populate);
    bool copyRDMLib();
    void setupBackupDir();
    void setupBackupDir(const std::string &group);
    bool backupEntry(const std::string &group, const fs::path &entry);
    void copyFileOrSym(const fs::path &source, const fs::path &dest);

    ModulesAndFlags parseModulesAndFlags(char* argv[], int count);
    bool parseAndInsertFlag(ModulesAndFlags& maf, const std::string &flag);
}
