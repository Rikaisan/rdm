#include <unordered_map>
#include <filesystem>
#include <iostream>
#include <string.h>
#include <fstream>
#include <lua.hpp>
#include <vector>

#include "Logger.hpp"

namespace fs = std::filesystem;
using FileContentMap = std::unordered_map<std::string, std::string>;
using FileList = std::vector<fs::path>;

const char* LUA_FILE_DIR = "RDM_FILE_ROOT";
const char* MODULE_PREFIX = "rdm-";

fs::path currentlyExecutingFile;

bool isAllowedPath(fs::path base, fs::path userPath) {
    LOG_DEBUG("Validating path: " << userPath);
    fs::path absoluteBase = fs::absolute(base);
    fs::path absoluteUser = fs::absolute(userPath);

    LOG_DEBUG("Absolute base: " << absoluteBase);
    LOG_DEBUG("Absolute user: " << absoluteUser);

    if (fs::exists(absoluteBase)) {
        absoluteBase = fs::canonical(absoluteBase);
        LOG_DEBUG("Canonical base: " << absoluteBase);
    }

    if (fs::exists(absoluteUser)) {
        absoluteUser = fs::canonical(absoluteUser);
        LOG_DEBUG("Canonical user: " << absoluteUser);
    } else {
        absoluteUser = "";
        LOG_DEBUG("Canonical user: INVALID");
    }

    bool isValid = absoluteUser.string().starts_with(absoluteBase.c_str());
    if (isValid) { LOG_DEBUG("Pass!"); } else { LOG_DEBUG("Denied!"); }
    return isValid;
}

int lapi_Read(lua_State* L) {
    if (lua_gettop(L) != 1) {
        lua_pushstring(L, "");
    } else {
        if (!lua_isstring(L, -1)) {
            lua_pushstring(L, "");
            return 1;
        }

        std::string fileName = lua_tostring(L, -1);

        fs::path fileToRead(currentlyExecutingFile.parent_path());
        fileToRead.append(fileName);

        if (!isAllowedPath(currentlyExecutingFile.parent_path(), fileToRead)) {
            lua_pushstring(L, "");
            return 1;
        }

        std::string buff;
        std::ifstream file;
        file.open(fileToRead);
        if (file.is_open()) {
            std::string line;
            while (getline(file, line)) {
                buff.append(line);
                buff.append("\n");
            }
            buff.pop_back();
            file.close();
        }
        lua_pushstring(L, buff.c_str());
    }
    return 1;
}

int lapi_OptionIsSet(lua_State* L) {
    int argc = lua_gettop(L);
    if (argc != 1) {
        lua_pushboolean(L, 0);
    } else {
        const char* option = lua_tostring(L, -1);
        if (strcmp(option, "hypr") == 0) {
            lua_pushboolean(L, 1);
        } else {
            lua_pushboolean(L, 0);
        }
    }
    return 1;
}

lua_State* setupLuaState(fs::path filePath) {
    currentlyExecutingFile = filePath;
    lua_State* L = luaL_newstate();
    lua_pushstring(L, filePath.parent_path().c_str());
    lua_setglobal(L, LUA_FILE_DIR);
    lua_pushcfunction(L, lapi_Read);
    lua_setglobal(L, "Read");
    lua_pushcfunction(L, lapi_OptionIsSet);
    lua_setglobal(L, "OptionIsSet");
    luaL_openlibs(L);
    luaL_dofile(L, filePath.c_str());
    return L;
}

FileContentMap getGeneratedFiles(lua_State* L) {
    FileContentMap files = FileContentMap();
    if (!lua_istable(L, -1)) return FileContentMap();

    lua_pushnil(L);
    while (lua_next(L, -2)) {
        if (lua_isstring(L, -2)) {
            std::string key = lua_tostring(L, -2);
            if (lua_isstring(L, -1)) {
                std::string value = lua_tostring(L, -1);
                files.emplace(key, value);
            }
        }
        lua_pop(L, 1);
    }

    return files;
}

FileList getModules(std::string root) {
    FileList modules;
    modules.reserve(32);

    for (auto& file : fs::directory_iterator(root)) {
        auto filePath = file.path();
        if (file.is_directory()) {
            FileList nestedModules = getModules(filePath);
            for (auto& script : nestedModules) {
                modules.emplace_back(script);
            }
        } else {
            std::string fileName = filePath.filename();
            if (fileName.starts_with(MODULE_PREFIX) && fileName.ends_with(".lua")) {
                modules.emplace_back(filePath);
            }
        }
    }

    return modules;
}

void printHelpMenu() {
    LOG("Usage: rdm <command> [args...]");
    LOG(" apply         Runs the scripts");
    LOG(" init          Initializes rdm with a git repository");
    LOG(" preview       Preview a specific module output");
}

std::string getModuleNameFromPath(std::filesystem::path path) {
    return path.stem().string().substr(4);
}

int main(int argc, char **argv) {
    if (argc == 1) {
        printHelpMenu();
        return EXIT_SUCCESS;
    }

    std::string cmd = argv[1];

    if (cmd == "preview") {
        if (argc < 3) {
            LOG_ERR("Not enough arguments supplied");
            LOG("Usage: rdm preview <module_name>");
            LOG(" <module_name>      rdm script with no prefix or extension (e.g. rdm-hyprland.lua -> hyprland)");
            return EXIT_FAILURE;
        }
        std::string moduleName = argv[2];

        FileList modules = getModules("home");
        for (auto& modulePath: modules) {
            if (getModuleNameFromPath(modulePath) == moduleName) {
                lua_State* L = setupLuaState(modulePath.c_str());
                FileContentMap generatedFiles = getGeneratedFiles(L);
                lua_close(L);

                if (generatedFiles.empty()) {
                    LOG("The module '" << moduleName << "' was found but returned no files");
                    return EXIT_SUCCESS;
                }

                LOG_SEP();
                for (auto& fileKV : generatedFiles) {
                    LOG(fileKV.first << ":");
                    LOG(fileKV.second);
                    LOG_SEP();
                }

                return EXIT_SUCCESS;
            }
        }

        LOG_ERR("Couldn't find module '" << moduleName << "'");
    } else if (cmd == "apply") {
        LOG_WARN("Unimplemented");
        return EXIT_FAILURE;
    } else if (cmd == "init") {
        LOG_WARN("Unimplemented");
        return EXIT_FAILURE;
    } else {
        LOG("Unrecognized command '" << cmd << "'");
        printHelpMenu();
    }
    
    return EXIT_SUCCESS;
}
