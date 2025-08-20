cmake_minimum_required(VERSION 3.20)

project(rtems-init ASM C CXX)

option(BUILD_TESTS "Enable to build the tests" ON)
option(HOST_BUILD "Enable to build for host system" OFF)
set(ENABLE_TEXT_SEGMENT "default" CACHE STRING "Enable the cexpsh text segment. Can be 'default', 'yes' or 'no'" FORCE)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(SRCDIR "${CMAKE_CURRENT_LIST_DIR}")

if (RTEMS_INSTALL)
    set(SHARED_PREFIX OFF)
    set(CMAKE_INSTALL_PREFIX "${RTEMS_TOP}/target/rtems/${RTEMS_ARCH}-rtems${RTEMS_TOOL_VERSION}/${RTEMS_BSP}")
endif()

find_package(PkgConfig)

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
set(USE_BESTLINE 1)

configure_file(config.h.in config.h)

add_definitions(-DHAVE_CONFIG_H)
include_directories("${CMAKE_BINARY_DIR}")

###################################################
# rtems-init
###################################################
set(RTEMS_INIT_SRCS
    src/init.c
    src/rtems-config.c
    src/util.c
    src/shell.c
    src/getopt_s.c
    src/nvram.c
    src/hack.c
    src/net.c
    src/rtems-lua.c
    src/cexp-text-region.c

    # Tests
    src/test/irq-tst.c
    src/test/nfs-perf.c

    # Lua
    lua/onelua.c
    lua/lua.c
)
    
if ("${RTEMS_NETWORKING_STACK}" STREQUAL "LEGACY")
    list(APPEND RTEMS_INIT_SRCS 
        src/net-legacy.c
    )
elseif("${RTEMS_NETWORKING_STACK}" STREQUAL "BSD")
    list(APPEND RTEMS_INIT_SRCS
        src/net-bsd.c
    )
endif()

set (USE_CEXP 1)
rtems_add_executable(
    rtems-init

    ${RTEMS_INIT_SRCS}
)

rtems_include_libs(
    TARGET rtems-init
    LIBS stdc++ c m
    LIBDIRS "${CMAKE_BINARY_DIR}"
)

get_target_property(CURRENT_LINK_FLAGS rtems-init LINK_FLAGS)

# Force linker to emit symbols
set_target_properties(
    rtems-init rtems-init-exe PROPERTIES 
        #LINKER_LANGUAGE CXX
        LINK_FLAGS "-Wl,-u __symbolRefDummy -Wl,--no-undefined"
)

rtems_add_rootfs(
    rtems-init "${CMAKE_CURRENT_LIST_DIR}/rootfs" "TAR"
)

if ("${RTEMS_BSP}" STREQUAL "mvme3100")
    #rtems_add_dts(rtems-init src/fdt/mvme3100.dts)
endif()
    
target_link_libraries(
    rtems-init PRIVATE
    
    cexp
    ntp
    telnetd
    tftpfs
    bsd
    debugger
    rtemscxx
    rtemscpu
    rtemsbsp
    stdc++
    c
    m
)

target_compile_definitions(
    rtems-init PRIVATE
    
    MAKE_LIB=1
    LUA_USE_POSIX=1
    HAVE_LUA=1
    HAVE_CEXP=1
)

target_include_directories(
    rtems-init PRIVATE
    
    lua
)

###################################################
# regexp (submoduled in cexp)
###################################################
set(REGEXP_SRCS
    cexp/regexp/regexp.c
    cexp/regexp/regerror.c
    cexp/regexp/regsub.c
)

add_library(
    spencer_regexp STATIC ${REGEXP_SRCS}
)

install(
    TARGETS spencer_regexp
)

###################################################
# pmelf (submoduled in cexp)
###################################################
set(PMELF_SRCS
    "cexp/pmbfd/symname.c"
    "cexp/pmbfd/secname.c"
    "cexp/pmbfd/putdat.c"
    "cexp/pmbfd/dmpgrps.c"
    "cexp/pmbfd/strm.c"
    "cexp/pmbfd/fstrm.c"
    "cexp/pmbfd/mstrm.c"
    "cexp/pmbfd/dmpsym.c"
    "cexp/pmbfd/dmpsymtab.c"
    "cexp/pmbfd/dmpshdr.c"
    "cexp/pmbfd/dmpshtab.c"
    "cexp/pmbfd/dmpehdr.c"
    "cexp/pmbfd/dmprels.c"
    "cexp/pmbfd/dmpphdr.c"
    "cexp/pmbfd/symtab.c"
    "cexp/pmbfd/findsymhdrs.c"
    "cexp/pmbfd/shtab.c"
    "cexp/pmbfd/getgrp.c"
    "cexp/pmbfd/getrel.c"
    "cexp/pmbfd/getscn.c"
    "cexp/pmbfd/getsym.c"
    "cexp/pmbfd/putsym.c"
    "cexp/pmbfd/getshdr.c"
    "cexp/pmbfd/getphdr.c"
    "cexp/pmbfd/putshdr.c"
    "cexp/pmbfd/getehdr.c"
    "cexp/pmbfd/putehdr.c"
    "cexp/pmbfd/attpbfasmatch.c"
    "cexp/pmbfd/attpbfasdestroy.c"
    "cexp/pmbfd/attpbfasread.c"
    "cexp/pmbfd/attpbfasprint.c"
    "cexp/pmbfd/attpbfaprint.c"
    "cexp/pmbfd/attpbprinttag.c"
    "cexp/pmbfd/attset.c"
    "cexp/pmbfd/attprint.c"
    "cexp/pmbfd/attvendfind.c"
    "cexp/pmbfd/guleb128.c"
    "cexp/pmbfd/getwrd.c"
    "cexp/pmbfd/att-gnu-powerpc.c"
    "cexp/pmbfd/noelf64.c" # TODO: NO_64BIT
)

