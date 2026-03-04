#include "utils.hpp"
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <fnmatch.h>
#include "Logger.hpp"
#include "rdmlib.hpp"

const std::unordered_map<std::string, rdm::Flag> rdm::FLAG_MAP = {
    { "--verbose", Flag::VERBOSE },
    { "-v",        Flag::VERBOSE },
};

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

fs::path rdm::getBackupDir() {
    return getDataDir() / "backup";
}

fs::path rdm::getBackupDir(std::string group) {
    return getBackupDir() / group;
}

bool rdm::isAllowedPath(fs::path base, fs::path userPath, bool mustExist) {
    // LOG_DEBUG("Validating path: " << userPath);
    fs::path absoluteBase = fs::weakly_canonical(base);
    fs::path absoluteUser = fs::weakly_canonical(userPath);

    // LOG_DEBUG("Absolute base: " << absoluteBase);
    // LOG_DEBUG("Absolute user: " << absoluteUser);

    if (mustExist && !fs::exists(absoluteBase)) {
        LOG_DEBUG("Denied path: " << userPath);
        LOG_DEBUG("[Reason] Path doesn't exist: " << absoluteBase);
        return false;
    }

    if (mustExist && !fs::exists(absoluteUser)) {
        LOG_DEBUG("Denied path: " << userPath);
        LOG_DEBUG("[Reason] Path doesn't exist: " << absoluteUser);
        return false;
    }

    bool isValid = absoluteUser.string().starts_with(absoluteBase.c_str());
    if (!isValid) {
        LOG_DEBUG("Denied path: " << userPath);
        LOG_DEBUG("[Reason] Invalid access");
    }
    return isValid;
}

std::vector<fs::path> rdm::getDirectoryFilesRecursive(fs::path root) {
    std::vector<fs::path> files;
    files.reserve(8);
    for (auto& entry : fs::directory_iterator(root)) {
        if (entry.is_directory() && !entry.is_symlink()) {
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

bool rdm::fileMatchesPattern(std::string fileName, std::string pattern) {
    return fnmatch(pattern.c_str(), fileName.c_str(), FNM_EXTMATCH) != FNM_NOMATCH;
}

void rdm::ensureDataDirExists(bool populate) {
    fs::path dataDir = getDataDir();
    if (!fs::exists(dataDir)) {
        fs::create_directories(dataDir);
    }

    if (populate) {
        fs::path homeDataDir = getDataDir() / "home";
        if (!fs::exists(homeDataDir)) {
            fs::create_directories(homeDataDir);
        }

        setupBackupDir();

        if (!copyRDMLib()) LOG_ERR("Couldn't copy RDM lib, use the copy at /usr/share/rdm/rdmlib.lua as a fallback.");

        // Maybe have a dirtectory for system files?
        // auto rootDataDir = getDataDir();
        // if (!fs::exists(rootDataDir)) {
        //     fs::create_directories(rootDataDir);
        // }
    }
}

bool rdm::copyRDMLib() {
    fs::path destinationLibPath = getDataDir() / "rdmlib.lua";
    if (!fs::exists(destinationLibPath.parent_path())) return false; // Do not copy if data dir doesn't exist

    if (fs::exists(destinationLibPath))
        fs::remove(destinationLibPath);
    
    std::ofstream libFile(destinationLibPath, std::fstream::binary | std::fstream::out);
    if (libFile.is_open()) {
        libFile.write(src_rdmlib_lua, src_rdmlib_lua_len);
        libFile.close();
    }

    return true;
}

void rdm::setupBackupDir() {
    fs::path backupsDataDir = getBackupDir();

    if (fs::exists(backupsDataDir) && !fs::is_empty(backupsDataDir)) {
        fs::remove_all(backupsDataDir);
    }

    if (!fs::exists(backupsDataDir)) {
        fs::create_directories(backupsDataDir);
    }
}

void rdm::setupBackupDir(std::string group) {
    fs::path backupsDataDir = getBackupDir(group);

    if (fs::exists(backupsDataDir) && !fs::is_empty(backupsDataDir)) {
        fs::remove_all(backupsDataDir);
    }

    if (!fs::exists(backupsDataDir)) {
        fs::create_directories(backupsDataDir);
    }
}

bool rdm::backupEntry(std::string group, fs::path entry) {
    fs::path backup_entry = getBackupDir(group) / entry.lexically_relative(getUserHome());
    if (fs::exists(backup_entry)) return false;
    copyFileOrSym(entry, backup_entry);
    return true;
}

void rdm::copyFileOrSym(fs::path source, fs::path dest) {
    fs::create_directories(dest.parent_path());
    if (fs::is_symlink(source)) {
        fs::copy_symlink(source, dest);
    } else {
        fs::copy_file(source, dest);
    }
}

rdm::ModulesAndFlags rdm::parseModulesAndFlags(char* argv[], int count) {
    ModulesAndFlags maf;
    if (count == 0) return maf;

    std::vector<std::string> args;
    args.reserve(count);
    for (int i = 0; i < count; ++i) {
        args.emplace_back(argv[i]);
    }

    int currentArg = 0;

    while (currentArg < count) {
        auto& arg = args.at(currentArg++);
        if (arg.starts_with("-")) {
            if (arg == "-f" || arg == "--flags") break;
            if (!parseAndInsertFlag(maf, arg)) LOG_WARN("Found unknown flag '" << arg << "', ignoring it...");
            continue;
        }
        maf.modules.insert(arg);
    }

    while (currentArg < count) {
        auto& arg = args.at(currentArg++);
        if (arg.starts_with("-")) {
            if (!parseAndInsertFlag(maf, arg)) LOG_WARN("Found unknown flag '" << arg << "', ignoring it...");
            continue;
        }
        maf.flags.insert(arg);
    }

    return maf;
}

bool rdm::parseAndInsertFlag(ModulesAndFlags& maf, std::string flag) {
    if (!FLAG_MAP.contains(flag)) return false;
    maf.programFlags.insert(FLAG_MAP.at(flag));
    return true;
}