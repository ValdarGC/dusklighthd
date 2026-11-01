#include "../lua_helpers.hpp"
#include "../runtime.hpp"
#include "services.hpp"

#include "mods/svc/tphd.h"

#include <string>

namespace luau_runtime::services {
namespace {

// TPHD mode helpers that make sense from a script (no game pointers): mode check, Wii U content
// folder, Wii U archive files, the Revo stretch values and reloading tphd_ texture packs.

int tphd_is_active(lua_State* state) {
    Vm& vm = vm_from_upvalue(state);
    lua_pushboolean(state, svc_tphd != nullptr && svc_tphd->is_active(vm.subject));
    return 1;
}

int tphd_content_path(lua_State* state) {
    Vm& vm = vm_from_upvalue(state);
    size_t size = 0;
    if (svc_tphd->get_content_path(vm.subject, nullptr, 0, &size) != MOD_OK || size == 0) {
        lua_pushnil(state);
        return 1;
    }
    std::string path(size, '\0');
    check_result(state, svc_tphd->get_content_path(vm.subject, path.data(), path.size(), &size),
        "tphd.content_path");
    path.resize(size - 1);
    lua_pushlstring(state, path.data(), path.size());
    return 1;
}

int tphd_read_arc_file(lua_State* state) {
    Vm& vm = vm_from_upvalue(state);
    const char* resPath = luaL_checkstring(state, 1);
    const char* filePath = luaL_checkstring(state, 2);
    size_t size = 0;
    if (svc_tphd->read_arc_file(vm.subject, resPath, filePath, nullptr, 0, &size) != MOD_OK) {
        lua_pushnil(state);
        return 1;
    }
    std::string data(size, '\0');
    check_result(state,
        svc_tphd->read_arc_file(vm.subject, resPath, filePath, data.data(), data.size(), &size),
        "tphd.read_arc_file");
    lua_pushlstring(state, data.data(), data.size());
    return 1;
}

int tphd_tv_stretch(lua_State* state) {
    Vm& vm = vm_from_upvalue(state);
    lua_pushnumber(state, svc_tphd->get_tv_stretch(vm.subject));
    return 1;
}

int tphd_n43_squeeze(lua_State* state) {
    Vm& vm = vm_from_upvalue(state);
    lua_pushnumber(state, svc_tphd->get_n43_squeeze(vm.subject));
    return 1;
}

int tphd_reload_texture_packs(lua_State* state) {
    Vm& vm = vm_from_upvalue(state);
    check_result(state, svc_tphd->reload_texture_packs(vm.subject), "tphd.reload_texture_packs");
    return 0;
}

}  // namespace

int open_tphd(lua_State* state) {
    Vm& vm = vm_from_upvalue(state);
    if (svc_tphd == nullptr) {
        service_unavailable(state, "TphdService");
    }
    lua_newtable(state);
    set_function(state, vm, "is_active", tphd_is_active);
    set_function(state, vm, "content_path", tphd_content_path);
    set_function(state, vm, "read_arc_file", tphd_read_arc_file);
    set_function(state, vm, "tv_stretch", tphd_tv_stretch);
    set_function(state, vm, "n43_squeeze", tphd_n43_squeeze);
    set_function(state, vm, "reload_texture_packs", tphd_reload_texture_packs);
    lua_setreadonly(state, -1, true);
    return 1;
}

}  // namespace luau_runtime::services
