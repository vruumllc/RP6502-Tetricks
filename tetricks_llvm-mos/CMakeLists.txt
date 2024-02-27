cmake_minimum_required(VERSION 3.18)

add_subdirectory(tools)

set(LLVM_MOS_PLATFORM rp6502)

find_package(llvm-mos-sdk REQUIRED)

project(MY-RP6502-PROJECT)

add_executable(tetricks)
rp6502_executable(tetricks)
target_include_directories(tetricks PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)
target_sources(tetricks PRIVATE
    src/bitmap_graphics.c
    src/tetricks.c
)
