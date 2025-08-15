/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: RTEMS lua integration
 * ----------------------------------------------------------------------------
 * This file is part of 'rtems-init'. It is subject to the license terms in the
 * LICENSE.txt file found in the top-level directory of this distribution,
 * and at:
 *    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
 * No part of 'rtems-init', including this file, may be copied, modified,
 * propagated, or distributed except according to the terms contained in the
 * LICENSE.txt file.
 * ----------------------------------------------------------------------------
 **/
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "lua.h"

#include "rtems-init.h"
#include "util.h"

#include "lua.h"

extern int lua_main(lua_State* L);

static int
lua_rtems_kinfo(lua_State* L)
{
  const char* str = luaL_checkstring(L, 1);
  klog("%s", str);
  return 0;
}

static int
lua_rtems_kwarn(lua_State* L)
{
  const char* str = luaL_checkstring(L, 1);
  kwarn("%s", str);
  return 0;
}

static int
lua_rtems_err(lua_State* L)
{
  const char* str = luaL_checkstring(L, 1);
  kerror("%s", str);
  return 0;
}

struct luaL_Reg klog_lib[] = {
  {"info", lua_rtems_kinfo},
  {"warn", lua_rtems_kwarn},
  {"err", lua_rtems_err},
  {NULL, NULL}
};

static int
lua_klog_setup(lua_State* L)
{
  luaL_newlib(L, klog_lib);
  return 1;
}

static luaL_Reg extra_libs[] = {
  {"klog", lua_klog_setup},
  {NULL, NULL},
};

static lua_State*
new_state()
{
  lua_State* L = luaL_newstate();
  luaL_openlibs(L);
  luaL_newlib(L, extra_libs);
  return L;
}

int
lua_exec_script(const char* file)
{
  int r;

  char olddir[PATH_MAX];
  getcwd(olddir, sizeof(olddir));

  char newdir[PATH_MAX];
  strncpySafe(newdir, file, sizeof(newdir));
  strip_filename(newdir);

  if (chdir(newdir) < 0) {
    kwarn("chdir to %s failed: %s\n", newdir, strerror(errno));
    return -1;
  }

  lua_State* L = new_state();
  if (luaL_dofile(L, file) != LUA_OK) {
    kerror("Script error: %s\n", lua_tostring(L, -1));
    r = -1;
  }
  lua_close(L);

  chdir(olddir);
  return r;
}

/* NOTE: The below code is taken from lua.c */

static int report (lua_State *L, int status) {
  if (status != LUA_OK) {
    const char *msg = lua_tostring(L, -1);
    if (msg == NULL)
      msg = "(error message not a string)";
    fprintf(stderr, "%s\n", msg);
    lua_pop(L, 1);  /* remove message */
  }
  return status;
}

int
shell_lua_main(int argc, char** argv)
{
  int status, result;
  lua_State *L = new_state();  /* create state */
  if (L == NULL) {
    fprintf(stderr, "cannot create state: not enough memory");
    return -1;
  }
  lua_gc(L, LUA_GCSTOP);  /* stop GC while building state */
  lua_pushcfunction(L, &lua_main);  /* to call 'pmain' in protected mode */
  lua_pushinteger(L, argc);  /* 1st argument */
  lua_pushlightuserdata(L, argv); /* 2nd argument */
  status = lua_pcall(L, 2, 1, 0);  /* do the call */
  result = lua_toboolean(L, -1);  /* get result */
  report(L, status);
  lua_close(L);
  return (result && status == LUA_OK) ? 0 : -1;
}
