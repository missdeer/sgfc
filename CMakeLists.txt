cmake_minimum_required(VERSION 3.10)
project(sgfc C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(CTest)

if(BUILD_TESTING)
        find_package(Threads REQUIRED)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(CHECK REQUIRED IMPORTED_TARGET check)
endif()

set(sgfc_core_sources
        src/all.h
        src/protos.h
        src/helpers.h
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

add_library(sgfc_core STATIC ${sgfc_core_sources})

target_include_directories(sgfc_core
        PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_link_libraries(sgfc_core
        PUBLIC
        m)

add_executable(sgfc
        src/main.c)

target_link_libraries(sgfc
        PRIVATE
        sgfc_core)

if(BUILD_TESTING)
        add_executable(sgfc-test
                tests/test-runner.c
                tests/test-common.h
                tests/test-helper.c
                tests/check-encoding.c
                tests/check-options.c
                tests/check-value.c
                tests/delete-node.c
                tests/gameinfo-correction.c
                tests/gameinfo-interactive.c
                tests/input-size.c
                tests/load-properties.c
                tests/nesting.c
                tests/other-games.c
                tests/parse-move.c
                tests/parse-text.c
                tests/position.c
                tests/saving.c
                tests/test-files.c
                tests/trigger-errors.c
                tests/value-length.c)

        target_compile_definitions(sgfc-test
                PRIVATE
                _POSIX_C_SOURCE=200112L)

        target_include_directories(sgfc-test
                PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/tests")

        target_link_libraries(sgfc-test
                PRIVATE
                sgfc_core
                PkgConfig::CHECK
                Threads::Threads)

        add_test(NAME sgfc-test COMMAND sgfc-test)
        set_tests_properties(sgfc-test PROPERTIES
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests")
endif()
