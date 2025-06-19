#include "api.hpp"
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include "utils.hpp"
#include "modules.hpp"

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

            fs::path fileToRead(Module::currentlyExecutingFile.parent_path());
            fileToRead.append(fileName);

            if (!isAllowedPath(Module::currentlyExecutingFile.parent_path(), fileToRead, true)) {
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
        if (lua_gettop(L) != 1) {
            lua_pushnil(L);
        } else {
            if (!lua_isstring(L, -1)) {
                lua_pushnil(L);
                return 1;
            }

            std::string fileName = lua_tostring(L, -1);

            fs::path sourceFile(Module::currentlyExecutingFile.parent_path());
            sourceFile.append(fileName);

            if (!isAllowedPath(Module::currentlyExecutingFile.parent_path(), sourceFile, true)) {
                lua_pushnil(L);
                return 1;
            }

            lua_newtable(L);
            lua_pushstring(L, "bytes");
            lua_pushstring(L, sourceFile.c_str());
            lua_settable(L, -3);
        }
        return 1;
    }

    int lapi_Spawn(lua_State* L) {
        if (lua_gettop(L) == 1) {
            if (!lua_isstring(L, -1)) return 0;

            std::string fileName = lua_tostring(L, -1);

            fs::path fileToExec(Module::currentlyExecutingFile.parent_path());
            fileToExec.append(fileName);

            if (!isAllowedPath(Module::currentlyExecutingFile.parent_path(), fileToExec, true)) return 0;

            std::system(fileToExec.c_str());
        }
        return 0;
    }

    int lapi_ForceSpawn(lua_State* L) {
        if (lua_gettop(L) == 1) {
            if (!lua_isstring(L, -1)) return 0;

            std::string fileName = lua_tostring(L, -1);

            fs::path fileToExec(Module::currentlyExecutingFile.parent_path());
            fileToExec.append(fileName);

            if (!isAllowedPath(Module::currentlyExecutingFile.parent_path(), fileToExec, true)) return 0;

            std::filesystem::permissions(fileToExec, std::filesystem::perms::owner_exec, std::filesystem::perm_options::add);
            std::system(fileToExec.c_str());
        }
        return 0;
    }

    int lapi_ModuleIsSet(lua_State* L) {
        int argc = lua_gettop(L);
        if (argc != 1 || !lua_isstring(L, -1)) {
            lua_pushboolean(L, 0);
        } else {
            std::string module = lua_tostring(L, -1);
            lua_pushboolean(L, ModuleManager::isModuleSet(module));
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
}