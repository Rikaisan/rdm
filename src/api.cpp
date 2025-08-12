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

            fs::path fileToRead(Module::getCurrentlyExecutingFile().parent_path());
            fileToRead.append(fileName);

            if (!isAllowedPath(Module::getCurrentlyExecutingFile().parent_path(), fileToRead, true)) {
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

            fs::path fileToExec(Module::getCurrentlyExecutingFile().parent_path());
            fileToExec.append(fileName);

            std::string name = Module::getNameFromPath(Module::getCurrentlyExecutingFile());
            if (!isAllowedPath(Module::getCurrentlyExecutingFile().parent_path(), fileToExec, true)) {
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

            fs::path fileToExec(Module::getCurrentlyExecutingFile().parent_path());
            fileToExec.append(fileName);

            std::string name = Module::getNameFromPath(Module::getCurrentlyExecutingFile());
            if (!isAllowedPath(Module::getCurrentlyExecutingFile().parent_path(), fileToExec, true)) {
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

    int lapi_IsSet(lua_State* L) {
        int argc = lua_gettop(L);
        if (argc != 1 || !lua_isstring(L, -1)) {
            lua_pushboolean(L, 0);
        } else {
            std::string item = lua_tostring(L, -1);
            lua_pushboolean(L, ModuleManager::isFlagSet(item) || ModuleManager::shouldProcessModule(item));
        }
        return 1;
    }

    int lapi_IsPreview(lua_State* L) {
        lua_pushboolean(L, ModuleManager::isFlagSet("preview"));
        return 1;
    }

    int lapi_stringExec(lua_State* L) {
        if (lua_gettop(L) != 1 || !lua_isstring(L, -1)) {
            lua_pushnil(L);
        } else {
            std::string content = lua_tostring(L, -1);

            lua_newtable(L);
            lua_pushstring(L, "type");
            lua_pushstring(L, "string");
            lua_settable(L, -3);
            lua_pushstring(L, "content");
            lua_pushstring(L, content.c_str());
            lua_settable(L, -3);
            lua_pushstring(L, "exec");
            lua_pushboolean(L, true);
            lua_settable(L, -3);
        }
        return 1;
    }

    int lapi_descriptorExec(lua_State* L) {
        if (lua_gettop(L) == 2 && lua_istable(L, -2) && lua_isstring(L, -1)) {
            lua_pushstring(L, "exec");
            lua_pushvalue(L, -2);
            lua_settable(L, -4);
            lua_pop(L, 1);
        } else if (lua_gettop(L) == 1 && lua_istable(L, -1)) {
            lua_pushstring(L, "exec");
            lua_pushboolean(L, true);
            lua_settable(L, -3);
        } else {
            const std::string name = Module::getNameFromPath(Module::getCurrentlyExecutingFile());
            if (lua_istable(L, -1)) {
                if (lua_getfield(L, -1, "path") == LUA_TSTRING) {
                    LOG_CUSTOM_ERR(name, "Invalid exec arguments for descriptor: " << lua_tostring(L, -1));
                } else {
                    LOG_CUSTOM_ERR(name, "Found invalid descriptor while trying to create exec rules");
                }
            } else {
                LOG_CUSTOM_ERR(name, "Invalid first argument, make sure to call exec as: descriptor:exec() or descriptor:exec(pattern)");
            }
            lua_pushnil(L);
        }
        return 1;
    }

    int createFileDescriptor(lua_State* L, std::string name) {
        if (lua_gettop(L) != 1 || !lua_isstring(L, -1)) {
            lua_pushnil(L);
        } else {
            std::string fileName = lua_tostring(L, -1);

            fs::path sourceFile(Module::getCurrentlyExecutingFile().parent_path());
            sourceFile.append(fileName);

            if (!isAllowedPath(Module::getCurrentlyExecutingFile().parent_path(), sourceFile, true)) {
                LOG_CUSTOM_ERR(Module::getNameFromPath(Module::getCurrentlyExecutingFile()), "Invalid path, not a module subdirectory or it doesn't exist.");
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

            // Modify table (FileDescriptor) metatable to allow execs
            if(luaL_newmetatable(L, "file_descriptor")) {
                lua_newtable(L);
                lua_pushcfunction(L, lapi_descriptorExec);
                lua_setfield(L, -2, "exec");
                lua_setfield(L, -2, "__index");
            }
            lua_setmetatable(L, -2);
        }
        return 1;
    }
}