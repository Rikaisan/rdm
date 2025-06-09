#include "utils.hpp"
#include "Logger.hpp"
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

std::filesystem::path rdm::getUserHome() {
    return std::getenv("HOME");
}

std::filesystem::path rdm::getDataDir() {
    std::filesystem::path rdmDataDir;
    std::string dataHome = std::getenv("XDG_DATA_HOME") != NULL ? std::getenv("XDG_DATA_HOME") : "";
    trim(dataHome);
    if (dataHome == "" || dataHome.starts_with('.')) {
        std::string userHome = getUserHome();
        rdmDataDir = std::filesystem::path(userHome);
        rdmDataDir /= ".local/share";
    } else {
        rdmDataDir = std::filesystem::path(dataHome);
    }
    rdmDataDir /= "rdm";
    return rdmDataDir;
}

bool rdm::isAllowedPath(std::filesystem::path base, std::filesystem::path userPath, bool mustExist) {
    LOG_DEBUG("Validating path: " << userPath);
    std::filesystem::path absoluteBase = std::filesystem::weakly_canonical(base);
    std::filesystem::path absoluteUser = std::filesystem::weakly_canonical(userPath);

    LOG_DEBUG("Absolute base: " << absoluteBase);
    LOG_DEBUG("Absolute user: " << absoluteUser);

    if (mustExist && !std::filesystem::exists(absoluteBase)) {
        LOG_DEBUG("Path doesn't exist: " << absoluteBase);
        LOG_DEBUG("Denied!");
        return false;
    }

    if (mustExist && !std::filesystem::exists(absoluteUser)) {
        LOG_DEBUG("Path doesn't exist: " << absoluteUser);
        LOG_DEBUG("Denied!");
        return false;
    }

    bool isValid = absoluteUser.string().starts_with(absoluteBase.c_str());
    if (isValid) { LOG_DEBUG("Pass!"); } else { LOG_DEBUG("Denied!"); }
    return isValid;
}

void rdm::ensureDataDirExists() {
    auto dataDir = getDataDir();
    if (!std::filesystem::exists(dataDir)) {
        std::filesystem::create_directory(dataDir);
    }
    
    auto homeDataDir = getDataDir() / "home";
    if (!std::filesystem::exists(homeDataDir)) {
        std::filesystem::create_directory(homeDataDir);
    }
    
    // Maybe have a dirtectory for system files?
    // auto rootDataDir = getDataDir();
    // if (!std::filesystem::exists(rootDataDir)) {
    //     std::filesystem::create_directory(rootDataDir);
    // }
}