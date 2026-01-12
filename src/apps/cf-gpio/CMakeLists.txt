rtems_add_executable(
    TARGET cf-gpio
    
    SOURCES
        main.c
        ../../common/util.c
)

target_include_directories(
    cf-gpio PUBLIC
    
    ${CMAKE_CURRENT_LIST_DIR}/../
    ${CMAKE_CURRENT_LIST_DIR}/../../common
)

target_link_libraries(
    cf-gpio PUBLIC
    
    ${NETWORK_LIB}
    rtemscxx
    rtemscpu
    rtemsbsp
    stdc++
    c
    m
)