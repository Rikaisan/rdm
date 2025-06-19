#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace rdm {
    inline void ltrim(std::string &str);
    inline void rtrim(std::string &str);
    void trim(std::string &str);
    
    bool isAllowedPath(fs::path base, fs::path userPath, bool mustExist);
    std::vector<fs::path> getDirectoryFilesRecursive(fs::path root);

    fs::path getDataDir();
    fs::path getUserHome();
    void ensureDataDirExists();
}
