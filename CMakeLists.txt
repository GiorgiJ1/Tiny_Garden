cmake_minimum_required(VERSION 3.14)
project(tiny_garden LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Use system raylib if present, otherwise fetch + build it
find_package(raylib 5.0 QUIET)
if(NOT raylib_FOUND)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW ON
    )
    FetchContent_MakeAvailable(raylib)
endif()

add_executable(tiny_garden
    src/main.cpp
    src/Game.cpp
    src/Garden.cpp
    src/Plant.cpp
    src/Weather.cpp
    src/ParticleSystem.cpp
    src/Snail.cpp
    src/Audio.cpp
    src/SaveSystem.cpp
    src/Inventory.cpp
    src/Economy.cpp
    src/Shop.cpp
    src/Menus.cpp
)
target_link_libraries(tiny_garden PRIVATE raylib)
