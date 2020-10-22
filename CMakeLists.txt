cmake_minimum_required(VERSION 3.10)
project(sgfc C)

set(CMAKE_C_STANDARD 11)

include_directories(src)

add_executable(sgfc
        src/all.h
        src/execute.c
        src/gameinfo.c
        src/load.c
        src/main.c
        src/options.c
        src/parse.c
        src/parse2.c
        src/properties.c
        src/protos.h
        src/save.c
        src/strict.c
        src/util.c)