add_library(
    pmelf STATIC ${PMELF_SRCS}
)

target_include_directories(
    pmelf PRIVATE cexp/pmbfd
)

install(
    TARGETS pmelf
)

install(
    FILES "cexp/pmbfd/pmelf.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

###################################################
# pmbfd
###################################################
set(PMBFD_SRCS
    "cexp/pmbfd/bfd.c"
    "cexp/pmbfd/opcodesup.c"
    "cexp/pmbfd/bfd-reloc-${RTEMS_ARCH}.c"
)

add_library(
    pmbfd STATIC ${PMBFD_SRCS}
)

target_include_directories(
    pmbfd PRIVATE cexp/pmbfd
)

install(
    TARGETS pmbfd
)

###################################################
# libtecla
###################################################
set(TECLA_SRCS
    "cexp/libtecla/chrqueue.c"
    "cexp/libtecla/cplfile.c"
    "cexp/libtecla/cplmatch.c"
    "cexp/libtecla/direader.c"
    "cexp/libtecla/errmsg.c"
    "cexp/libtecla/expand.c"
    "cexp/libtecla/freelist.c"
    "cexp/libtecla/getline.c"
    "cexp/libtecla/hash.c"
    "cexp/libtecla/history.c"
    "cexp/libtecla/homedir.c"
    "cexp/libtecla/ioutil.c"
    "cexp/libtecla/keytab.c"
    "cexp/libtecla/pathutil.c"
    "cexp/libtecla/pcache.c"
    "cexp/libtecla/stringrp.c"
    "cexp/libtecla/strngmem.c"
    "cexp/libtecla/version.c"
)

add_library(
    tecla STATIC ${TECLA_SRCS}
)

add_library(
    tecla_r STATIC ${TECLA_SRCS}
)

target_include_directories(
    tecla PRIVATE cexp
)

target_include_directories(
    tecla_r PRIVATE cexp
)

target_compile_definitions(
    tecla_r PRIVATE "_POSIX_C_SOURCE=199506L" PREFER_REENTRANT=1
)

###################################################
# cexpsh
###################################################

# Determine the default text segment size
if ("${ENABLE_TEXT_SEGMENT}" STREQUAL "default")
    if ("${RTEMS_ARCH}" STREQUAL "powerpc" OR "${RTEMS_ARCH}" STREQUAL "arm")
        set(CEXP_TEXT_REGION_SIZE "0")
    endif()
elseif("${ENABLE_TEXT_SEGMENT}" STREQUAL "yes")
    set(CEXP_TEXT_REGION_SIZE "0x800000")
endif()

# Generate the gentab executable
add_custom_target(cexp_gentab
    BYPRODUCTS "${CMAKE_BINARY_DIR}/cexp/gentab"
    COMMAND mkdir -p "${CMAKE_BINARY_DIR}/cexp" &&
        cc -o "${CMAKE_BINARY_DIR}/cexp/gentab"
        "${SRCDIR}/cexp/gentab.c"
    DEPENDS "${SRCDIR}/cexp/gentab.c"
)

# Generate the jumptab
add_custom_target(cexp_jumptab
    BYPRODUCTS "${CMAKE_BINARY_DIR}/cexp/jumptab.c"
    COMMAND "${CMAKE_BINARY_DIR}/cexp/gentab"
        -o "${CMAKE_BINARY_DIR}/cexp/jumptab.c"
    DEPENDS cexp_gentab
)

# Generate grammar with bison
add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/cexp/cexp.tab.c"
           "${CMAKE_BINARY_DIR}/cexp/cexp.tab.h"
    COMMAND mkdir -p "${CMAKE_BINARY_DIR}/cexp" &&
        bison -v -d -p cexp
        -o "${CMAKE_BINARY_DIR}/cexp/cexp.tab.c"
        --header="${CMAKE_BINARY_DIR}/cexp/cexp.tab.h"
        "${SRCDIR}/cexp/cexp.y"
    DEPENDS "${SRCDIR}/cexp/cexp.y"
)

set(
    CEXP_DEFINES
    "PACKAGE_VERSION=\"6_dev\"" # TODO: FIXME: Implement me!
)

