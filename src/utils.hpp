#pragma once
#include <filesystem>
#include <string>

namespace rdm {
    inline void ltrim(std::string &str);
    inline void rtrim(std::string &str);
    void trim(std::string &str);

    std::filesystem::path getDataDir();
}
