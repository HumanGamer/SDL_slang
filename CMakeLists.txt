cmake_minimum_required(VERSION 3.30)
project(SDL_slang)

set(PROJECT_OUT_DIR ${CMAKE_BINARY_DIR}/game)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_OUT_DIR})

find_package(SDL3 REQUIRED)
find_package(slang REQUIRED)

add_custom_target(prjcopy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${PROJECT_OUT_DIR}/assets
    COMMENT "Copying assets to output directory")

add_subdirectory(src)
add_subdirectory(examples)
