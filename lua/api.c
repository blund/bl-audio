#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../src/context.h"

int lua_cadence_setup(lua_State* L) {
  int sample_rate = luaL_checkinteger(L, 1);
  cadence_ctx* ctx = cadence_setup(sample_rate);

  lua_pushlightuserdata(L, ctx);
  return 1;
}

int lua_new_sine(lua_State* L) {
  cadence_ctx* ctx = (cadence_ctx*)lua_touserdata(L, 1);
  sine_t* sine = new_sine(ctx);

  lua_pushlightuserdata(L, sine);
  return 1;
}

int lua_set_sine_freq(lua_State* L) {
  sine_t* sine = (sine_t*)lua_touserdata(L, 1);
  float freq = luaL_checknumber(L, 2);
  sine->freq = freq;
  return 0;
}

int lua_gen_sine(lua_State* L) {
  cadence_ctx* ctx = (cadence_ctx*)lua_touserdata(L, 1);
  sine_t* sine = (sine_t*)lua_touserdata(L, 2);

  if (!sine) return luaL_error(L, "Invalid sine pointer");

  float sample = gen_sine(ctx, sine);
  lua_pushnumber(L, sample);
  return 1;
}

// Register functions in Lua
void register_cadence_functions(lua_State* L) {
  lua_register(L, "cadence_setup", lua_cadence_setup);
  lua_register(L, "new_sine", lua_new_sine);
  lua_register(L, "set_sine_freq", lua_set_sine_freq);
  lua_register(L, "gen_sine", lua_gen_sine);
}
