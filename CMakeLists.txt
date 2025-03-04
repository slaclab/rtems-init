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

target_link_libraries(
    rtems-init
    
    rtemscpu rtemsbsp ntp telnetd bsd debugger tftpfs c m
)

rtems_add_rootfs(
    rtems-init "${CMAKE_CURRENT_LIST_DIR}/rootfs"
)