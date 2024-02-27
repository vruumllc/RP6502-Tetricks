cmake_minimum_required(VERSION 3.13)

project(MY-RP6502-PROJECT C ASM)

add_subdirectory(tools)

add_executable(tetricks)
rp6502_executable(tetricks)
target_include_directories(tetricks PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)
target_sources(tetricks PRIVATE
    src/bitmap_graphics.c
    src/tetricks.c
)
