cmake_minimum_required(VERSION 3.20)

project(rtems-init ASM C CXX)

option(BUILD_TESTS "Enable to build the tests" ON)
option(HOST_BUILD "Enable to build for host system" OFF)
option(BUILD_APPS "Build standalone applications for testing" ON)

set(ENABLE_TEXT_SEGMENT "default" CACHE STRING "Enable the cexpsh text segment. Can be 'default', 'yes' or 'no'" FORCE)
set(WITH_CEXP_LOADER "pmbfd" CACHE STRING "The Cexpsh loader to use. Can be 'pmbfd', 'rtl' or 'none'" FORCE)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(SRCDIR "${CMAKE_CURRENT_LIST_DIR}")

if (RTEMS_INSTALL)
    set(SHARED_PREFIX OFF)
    set(CMAKE_INSTALL_PREFIX "${RTEMS_TOP}/target/rtems/${RTEMS_ARCH}-rtems${RTEMS_TOOL_VERSION}/${RTEMS_BSP}")
endif()

find_package(PkgConfig)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/${RTEMS_ARCH}-${RTEMS_BSP}.cmake" OPTIONAL)

# Init rtems cmake integration
include(tools/cmake/rtems.cmake)
rtems_cmake_init()

rtems_check_include("rtems.h" HAVE_RTEMS_H)
rtems_check_include("sys/mman.h" HAVE_SYS_MMAN_H)
rtems_check_include("strings.h" HAVE_STRINGS_H)
rtems_check_include("sys/select.h" HAVE_SYS_SELECT_H)
rtems_check_include("sys/termios.h" HAVE_SYS_TERMIOS_H)
rtems_check_include("termios.h" HAVE_TERMIOS_H)
rtems_check_include("ncurses/term.h" HAVE_NCURSES_TERM_H)
rtems_check_include("ncurses/curses.h" HAVE_NCURSES_CURSES_H)
rtems_check_include("sys/features.h" HAVE_SYS_FEATURES_H)
rtems_check_include("link.h" HAVE_LINK_H)
rtems_check_include("pthread.h" HAVE_PTHREADS)
rtems_check_include("rtems/rtems/cache.h" HAVE_RTEMS_CACHE_H)
rtems_check_lib("debugger" "rtems_debugger_start" HAVE_DEBUGGER)
rtems_check_include("rtems/bsd/bsd.h" HAVE_BSD_NETWORKING)
rtems_check_include("rtems/bsdnet/servers.h" HAVE_LEGACY_NETWORKING)
rtems_check_lib("rtemsbsp" "BSP_pci_configuration" HAVE_PCI)
set(USE_BESTLINE 1)

configure_file(config.h.in config.h)

add_definitions(-DHAVE_CONFIG_H)
include_directories("${CMAKE_BINARY_DIR}")

# Force disable cexpsh if unsupported by this target
set(CEXP_ARCHES "arm" "powerpc" "i386" "m68k")
if (NOT RTEMS_ARCH IN_LIST CEXP_ARCHES)
    set(ENABLE_CEXP OFF CACHE STRING "" FORCE)
endif()

if (ENABLE_CEXP)
    message(STATUS "Cexpsh is ENABLED")
else()
    message(STATUS "Cexpsh is DISABLED")
endif()

if (ENABLE_LUA)
    message(STATUS "Lua is ENABLED")
else()
    message(STATUS "Lua is DISABLED")
endif()

###################################################
# rtems-init
###################################################
set(RTEMS_INIT_SRCS
    src/init.c
    src/rtems-config.c
    src/shell.c
    src/shell-tools.c
    src/nvram.c
    src/hack.c
    src/net.c
    src/cexp-text-region.c

    src/common/getopt_s.c
    src/common/util.c

    # Tests
    src/test/tests.c
    src/test/nfs-perf.c

)

if (ENABLE_LUA)
    list(
        APPEND RTEMS_INIT_SRCS

        # Lua
        modules/lua/onelua.c
        modules/lua/lua.c
        src/rtems-lua.c
    )
endif()
    
if ("${RTEMS_NETWORKING_STACK}" STREQUAL "LEGACY")
    list(APPEND RTEMS_INIT_SRCS 
        src/net-legacy.c
    )
elseif("${RTEMS_NETWORKING_STACK}" STREQUAL "BSD")
    list(APPEND RTEMS_INIT_SRCS
        src/net-bsd.c
    )
