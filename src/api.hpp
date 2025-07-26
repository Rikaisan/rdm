#pragma once
#include <lua.hpp>
#include <string>

namespace rdm {
    int lapi_Read(lua_State* L);
    int lapi_FlagIsSet(lua_State* L);
    int lapi_ModuleIsSet(lua_State* L);
    int lapi_IsSet(lua_State* L);
    int lapi_IsPreview(lua_State* L);
    int lapi_ForceSpawn(lua_State* L);
    int lapi_Spawn(lua_State* L);
    int lapi_File(lua_State* L);
    int lapi_Directory(lua_State* L);

    int createFileDescriptor(lua_State* L, std::string name);
}