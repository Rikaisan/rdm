#pragma once
#include <filesystem>
#include <string>

namespace rdm {
    inline void ltrim(std::string &str);
    inline void rtrim(std::string &str);
    void trim(std::string &str);
    
    bool isAllowedPath(std::filesystem::path base, std::filesystem::path userPath, bool mustExist);
    bool isAllowedPath(std::filesystem::path base, std::filesystem::path userPath, bool mustExist);

    std::filesystem::path getDataDir();
    std::filesystem::path getUserHome();
    void ensureDataDirExists();
}
