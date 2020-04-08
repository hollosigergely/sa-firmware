cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(observer)
add_executable(${PROJECT_NAME}
    main.cpp
    dump.cpp
    utils.cpp
    dump_std.cpp
    aggregate.cpp
    crc.c)

include_directories(
    ../firmware
    )
