#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "api.h"

int main(int argc, char ** argv) {

  lua_State *L = luaL_newstate();
  luaL_openlibs(L);

  register_cadence_functions(L);

  // Work with lua API
  luaL_dofile(L, "test.lua");

  lua_close(L);
  return 0;
}