if (DEFINED CEXP_TEXT_REGION_SIZE)
    list(
        APPEND CEXP_DEFINES
        "CEXP_TEXT_REGION_SIZE=${CEXP_TEXT_REGION_SIZE}"
    )
endif()

set(
    CEXP_LIBS
    spencer_regexp
)

set(
    CEXP_INCLUDES
    "cexp"
    "cexp/regexp"
    "${CMAKE_BINARY_DIR}/cexp" # Needed for cexp.tab.h
)

set(
    CEXP_SRCS
    "cexp/cexp.c"
    "cexp/ctyps.c"
    "cexp/cexpsyms.c"
    "cexp/vars.c"
    "cexp/rshload.c"
    "cexp/cexplock.c"
    "cexp/cexpmod.c"
    "cexp/cexpveneer.c"
    "cexp/getopt/mygetopt_r.c"
    "cexp/help.c"
    "cexp/cexpsegs.c"
    "cexp/cexpsegs-alloc.c"
    "cexp/wrap.c"
    
    # Generated sources
    "${CMAKE_BINARY_DIR}/cexp/cexp.tab.c"
)

# TODO: Make these actually work
set(USE_PMBFD 1)
set(ENABLE_LOADER 1)
set(USE_TECLA 1)
set(USE_BESTLINE 1)
set(USE_ELFSYMS 0)
set(USE_RTL 0)

# Not compatible with pmbfd/elf
if (USE_RTL)
    set(USE_PMBFD 0)
endif()

# Tecla support
if (USE_BESTLINE)
    list(
        APPEND CEXP_SRCS
        cexp/bestline/bestline.c
        cexp/bestline/bestline.h
    )
    list(
        APPEND CEXP_DEFINES
        "HAVE_BESTLINE=1"
    )
elseif (USE_TECLA)
    list(
        APPEND CEXP_SRCS
        cexp/teclastuff.c
    )
    list(
        APPEND CEXP_INCLUDES
        cexp/libtecla
    )
    list(
        APPEND CEXP_LIBS
        tecla
    )
endif()

# ELF loader support
if (ENABLE_LOADER)
    list(
        APPEND CEXP_SRCS 
        #cexp/bfdstuff.c
    )
    list(
        APPEND CEXP_DEFINES
        "USELOADER=1"
        "USE_LOADER=1"
    )
elseif (USE_ELFSYMS)
    list(
        APPEND CEXP_SRCS
        cexp/elfsyms.c
        cexp/elfdlmap.c
    )
else()
    list(
        APPEND CEXP_SRCS
        cexp/noloader.c
    )
endif()

# Permissive BFD support
if (USE_PMBFD)
    list(
        APPEND CEXP_LIBS
        pmbfd
        pmelf
    )
    list(
        APPEND CEXP_DEFINES
        "USEPMBFD=1"
        "USE_PMBFD=1"
    )
    list(
        APPEND CEXP_INCLUDES
        "cexp/pmbfd"
    )
    list(
        APPEND CEXP_SRCS
        "cexp/bfdstuff.c"
    )
endif()

# RTEMS RTL support
if (USE_RTL)
    list(
        APPEND CEXP_DEFINES
        "USE_RTL=1"
    )
    list(
        APPEND CEXP_SRCS
        "cexp/cexprtl.c"
    )
endif()

add_library(
    cexp STATIC ${CEXP_SRCS}
)

add_dependencies(
    cexp cexp_jumptab
)

target_include_directories(
    cexp PRIVATE ${CEXP_INCLUDES}
)

target_link_libraries(
    cexp PUBLIC ${CEXP_LIBS}
)

target_compile_definitions(
    cexp PRIVATE ${CEXP_DEFINES}
)

target_include_directories(
    cexp PUBLIC cexp
)

install(
    TARGETS cexp
)

install(
    FILES
        "cexp/cexp.h"
        "cexp/cexpHelp.h"
        "cexp/ctyps.h"
        "cexp/cexpsyms.h"
    DESTINATION
        "${CMAKE_INSTALL_INCLUDEDIR}"
)

###################################################
# libbspExt
###################################################
set(BSPEXT_SRCS
    bspExt/isrWrap.c
    bspExt/memProbe.c
    bspExt/dabrBpnt.c
    bspExt/bspExt.c
)

if ("${RTEMS_ARCH}" STREQUAL "powerpc")
    rtems_add_object(
        bspExt rtems-init
        
        ${BSPEXT_SRCS}
    )

    install(
        FILES "bspExt/bspExt.h"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/bsp"
    )
endif()

###################################################
# miscUtils
###################################################
set(MISCUTILS_SRCS
    miscUtils/shell-tools.c
    miscUtils/mem-tools.c
)

rtems_add_object(
    miscUtils rtems-init

    ${MISCUTILS_SRCS}
)

target_include_directories(
    miscUtils PRIVATE "cexp"
)

target_compile_definitions(
    miscUtils PRIVATE HAVE_CEXP
)

###################################################
# Host tests
###################################################
if (BUILD_TESTS AND HOST_BUILD)
    add_executable(misc_test src/test/test.c)
endif()
