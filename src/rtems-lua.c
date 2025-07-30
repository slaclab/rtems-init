
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

static lua_State*
new_state()
{
  lua_State* L = luaL_newstate();
  luaL_openlibs(L);
  return L;
}

int
lua_exec_script(const char* file)
{
  lua_State* L = new_state();
  if (luaL_dofile(L, file) != LUA_OK) {
    fprintf(stderr, "Script error: %s\n", lua_tostring(L, -1));
    lua_close(L);
    return -1;
  }
  lua_close(L);
  return 0;
}