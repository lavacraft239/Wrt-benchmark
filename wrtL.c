// lua_wrt.c
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "wrt.h"  // Tu header de WRT

// Función Lua que llama a wrt_start o a la función que quieras
static int lua_wrt_start(lua_State *L) {
    const char *url = luaL_checkstring(L, 1);

    wrt_config_t config;
    wrt_init_config(&config);
    strncpy(config.url, url, sizeof(config.url));
    wrt_start(&config);

    return 0;  // número de valores devueltos a Lua
}

// Registra las funciones del módulo
int luaopen_wrt(lua_State *L) {
    static const struct luaL_Reg wrt_funcs[] = {
        {"start", lua_wrt_start},
        {NULL, NULL}
    };
    luaL_newlib(L, wrt_funcs);
    return 1;
}