endif()

if (ENABLE_CEXP)
    rtems_add_executable(
        TARGET rtems-init
        CEXPSH  # Enable Cexpsh symbol table generation
        SOURCES ${RTEMS_INIT_SRCS}
    )
else()
    rtems_add_executable(
        TARGET rtems-init
        RTEMS_SYMS  # RTEMS RTL symbol table
        SOURCES ${RTEMS_INIT_SRCS}
    )
endif()

# Add a u-boot image for targets that generally need it.
if ("${RTEMS_ARCH}" STREQUAL "aarch64")
    rtems_add_uboot_image(rtems-init 0x10000)
endif()

rtems_include_libs(
    TARGET rtems-init
    LIBS c stdc++ m
    LIBDIRS "${CMAKE_BINARY_DIR}"
)

target_include_directories(
    rtems-init PUBLIC src
)

get_target_property(CURRENT_LINK_FLAGS rtems-init LINK_FLAGS)

# Force linker to emit symbols
set_target_properties(
    rtems-init rtems-init-exe PROPERTIES 
        #LINKER_LANGUAGE CXX
        LINK_FLAGS "-Wl,-T${CMAKE_CURRENT_LIST_DIR}/src/linker.ld -Wl,--no-undefined"
)

rtems_add_rootfs(
    rtems-init "${CMAKE_CURRENT_LIST_DIR}/rootfs" "TAR"
)

if ("${RTEMS_BSP}" STREQUAL "mvme3100")
    #rtems_add_dts(rtems-init src/fdt/mvme3100.dts)
endif()

if (HAVE_DEBUGGER)
    target_link_libraries(
        rtems-init PRIVATE debugger
    )
    target_compile_definitions(
        rtems-init PRIVATE HAVE_DEBUGGER
    )
endif()

if (RTEMS_BSD_STACK)
    set(NETWORK_LIB "bsd")
elseif (RTEMS_LEGACY_STACK)
    set(NETWORK_LIB "networking")
endif()

if (ENABLE_CEXP)
    target_link_libraries(
        rtems-init PRIVATE

        cexp
    )
endif()

target_link_libraries(
    rtems-init PRIVATE

    ntp
    ttcp
    telnetd
    tftpfs
    ${NETWORK_LIB}
    rtemscxx
    rtemscpu
    rtemsbsp
    stdc++
    c
    m
)

if (ENABLE_LUA)
    target_compile_definitions(rtems-init PRIVATE HAVE_LUA=1 LUA_USE_POSIX=1 MAKE_LIB=1)
endif()

if (ENABLE_CEXP)
    target_compile_definitions(rtems-init PRIVATE HAVE_CEXP=1)
endif()

target_include_directories(
    rtems-init PRIVATE
    
    modules/lua
)

###################################################
# Cexpsh
###################################################
if (ENABLE_CEXP)
    include(cexp.cmake)
endif()

###################################################
# libbspExt
###################################################
set(BSPEXT_SRCS
    modules/bspExt/isrWrap.c
    modules/bspExt/memProbe.c
    modules/bspExt/dabrBpnt.c
    modules/bspExt/bspExt.c
)

if ("${RTEMS_ARCH}" STREQUAL "powerpc")
    rtems_add_object(
        bspExt rtems-init

        ${BSPEXT_SRCS}
    )

    install(
        FILES "modules/bspExt/bspExt.h"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/bsp"
    )
endif()

###################################################
# miscUtils
###################################################
#set(MISCUTILS_SRCS
#    modules/miscUtils/mem-tools.c
#)
#
#rtems_add_object(
#    miscUtils rtems-init
#
#    ${MISCUTILS_SRCS}
#)
#
#target_include_directories(
#    miscUtils PRIVATE modules/cexp
#)
#
#target_compile_definitions(
#    miscUtils PRIVATE HAVE_CEXP
#)

###################################################
# dynamicTest
###################################################
rtems_add_object(
    dynamicTest rtems-init
    
    modules/test/loadable-test.c
)

###################################################
# Host tests
###################################################
if (BUILD_TESTS AND HOST_BUILD)
    add_executable(misc_test src/test/native-test.c)
endif()

###################################################
# Standalone apps
###################################################
if (BUILD_APPS)
    add_subdirectory(src/apps)
endif()
