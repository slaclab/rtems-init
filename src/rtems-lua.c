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
#include "lrtemslib.h"

#include <rtems.h>
#include <rtems/error.h>
#include <rtems/rtems/sem.h>

#include "rtems-init.h"
#include "common/util.h"

#include "lua.h"

static luart_alloc_state_t s_luaAllocState;

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

static int
rtems_lua_atpanic(lua_State *L)
{
  rtems_fatal(RTEMS_FATAL_SOURCE_APPLICATION, 0x122F);
  return 0;
}

static lua_State*
new_state()
{
  lua_State* L = luaL_newstate();

  /* setup allocator if we haven't already */
  if (!s_luaAllocState.heapSize) {
    luaRT_init_alloc_state(&s_luaAllocState, 1<<17); /* 128k for now */
  }
  lua_setallocf(L, luaRT_alloc, &s_luaAllocState);
  lua_atpanic(L, rtems_lua_atpanic);
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

int
shell_luainfo_main(int argc, char** argv)
{
  size_t used = luaRT_heap_usage(&s_luaAllocState);
  size_t free = luaRT_heap_free(&s_luaAllocState);
  printf("Lua Heap: %2.f KiB used, %.2f KiB free, %.2f KiB total\n",
    used / 1024.f, free / 1024.f, s_luaAllocState.heapSize / 1024.f);
  return 0;
}

struct luad_daemon {
  lua_State* L;
  rtems_name name;
  rtems_id id;
  char path[PATH_MAX];

  struct luad_daemon* next;
};

static rtems_mutex daemon_guard = RTEMS_MUTEX_INITIALIZER("lDLK");
struct luad_daemon* s_daemons;

static void
luad_usage()
{
  printf(
    "USAGE: luad start|stop|info ...\n"
    "Subcommands:"
    "  luad start taskname /path/to/script.lua [prio] [modes]\n"
    "    taskname must be at most 4 chars long.\n"
    "    modes is a string in the format [PpAaTt] where:\n"
    "      P = enable preempt, p = disable preempt\n"
    "      A = enable ASR, a = disable ASR\n"
    "      T = enable timeslice, t = disable timeslice\n"
    "\n"
    "  luad stop taskname\n"
    "    sends the stop signal to the specified task\n"
    "\n"
    "  luad info\n"
    "    lists all luad tasks\n"
    "\n"
  );
}

static int
luad_list(int argc, char** argv)
{
  printf("Lua daemon task list:\n");
  rtems_mutex_lock(&daemon_guard);
  for (struct luad_daemon* d = s_daemons; d; d = d->next) {
    char buf[5] = {0};
    rtems_name_to_characters(d->name, &buf[0], &buf[1], &buf[2], &buf[3]);
    printf("%s\n", buf);
  }
  rtems_mutex_unlock(&daemon_guard);
  return 0;
}

/**
 * \brief Sends the "stop" signal to the specified Lua task.
 */
static int
luad_kill(int argc, char** argv)
{
  rtems_error_code_t r;
  int ret = -1;

  if (argc < 3) {
    luad_usage();
    return -1;
  }

  /* build a name */
  char buf[5] = {0};
  strncpy(buf, argv[2], sizeof(buf));
  rtems_name name = rtems_build_name(buf[0], buf[1], buf[2], buf[3]);

  rtems_mutex_lock(&daemon_guard);
  for (struct luad_daemon* d = s_daemons; d; d = d->next) {
    if (d->name == name) {
      r = rtems_signal_send(d->name, RTEMS_SIGNAL_31);
      if (r != RTEMS_SUCCESSFUL) {
        rtems_error(r, "luad_kill: rtems_signal_send");
        ret = -1;
      } else {
        ret = 0;
      }
      break;
    }
  }
  rtems_mutex_unlock(&daemon_guard);

  if (ret < 0) {
    printf("No such task '%s'\n", buf);
  }
  return ret;
}

static void
luad_start_task(uintptr_t arg)
{
  struct luad_daemon* d = (struct luad_daemon*)arg;

  rtems_error_code_t r = rtems_signal_catch(0, RTEMS_NO_ASR);
  if (r != RTEMS_SUCCESSFUL) {
    rtems_error(r, "luad_start_task: rtems_signal_catch");
    goto error;
  }

  d->L = new_state();
  if (!d->L) {
    printf("Failed to create new lua state\n");
    goto error;
  }

  if (luaL_dofile(d->L, d->path) != 0) {
    printf("Failed to run '%s'\n", d->path);
    goto error;
  }

  /* start the daemon */
  /* TODO: ARGS! */
  lua_getglobal(d->L, "main");
  if (!lua_isfunction(d->L, -1)) {
    printf("Unable to find main\n");
    goto error;
  }

  /* add it to the task list (for bookkeeping) */
  rtems_mutex_lock(&daemon_guard);
  d->next = s_daemons;
  s_daemons = d;
  rtems_mutex_unlock(&daemon_guard);

  lua_call(d->L, 0, 1);
  if (lua_tointeger(d->L, 0) != 0) {
    printf("Daemon returned error\n");
  }

  /* remove from task list */
  rtems_mutex_lock(&daemon_guard);
  for (struct luad_daemon *md = s_daemons, *pd = NULL; md; pd = md, md = md->next) {
    if (md->name != d->name)
      continue;

    /* unlink it */
    if (pd)
      pd->next = md->next;
    break;
  }
  rtems_mutex_unlock(&daemon_guard);

  lua_close(d->L);

error:
  free(d);
  rtems_task_delete(RTEMS_SELF);
}

static const char* DEFAULT_MODESTR = "P";

/**
 * \brief Starts the Lua task in the background.
 */
static int
luad_start(int argc, char** argv)
{
  rtems_mode mode = 0;

  if (argc < 3) {
    luad_usage();
    return -1;
  }

  /* build mode flags list */
  const char* modestr = argc > 5 ? argv[5] : DEFAULT_MODESTR;
  for (const char* s = modestr; *s; s++) {
    switch (*s) {
    case 'T':
      mode |= RTEMS_TIMESLICE; break;
    case 't':
      mode &= ~RTEMS_TIMESLICE; break;
    case 'A':
      mode &= ~RTEMS_NO_ASR; break;
    case 'a':
      mode |= RTEMS_NO_ASR; break;
    case 'P':
      mode &= ~RTEMS_NO_PREEMPT; break;
    case 'p':
      mode |= RTEMS_NO_PREEMPT; break;
    default:
      printf("Invalid mode modifier '%c': must be TtAaPp\n", *s);
      return -1;
    }
  }

  /* build a task name */
  char tn[5];
  strncpy(tn, argv[2], 5);
  rtems_name name = rtems_build_name(tn[0], tn[1], tn[2], tn[3]);
  
  /* check that the file actually exists... */
  if (access(argv[3], 0777) != 0) {
    printf("Script '%s' does not exist or is not executable\n", argv[3]);
    return -1;
  }

  uint32_t prio = argc > 4 ? atoi(argv[4]) : 100;
  if (!prio) {
    printf("Invalid priority '%s'\n", argv[3]);
    luad_usage();
    return -1;
  }

  rtems_id ident;
  rtems_error_code_t r = rtems_task_create(
    name,
    prio,
    16384, /* stack size */
    mode,
    RTEMS_FLOATING_POINT,
    &ident
  );
  
  if (r != RTEMS_SUCCESSFUL) {
    rtems_error(r, "Unable to start Lua task");
    return -1;
  }
  
  /* add to the task list */
  struct luad_daemon* d = calloc(sizeof(struct luad_daemon), 1);
  d->id = ident;
  d->name = name;
  strncpySafe(d->path, argv[3], sizeof(d->path));

  r = rtems_task_start(ident, luad_start_task, (rtems_task_argument)d);
  if (r != RTEMS_SUCCESSFUL) {
    rtems_error(r, "rtems_task_start failed");
    free(d);
    rtems_task_delete(ident);
    return -1;
  }
  
  printf("Started daemon %s\n", tn);

  return 0;
}

int
shell_luad_main(int argc, char** argv)
{
  if (argc < 2) {
    luad_usage();
    return -1;
  }
  
  if (!strcmp(argv[1], "start")) {
    return luad_start(argc, argv);
  } else if (!strcmp(argv[1], "stop")) {
    return luad_kill(argc, argv);
  } else if (!strcmp(argv[1], "info")) {
    return luad_list(argc, argv);
  } else if (!strcmp(argv[1], "kill")) {
    return luad_kill(argc, argv);
  } else {
    printf("Invalid subcommand '%s'\n", argv[1]);
    luad_usage();
    return -1;
  }
}
