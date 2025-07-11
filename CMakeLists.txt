cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(Statuses VERSION 1.0.0)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

file(GLOB_RECURSE SOURCES
    src/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME})

CPMAddPackage(
  NAME ixwebsocket
  GITHUB_REPOSITORY machinezone/IXWebSocket
  VERSION 11.4.6
  OPTIONS "USE_ZLIB FALSE"
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND WIN32)
    target_compile_definitions(ixwebsocket PRIVATE _ITERATOR_DEBUG_LEVEL=0)
endif()

target_link_libraries(${PROJECT_NAME} ixwebsocket)

CPMAddPackage("gh:GlobedGD/argon@1.1.7")
target_link_libraries(${PROJECT_NAME} argon)