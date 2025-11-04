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
rtems_check_lib("debugger" "rtems_debugger_start" HAVE_DEBUGGER)
rtems_check_include("rtems/bsd/bsd.h" HAVE_BSD_NETWORKING)
rtems_check_include("rtems/bsdnet/servers.h" HAVE_LEGACY_NETWORKING)
rtems_check_lib("pci_initialize" "pci_read_config_word" HAVE_PCI)
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
    modules/lua/onelua.c
    modules/lua/lua.c
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
    LIBS c stdc++ m
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

if (HAVE_DEBUGGER)
    target_link_libraries(
        rtems-init PRIVATE debugger
    )
endif()

if (HAVE_BSD_NETWORKING)
    set(NETWORK_LIB "bsd")
elseif (HAVE_LEGACY_NETWORKING)
    set(NETWORK_LIB "networking")
endif()

target_link_libraries(
    rtems-init PRIVATE
    
    cexp
    ntp
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

target_compile_definitions(
    rtems-init PRIVATE
    
    MAKE_LIB=1
    LUA_USE_POSIX=1
    HAVE_LUA=1
    HAVE_CEXP=1
)

target_include_directories(
    rtems-init PRIVATE
    
    modules/lua
)

###################################################
# regexp (submoduled in cexp)
###################################################
set(REGEXP_SRCS
    modules/cexp/regexp/regexp.c
    modules/cexp/regexp/regerror.c
    modules/cexp/regexp/regsub.c
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
    "modules/cexp/pmbfd/symname.c"
    "modules/cexp/pmbfd/secname.c"
    "modules/cexp/pmbfd/putdat.c"
    "modules/cexp/pmbfd/dmpgrps.c"
    "modules/cexp/pmbfd/strm.c"
    "modules/cexp/pmbfd/fstrm.c"
    "modules/cexp/pmbfd/mstrm.c"
    "modules/cexp/pmbfd/dmpsym.c"
    "modules/cexp/pmbfd/dmpsymtab.c"
    "modules/cexp/pmbfd/dmpshdr.c"
    "modules/cexp/pmbfd/dmpshtab.c"
    "modules/cexp/pmbfd/dmpehdr.c"
    "modules/cexp/pmbfd/dmprels.c"
    "modules/cexp/pmbfd/dmpphdr.c"
    "modules/cexp/pmbfd/symtab.c"
    "modules/cexp/pmbfd/findsymhdrs.c"
    "modules/cexp/pmbfd/shtab.c"
    "modules/cexp/pmbfd/getgrp.c"
    "modules/cexp/pmbfd/getrel.c"
    "modules/cexp/pmbfd/getscn.c"
    "modules/cexp/pmbfd/getsym.c"
    "modules/cexp/pmbfd/putsym.c"
    "modules/cexp/pmbfd/getshdr.c"
    "modules/cexp/pmbfd/getphdr.c"
    "modules/cexp/pmbfd/putshdr.c"
    "modules/cexp/pmbfd/getehdr.c"
    "modules/cexp/pmbfd/putehdr.c"
    "modules/cexp/pmbfd/attpbfasmatch.c"
    "modules/cexp/pmbfd/attpbfasdestroy.c"
    "modules/cexp/pmbfd/attpbfasread.c"
    "modules/cexp/pmbfd/attpbfasprint.c"
    "modules/cexp/pmbfd/attpbfaprint.c"
    "modules/cexp/pmbfd/attpbprinttag.c"
    "modules/cexp/pmbfd/attset.c"
    "modules/cexp/pmbfd/attprint.c"
    "modules/cexp/pmbfd/attvendfind.c"
    "modules/cexp/pmbfd/guleb128.c"
    "modules/cexp/pmbfd/getwrd.c"
    "modules/cexp/pmbfd/att-gnu-powerpc.c"
    "modules/cexp/pmbfd/noelf64.c" # TODO: NO_64BIT
)

add_library(
    pmelf STATIC ${PMELF_SRCS}
)

target_include_directories(
    pmelf PRIVATE modules/cexp/pmbfd
)

install(
    TARGETS pmelf
)

install(
    FILES "modules/cexp/pmbfd/pmelf.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

###################################################
# pmbfd
###################################################
set(PMBFD_SRCS
    "modules/cexp/pmbfd/bfd.c"
    "modules/cexp/pmbfd/opcodesup.c"
    "modules/cexp/pmbfd/bfd-reloc-${RTEMS_ARCH}.c"
)

add_library(
    pmbfd STATIC ${PMBFD_SRCS}
)

target_include_directories(
    pmbfd PRIVATE modules/cexp/pmbfd
)

install(
    TARGETS pmbfd
)

###################################################
# libtecla
###################################################
set(TECLA_SRCS
    "modules/cexp/libtecla/chrqueue.c"
    "modules/cexp/libtecla/cplfile.c"
    "modules/cexp/libtecla/cplmatch.c"
    "modules/cexp/libtecla/direader.c"
    "modules/cexp/libtecla/errmsg.c"
    "modules/cexp/libtecla/expand.c"
    "modules/cexp/libtecla/freelist.c"
    "modules/cexp/libtecla/getline.c"
    "modules/cexp/libtecla/hash.c"
    "modules/cexp/libtecla/history.c"
    "modules/cexp/libtecla/homedir.c"
    "modules/cexp/libtecla/ioutil.c"
    "modules/cexp/libtecla/keytab.c"
    "modules/cexp/libtecla/pathutil.c"
    "modules/cexp/libtecla/pcache.c"
    "modules/cexp/libtecla/stringrp.c"
    "modules/cexp/libtecla/strngmem.c"
    "modules/cexp/libtecla/version.c"
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
        "${SRCDIR}/modules/cexp/gentab.c"
    DEPENDS "${SRCDIR}/modules/cexp/gentab.c"
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
        "${SRCDIR}/modules/cexp/cexp.y"
    DEPENDS "${SRCDIR}/modules/cexp/cexp.y"
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
    "modules/cexp"
    "modules/cexp/regexp"
    "${CMAKE_BINARY_DIR}/cexp" # Needed for cexp.tab.h
)

set(
    CEXP_SRCS
    "modules/cexp/cexp.c"
    "modules/cexp/ctyps.c"
    "modules/cexp/cexpsyms.c"
    "modules/cexp/vars.c"
    "modules/cexp/rshload.c"
    "modules/cexp/cexplock.c"
    "modules/cexp/cexpmod.c"
    "modules/cexp/cexpveneer.c"
    "modules/cexp/getopt/mygetopt_r.c"
    "modules/cexp/help.c"
    "modules/cexp/cexpsegs.c"
    "modules/cexp/cexpsegs-alloc.c"
    "modules/cexp/wrap.c"
    
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
        modules/cexp/bestline/bestline.c
        modules/cexp/bestline/bestline.h
    )
    list(
        APPEND CEXP_DEFINES
        "HAVE_BESTLINE=1"
    )
elseif (USE_TECLA)
    list(
        APPEND CEXP_SRCS
        modules/cexp/teclastuff.c
    )
    list(
        APPEND CEXP_INCLUDES
        modules/cexp/libtecla
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
        modules/cexp/elfsyms.c
        modules/cexp/elfdlmap.c
    )
else()
    list(
        APPEND CEXP_SRCS
        modules/cexp/noloader.c
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
        "modules/cexp/pmbfd"
    )
    list(
        APPEND CEXP_SRCS
        "modules/cexp/bfdstuff.c"
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
        "modules/cexp/cexprtl.c"
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
    cexp PUBLIC modules/cexp
)

install(
    TARGETS cexp
)

install(
    FILES
        "modules/cexp/cexp.h"
        "modules/cexp/cexpHelp.h"
        "modules/cexp/ctyps.h"
        "modules/cexp/cexpsyms.h"
    DESTINATION
        "${CMAKE_INSTALL_INCLUDEDIR}"
)

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
set(MISCUTILS_SRCS
    modules/miscUtils/shell-tools.c
    modules/miscUtils/mem-tools.c
)

rtems_add_object(
    miscUtils rtems-init

    ${MISCUTILS_SRCS}
)

target_include_directories(
    miscUtils PRIVATE modules/cexp
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
