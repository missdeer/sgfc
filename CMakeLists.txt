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

add_executable(sgfc
        ${src_files}
        src/main.c)

target_link_libraries(sgfc m)

add_executable(sgfc-test
        ${src_files}
        tests/test-runner.c
        tests/test-common.h
        tests/test-helper.c
        tests/check-value.c
        tests/delete-node.c
        tests/encoding.c
        tests/load-properties.c
        tests/options.c
        tests/other-games.c
        tests/parse-text.c
        tests/position.c
        tests/test-files.c
        tests/trigger-errors.c
        tests/value-length.c)

target_link_libraries(sgfc-test check rt pthread subunit m)
