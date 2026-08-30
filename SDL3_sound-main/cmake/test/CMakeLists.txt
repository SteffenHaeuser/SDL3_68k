# This cmake build script is meant for verifying the various CMake configuration script.

cmake_minimum_required(VERSION 3.12)
project(sdl_test LANGUAGES C)

cmake_policy(SET CMP0074 NEW)

# Override CMAKE_FIND_ROOT_PATH_MODE to allow search for SDL3_sound outside of sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)

include(FeatureSummary)

option(TEST_SHARED "Test linking to shared SDL3_sound library" ON)
add_feature_info("TEST_SHARED" TEST_SHARED "Test linking with shared library")

option(TEST_STATIC "Test linking to static SDL3_sound libary" ON)
add_feature_info("TEST_STATIC" TEST_STATIC "Test linking with static library")

macro(my_find_sdl3)
    # FIXME: this should unconditionally use CMake config file
    find_package(SDL3)
    if(NOT TARGET SDL3::SDL3)
        find_library(SDL3_LIBRARY NAMES SDL3)
        find_path(SDL3_INCLUDE_DIRS FILES SDL.h SUFFIXES SDL3)
        if(NOT SDL3_LIBRARY OR NOT SDL3_INCLUDE_DIRS)
            message(FATAL_ERROR "Faild to find SDL3 (configure SDL3_LIBRARY and SDL3_INCLUDE_DIRS)")
        endif()
        add_library(SDL3::SDL3 SHARED IMPORTED)
        set_target_properties(SDL3::SDL3 PROPERTIES IMPORTED_LOCATION "${SDL3_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${SDL3_INCLUDE_DIRS}")
    endif()
endmacro()

if(TEST_SHARED)
    find_package(SDL3_sound REQUIRED CONFIG)
    my_find_sdl3()
    add_executable(main_shared main.c)
    target_link_libraries(main_shared PRIVATE SDL3_sound::SDL3_sound SDL3::SDL3)
endif()

if(TEST_STATIC)
    find_package(SDL3_sound REQUIRED CONFIG)
    my_find_sdl3()
    add_executable(main_static main.c)
    target_link_libraries(main_static PRIVATE SDL3_sound::SDL3_sound-static SDL3::SDL3)
endif()

feature_summary(WHAT ALL)
