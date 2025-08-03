cmake_minimum_required(VERSION 3.10)
project(sgfc C)

set(CMAKE_C_STANDARD 99)

include_directories(src)
include_directories(tests)

set(src_files
        src/all.h
        src/protos.h
        src/encoding.c
        src/error.c
        src/execute.c
        src/gameinfo.c
        src/load.c
        src/options.c
        src/parse.c
        src/parse2.c
        src/properties.c
        src/save.c
        src/strict.c
        src/util.c)

# Create static library from SGFC source files (excluding main.c)
add_library(sgfc STATIC ${src_files})

# Find and link iconv
find_package(Iconv REQUIRED)
target_link_libraries(sgfc Iconv::Iconv)
target_include_directories(sgfc PUBLIC src)
