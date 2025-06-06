#include "utils.hpp"
#include <algorithm>

inline void rdm::ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

inline void rdm::rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

void rdm::trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

std::filesystem::path rdm::getDataDir() {
    std::filesystem::path rdmDataDir;
    std::string dataHome = std::getenv("XDG_DATA_HOME") != NULL ? std::getenv("XDG_DATA_HOME") : "";
    trim(dataHome);
    if (dataHome == "" || dataHome.starts_with('.')) {
        std::string userHome = std::getenv("HOME");
        rdmDataDir = std::filesystem::path(userHome);
        rdmDataDir /= ".local/share";
    } else {
        rdmDataDir = std::filesystem::path(dataHome);
    }
    rdmDataDir /= "rdm";
    return rdmDataDir;
}