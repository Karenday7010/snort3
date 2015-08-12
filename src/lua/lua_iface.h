//--------------------------------------------------------------------------
// Copyright (C) 2015-2015 Cisco and/or its affiliates. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 as published
// by the Free Software Foundation.  You may not use, modify or distribute
// this program under any other version of the GNU General Public License.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//--------------------------------------------------------------------------
// lua_iface.h author Joel Cornett <jocornet@cisco.com>

#ifndef LUA_IFACE_H
#define LUA_IFACE_H

#include <luajit-2.0/lua.hpp>

#include "lua.h"
#include "lua_ref.h"
#include "lua_table.h"

namespace Lua
{
// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
template<typename T>
inline T** regurgitate(lua_State* L, const char* name, int arg)
{ return static_cast<T**>(const_cast<void*>(luaL_checkudata(L, arg, name))); }

static inline int register_methods(
    lua_State* L, const luaL_Reg* methods, const char* name = nullptr)
{
    luaL_register(L, name, methods);
    return lua_gettop(L);
}

static inline int register_metamethods(
    lua_State* L, const luaL_Reg* methods, const char* name)
{
    luaL_newmetatable(L, name);
    luaL_register(L, nullptr, methods);
    return lua_gettop(L);
}

static inline void register_with_closure(
    lua_State* L, const luaL_Reg* methods, int table, int cl)
{
    ManageStack ms(L, 2);

    for ( auto entry = methods; entry->func; entry++ )
    {
        lua_pushstring(L, entry->name);
        lua_pushvalue(L, cl);
        lua_pushcclosure(L, entry->func, 1);
        lua_rawset(L, table);
    }
}

static inline int new_lib(lua_State* L, const char* name)
{
    const luaL_Reg empty[] = { { nullptr, nullptr } };
    return register_methods(L, empty, name);
}

// -----------------------------------------------------------------------------
// TypeInterface
// -----------------------------------------------------------------------------
template<typename T>
struct TypeInterface
{
    using type = T;
    using AccessorCallback = void (*)(lua_State*, int, T&);

    const char* name;
    const luaL_Reg* methods;
    const luaL_Reg* metamethods;

    T** regurgitate(lua_State* L, int arg = 1) const
    { return ::Lua::regurgitate<T>(L, name, arg); }

    T& get(lua_State* L, int arg = 1) const
    { return **regurgitate(L, arg); }

    bool is(lua_State* L, int arg = 1) const
    {
        ManageStack ms(L, 1);

        if ( lua_type(L, arg) != LUA_TUSERDATA )
            return false;

        // Check the registry for metatable with name
        lua_getfield(L, LUA_REGISTRYINDEX, name);
        lua_getmetatable(L, arg);

        if ( !lua_rawequal(L, -1, -2) )
            return false;

        return true;
    }

    T** allocate(lua_State*) const;

    template<typename... Args>
    T& create(lua_State*, Args&&...) const;

    void destroy(lua_State*, T** = nullptr) const;

    int default_tostring(lua_State* L) const
    {
        lua_pushfstring(L, "%s@0x%p", this->name, &this->get(L));
        return 1;
    }

    int default_gc(lua_State* L) const
    {
        this->destroy(L);
        return 0;
    }

    int default_getter(lua_State* L, AccessorCallback acb) const
    {
        auto& self = this->get(L);
        lua_newtable(L);
        acb(L, lua_gettop(L), self);
        return 1;
    }

    int default_setter(lua_State* L, AccessorCallback acb) const
    {
        auto& self = this->get(L, 1);
        luaL_checktype(L, 2, LUA_TTABLE);
        acb(L, 2, self);
        return 0;
    }
};

template<typename T>
T** TypeInterface<T>::allocate(lua_State* L) const
{
    auto t = static_cast<T**>(lua_newuserdata(L, sizeof(T*)));

    luaL_getmetatable(L, name);
    lua_setmetatable(L, -2);

    return t;
}

template<typename T>
template<typename... Args>
T& TypeInterface<T>::create(lua_State* L, Args&&... args) const
{
    auto t = allocate(L);

    *t = new T(std::forward<Args>(args)...);

    return **t;
}

template<typename T>
void TypeInterface<T>::destroy(lua_State* L, T** t) const
{
    if ( !t )
        t = regurgitate(L, 1);

    if ( *t )
    {
        remove_refs(L, static_cast<void*>(*t));
        delete *t;
        t = nullptr;
    }
}

// -----------------------------------------------------------------------------
// InstanceInterface
// -----------------------------------------------------------------------------
template<typename T>
struct InstanceInterface
{
    using type = T;
    const char* name;
    const luaL_Reg* methods;

    T& get(lua_State*, int = 1) const;
};

template<typename T>
T& InstanceInterface<T>::get(lua_State* L, int up) const
{
    int idx = lua_upvalueindex(up);
    luaL_checktype(L, idx, LUA_TLIGHTUSERDATA);
    return *static_cast<T*>(const_cast<void*>(lua_topointer(L, idx)));
}

// -----------------------------------------------------------------------------
// Library
// -----------------------------------------------------------------------------

struct Library
{
    const char* name;
    const luaL_Reg* methods;
};

// -----------------------------------------------------------------------------
// Installers
// -----------------------------------------------------------------------------
template<typename T>
void install(lua_State* L, const struct TypeInterface<T>& iface)
{
    ManageStack ms(L, 4);

    int m = register_methods(L, iface.methods, iface.name);
    int mt = register_metamethods(L, iface.metamethods, iface.name);

    Table meta(L, mt);
    meta.raw_set_field_from_stack("__index", m);
    meta.raw_set_field_from_stack("__metatable", m);
}

template<typename T>
void install(lua_State* L, const struct InstanceInterface<T>& iface, T* instance)
{
    ManageStack ms(L, 2);

    int table = new_lib(L, iface.name);
    lua_pushlightuserdata(L, static_cast<void*>(instance));
    int inst = lua_gettop(L);
    register_with_closure(L, iface.methods, table, inst);
}

static inline void install(lua_State* L, const struct Library& lib)
{ register_methods(L, lib.methods, lib.name); }

}

#endif
