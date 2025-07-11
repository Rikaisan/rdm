#include "api.hpp"
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include "utils.hpp"
#include "modules.hpp"
#include "Logger.hpp"

namespace fs = std::filesystem;

namespace rdm {
    int lapi_Read(lua_State* L) {
        if (lua_gettop(L) != 1) {
            lua_pushnil(L);
        } else {
            if (!lua_isstring(L, -1)) {
                lua_pushnil(L);
                return 1;
            }

            std::string fileName = lua_tostring(L, -1);

            fs::path fileToRead(Module::s_currentlyExecutingFile.parent_path());
            fileToRead.append(fileName);

            if (!isAllowedPath(Module::s_currentlyExecutingFile.parent_path(), fileToRead, true)) {
                lua_pushnil(L);
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

    int lapi_File(lua_State* L) {
        return createFileDescriptor(L, "bytes");
    }

    int lapi_Directory(lua_State* L) {
        return createFileDescriptor(L, "directory");
    }

    int lapi_Spawn(lua_State* L) {
        if (lua_gettop(L) == 1) {
            if (!lua_isstring(L, -1)) return 0;

            std::string fileName = lua_tostring(L, -1);

            fs::path fileToExec(Module::s_currentlyExecutingFile.parent_path());
            fileToExec.append(fileName);

            std::string name = Module::getNameFromPath(Module::s_currentlyExecutingFile);
            if (!isAllowedPath(Module::s_currentlyExecutingFile.parent_path(), fileToExec, true)) {
                LOG_CUSTOM_ERR(name, "File '" << fileName << "' is not allowed or doesn't exist.");
                lua_pushnil(L);
                return 1;
            }

            LOG_CUSTOM_INFO(name, "Executing '" << fileName << "'...");
            int exitCode = std::system(fileToExec.c_str());
            lua_pushnumber(L, exitCode);
        } else {
            lua_pushnil(L);
        }
        return 1;
    }

    int lapi_ForceSpawn(lua_State* L) {
        if (lua_gettop(L) == 1) {
            if (!lua_isstring(L, -1)) return 0;

            std::string fileName = lua_tostring(L, -1);

            fs::path fileToExec(Module::s_currentlyExecutingFile.parent_path());
            fileToExec.append(fileName);

            std::string name = Module::getNameFromPath(Module::s_currentlyExecutingFile);
            if (!isAllowedPath(Module::s_currentlyExecutingFile.parent_path(), fileToExec, true)) {
                LOG_CUSTOM_ERR(name, "File '" << fileName << "' is not allowed or doesn't exist.");
                lua_pushnil(L);
                return 1;
            }

            std::filesystem::permissions(fileToExec, std::filesystem::perms::owner_exec, std::filesystem::perm_options::add);

            LOG_CUSTOM_INFO(name, "Executing '" << fileName << "'...");
            int exitCode = std::system(fileToExec.c_str());
            lua_pushnumber(L, exitCode);
        } else {
            lua_pushnil(L);
        }
        return 1;
    }

    int lapi_ModuleIsSet(lua_State* L) {
        int argc = lua_gettop(L);
        if (argc != 1 || !lua_isstring(L, -1)) {
            lua_pushboolean(L, 0);
        } else {
            std::string module = lua_tostring(L, -1);
            lua_pushboolean(L, ModuleManager::shouldProcessModule(module));
        }
        return 1;
    }

    int lapi_FlagIsSet(lua_State* L) {
        int argc = lua_gettop(L);
        if (argc != 1 || !lua_isstring(L, -1)) {
            lua_pushboolean(L, 0);
        } else {
            std::string flag = lua_tostring(L, -1);
            lua_pushboolean(L, ModuleManager::isFlagSet(flag));
        }
        return 1;
    }

    int createFileDescriptor(lua_State* L, std::string name) {
        if (lua_gettop(L) != 1) {
            lua_pushnil(L);
        } else {
            if (!lua_isstring(L, -1)) {
                lua_pushnil(L);
                return 1;
            }

            std::string fileName = lua_tostring(L, -1);

            fs::path sourceFile(Module::s_currentlyExecutingFile.parent_path());
            sourceFile.append(fileName);

            if (!isAllowedPath(Module::s_currentlyExecutingFile.parent_path(), sourceFile, true)) {
                lua_pushnil(L);
                return 1;
            }

            lua_newtable(L);
            lua_pushstring(L, "type");
            lua_pushstring(L, name.c_str());
            lua_settable(L, -3);
            lua_pushstring(L, "path");
            lua_pushstring(L, sourceFile.c_str());
            lua_settable(L, -3);
        }
        return 1;
    }
}