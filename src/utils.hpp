#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_set>

namespace fs = std::filesystem;

namespace rdm {
    enum class Flag {
        VERBOSE
    };

    inline void ltrim(std::string &str);
    inline void rtrim(std::string &str);
    void trim(std::string &str);
    
    bool isAllowedPath(fs::path base, fs::path userPath, bool mustExist);
    std::vector<fs::path> getDirectoryFilesRecursive(fs::path root);
    bool fileMatchesPattern(std::string fileName, std::string pattern);

    fs::path getDataDir();
    fs::path getUserHome();
    void ensureDataDirExists(bool populate);
    bool copyRDMLib();

    bool isFlagPresent(Flag flag, std::unordered_set<std::string> flags);
}
