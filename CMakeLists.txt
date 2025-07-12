cmake_minimum_required(VERSION 3.24)
project(bzzt VERSION 0.1 LANGUAGES C)

# -------------  Global settings -------------
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose Debug, Release, RelWithDebInfo or MinSizeRel" FORCE)
endif()

if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()


file(GLOB_RECURSE BZZT_SRC CONFIGURE_DEPENDS
     src/*.c)

add_executable(bzzt ${BZZT_SRC})

# -------------  Dependencies -------------
find_package(raylib 5 QUIET) 

if (NOT raylib_FOUND)
  message(STATUS "Raylib not found; fetching it…")
  include(FetchContent)
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
  )
  FetchContent_MakeAvailable(raylib)
endif()

# -------------  Linkage -------------
target_link_libraries(bzzt PRIVATE raylib)

# Windows needs these extra system libs when using GCC/Clang
if (WIN32 AND NOT MSVC)
  target_link_libraries(bzzt PRIVATE
        winmm gdi32 opengl32 shell32 pthread)
endif()

# -------------  Include paths -------------
target_include_directories(bzzt PRIVATE src
src/core
src/editor
src/ui
src/utils)

# -------------  IDE niceties -------------
set_target_properties(bzzt PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Generates compile_commands.json for clangd / cpp-tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
