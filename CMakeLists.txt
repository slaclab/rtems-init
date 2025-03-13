cmake_minimum_required(VERSION 3.20)

project(rtems-init C)

if (NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "${RTEMS_TOP}/target/rtems/${RTEMS_ARCH}-rtems${RTEMS_VERSION}/${RTEMS_BSP}")
endif()

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
)

# Force linker language to C++ to bring in C++ STL symbols
#set_target_properties(
#    rtems-init PROPERTIES 
#        #LINKER_LANGUAGE CXX
#        LINK_FLAGS "-Wl,--no-gc-sections"
#)

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
