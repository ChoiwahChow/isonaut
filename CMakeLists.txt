cmake_minimum_required(VERSION 2.6)

set (CMAKE_CXX_STANDARD 11)

project(isonaut)
set(CMAKE_STATIC_LIBRARY_PREFIX "")

add_library(libisonaut
             model.cpp
             isofilter.cpp
             nauty_utils.cpp
            )

add_executable (isonaut ./main.cpp)

target_link_libraries(isonaut libisonaut ${CMAKE_SOURCE_DIR}/nauty.a)
