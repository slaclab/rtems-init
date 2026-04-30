# Shell & Scripting

rtems-init supports two system shells: Cexpsh and RTEMS shell.

Cexpsh, or C Expression Shell, is an open source shell that resembles the VxWorks system shell. It supports loading partially relocated
ELF objects via a built-in object loader, and can call arbitrary C functions.

The RTEMS shell (or RTSH), is the default built-in shell provided by RTEMS.

In addition to these shells, rtems-init also supports scripting with Lua, a lightweight interpreted scripting language.

If configured to do so (by default it is), rtems-init will display a "press any key to skip init" message on boot. This can be used to drop
into a rescue shell that uses the RTEMS shell. This is particularly handy if Cexpsh becomes useless due to loss of NFS mount or something similar.

## Cexpsh

A number of built-in functions that are callable through Cexpsh are built into the base image:
| Function | Description |
|---|---|
| `cp(const char* src, const char* dst)` | Copy a file |
| `cat(const char* file)` | Cat a file to stdout |
| `ls(const char* dir)` | List files in a directory |
| `pwd()` | Print current working directory |
| `dumpEnv()` | Dump the current environment |
| `workspaceUsage()` | Show workspace (memory) usage stats |
| `mallocStats()` | Show malloc stats |
| `nfsMount(const char* ip, const char* src, const char* mntpt)` | Compatibility with RTEMS 4's nfsMount |
| `sh()` | Launch an instance of the RTEMS shell |

## RTEMS Shell

The following commands are registered by rtems-init:
| Command | Description |
|---|---|
| `dlopen`      | Open an ELF object with the RTEMS run-time linker (RTL) |
| `dlsym`       | Find a symbol in an ELF object |
| `dlclose`     | Close an ELF object |
| `dlcall`      | Call a function in an ELF object |
| `rtl`         | RTEMS run-time linker (RTL) commands |
| `dbgstart`    | Start the remote debugger |
| `dbgstop`     | Stop the remote debugger |
| `nvramGet`    | Get a parameter from NVRAM (if supported by BSP) |
| `nvramShow`   | Show all NVRAM parameters (if supported by BSP) |
| `gevGet`      | Get a global environment variable (Motload only) |
| `gevShow`     | Show all global environment variables (Motload only) |
| `ntpd`        | Start the NTP daemon |
| `dumpenv`     | Dump all environment variables |
| `setuid`      | Sets the effective UID |
| `getuid`      | Gets the effective UID |
| `setgid`      | Sets the effective GID |
| `getaddrinfo` | Resolves an address, printing the resulting IP |
| `apropos`     | Fuzzy search for a term in the shell command list |
| `getifaddrs`  | Gets all IP addresses for all adapters on the system |
| `lspci`       | List all available PCI devices (similar to lspci on Linux) |
| `sh`          | Launch new instance of the RTEMS shell |
| `ttcp`        | ttcp utility for testing TCP: https://en.wikipedia.org/wiki/Ttcp |
| `lua`         | Launch an interactive Lua shell, or run a Lua script off of the file system |

## Lua Scripting

An interactive Lua interpreter can be accessed from the RTEMS shell using the `lua` command.
This can also be used to run Lua scripts off of the file system-- it's no different than the `lua` command on a Linux host.

Lua scripts located in `/bin` are automatically registered as RTEMS shell commands and can be easily run from anywhere.
For example, `/bin/rtems-version.lua` can be run as `rtems-version.lua` from the RTEMS shell.

These scripts are _not_ registered with Cexpsh in any way.