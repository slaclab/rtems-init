cmake_minimum_required(VERSION 3.20)

project(rtems-init ASM C CXX)

option(BUILD_TESTS "Enable to build the tests" ON)
option(HOST_BUILD "Enable to build for host system" OFF)

set(CMAKE_VERBOSE_MAKEFILE ON)

if (RTEMS_INSTALL)
    set(SHARED_PREFIX OFF)
    set(CMAKE_INSTALL_PREFIX "${RTEMS_TOP}/target/rtems/${RTEMS_ARCH}-rtems${RTEMS_TOOL_VERSION}/${RTEMS_BSP}")
endif()

###################################################
# Cross compiled build
###################################################
if (NOT HOST_BUILD)
    include(rtems-tools/cmake/rtems.cmake)

    rtems_add_executable(
        rtems-init 
        
        src/init.c
        src/rtems-config.c
        src/util.c
        src/shell.c
        src/getopt_s.c
        src/nvram.c
        src/ntp.c
        src/dummy.cc
    )

    get_target_property(CURRENT_LINK_FLAGS rtems-init LINK_FLAGS)

    # Force linker to emit symbols
    set_target_properties(
        rtems-init rtems-init-exe PROPERTIES 
            #LINKER_LANGUAGE CXX
            #-Wl,--whole-archive -lrtemscpu -Wl,--no-whole-archive
            LINK_FLAGS "-Wl,--whole-archive -lstdc++ -Wl,--no-whole-archive"
    )

    target_link_libraries(
        rtems-init
        
        rtemscxx rtemscpu rtemsbsp ntp telnetd bsd debugger tftpfs c m
    )

    rtems_add_rootfs(
        rtems-init "${CMAKE_CURRENT_LIST_DIR}/rootfs" "TAR"
    )

    #rtems_add_executable(
    #    test-dl-target
    #    
    #    src/test/test-dl-target.c
    #)
endif()

###################################################
# Host tests
###################################################
if (BUILD_TESTS AND HOST_BUILD)
    add_executable(misc_test src/test/test.c)
endif()