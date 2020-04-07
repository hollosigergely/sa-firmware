cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(observer_decoder)
add_executable(${PROJECT_NAME}
    main.cpp
    dump.cpp
    utils.cpp
    dump_std.cpp
    aggregate.cpp)

include_directories(
    ../firmware
    )
