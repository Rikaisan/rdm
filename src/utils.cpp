#include "utils.hpp"
#include "Logger.hpp"
#include <algorithm>

#include "rdmlib.hpp"
#include <fstream>

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

fs::path rdm::getUserHome() {
    return std::getenv("HOME");
}

fs::path rdm::getDataDir() {
    fs::path rdmDataDir;
    std::string dataHome = std::getenv("XDG_DATA_HOME") != NULL ? std::getenv("XDG_DATA_HOME") : "";
    trim(dataHome);
    if (dataHome == "" || dataHome.starts_with('.')) {
        std::string userHome = getUserHome();
        rdmDataDir = fs::path(userHome);
        rdmDataDir /= ".local/share";
    } else {
        rdmDataDir = fs::path(dataHome);
    }
    rdmDataDir /= "rdm";
    return rdmDataDir;
}

bool rdm::isAllowedPath(fs::path base, fs::path userPath, bool mustExist) {
    LOG_DEBUG("Validating path: " << userPath);
    fs::path absoluteBase = fs::weakly_canonical(base);
    fs::path absoluteUser = fs::weakly_canonical(userPath);

    LOG_DEBUG("Absolute base: " << absoluteBase);
    LOG_DEBUG("Absolute user: " << absoluteUser);

    if (mustExist && !fs::exists(absoluteBase)) {
        LOG_DEBUG("Path doesn't exist: " << absoluteBase);
        LOG_DEBUG("Denied!");
        return false;
    }

    if (mustExist && !fs::exists(absoluteUser)) {
        LOG_DEBUG("Path doesn't exist: " << absoluteUser);
        LOG_DEBUG("Denied!");
        return false;
    }

    bool isValid = absoluteUser.string().starts_with(absoluteBase.c_str());
    if (isValid) { LOG_DEBUG("Pass!"); } else { LOG_DEBUG("Denied!"); }
    return isValid;
}

std::vector<fs::path> rdm::getDirectoryFilesRecursive(fs::path root) {
    std::vector<fs::path> files;
    files.reserve(8);
    for (auto& entry : fs::directory_iterator(root)) {
        if (entry.is_directory()) {
            auto nestedFiles = rdm::getDirectoryFilesRecursive(entry.path());
            for (auto& nestedFile : nestedFiles) {
                files.push_back(nestedFile);
            }
        } else {
            files.push_back(entry.path());
        }
    }
    return files;
}

void rdm::ensureDataDirExists() {
    auto dataDir = getDataDir();
    if (!fs::exists(dataDir)) {
        fs::create_directory(dataDir);
    }
    
    auto homeDataDir = getDataDir() / "home";
    if (!fs::exists(homeDataDir)) {
        fs::create_directory(homeDataDir);
    }
    
    // Maybe have a dirtectory for system files?
    // auto rootDataDir = getDataDir();
    // if (!fs::exists(rootDataDir)) {
    //     fs::create_directory(rootDataDir);
    // }
}

// TODO: Return true when it was able to copy it from the system location, false if it used the embedded one
bool rdm::copyRDMLib() {
    fs::path libPath = getDataDir() / "rdmlib.lua";
    if (fs::exists(libPath))
        fs::remove(libPath);
    
    std::ofstream libFile(libPath, std::fstream::binary | std::fstream::out);
    if (libFile.is_open()) {
        libFile.write(src_rdmlib_lua, src_rdmlib_lua_len);
        libFile.close();
    }

    return false;
}