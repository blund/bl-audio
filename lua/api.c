#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../src/context.h"

// Lua wrapper for cadence_setup
int lua_cadence_setup(lua_State* L) {
  int sample_rate = luaL_checkinteger(L, 1); // Get the sample_rate argument from Lua

  // Call the original C function
  cadence_ctx* ctx = cadence_setup(sample_rate);

  // Push the context pointer as light userdata to Lua
  lua_pushlightuserdata(L, ctx);
  return 1; // Return 1 value (the pointer)
}

// Expose new_sine to Lua
int lua_new_sine(lua_State* L) {
  cadence_ctx* ctx = (cadence_ctx*)lua_touserdata(L, 1); // Get the context
  sine_t* sine = new_sine(ctx); // Call C function to create a new sine object

  lua_pushlightuserdata(L, sine); // Push the pointer as light userdata
  return 1; // Return the pointer
}

int lua_set_sine_freq(lua_State* L) {
  sine_t* sine = (sine_t*)lua_touserdata(L, 1); // Get the sine pointer
  float freq = luaL_checknumber(L, 2); // Get the context
  sine->freq = freq;
  return 0;
}

// Expose gen_sine to Lua
int lua_gen_sine(lua_State* L) {
  cadence_ctx* ctx = (cadence_ctx*)lua_touserdata(L, 1); // Get the context
  sine_t* sine = (sine_t*)lua_touserdata(L, 2); // Get the sine pointer

  if (!sine) return luaL_error(L, "Invalid sine pointer");

  float sample = gen_sine(ctx, sine); // Call C function
  lua_pushnumber(L, sample); // Push the result
  return 1; // Return the sample
}

// Register functions in Lua
void register_cadence_functions(lua_State* L) {
  lua_register(L, "cadence_setup", lua_cadence_setup);
  lua_register(L, "new_sine", lua_new_sine);
  lua_register(L, "set_sine_freq", lua_set_sine_freq);
  lua_register(L, "gen_sine", lua_gen_sine);
}
