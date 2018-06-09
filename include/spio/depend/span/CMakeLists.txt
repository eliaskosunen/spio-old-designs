cmake_minimum_required(VERSION 3.2.3)

project(span CXX)

set(WERROR OFF CACHE BOOL "Treat warnings as errors")

set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()
file(GLOB span_tests_sources tests/*.cpp)
add_executable(span_tests ${span_tests_sources})
add_test(NAME tests COMMAND span_tests)

target_include_directories(span_tests PUBLIC "${PROJECT_SOURCE_DIR}/span")
target_include_directories(span_tests SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/tests/doctest/doctest")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(span_tests PRIVATE
        -pedantic-errors
        -fvisibility=hidden
        -fstrict-aliasing)
    if(NOT CMAKE_CXX_STANDARD)
        target_compile_options(span_tests PRIVATE
            -std=c++1z)
    endif()

    if(WERROR)
        target_compile_options(span_tests PRIVATE -Werror)
    else()
        target_compile_options(span_tests PRIVATE "$<$<CONFIG:RELEASE>:-Werror>")
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(span_tests PRIVATE
        -Wall
        -Wextra
        -fdiagnostics-show-option
        -Wconversion
        -Wold-style-cast
        -Wfloat-equal
        -Wlogical-op
        -Wundef
        -Wredundant-decls
        -Wshadow
        -Wstrict-overflow=5
        -Wwrite-strings
        -Wpointer-arith
        -Wcast-qual
        -Wformat=2
        -Wswitch-default
        -Wmissing-include-dirs
        -Wcast-align
        -Wswitch-enum
        -Wnon-virtual-dtor
        -Wsign-conversion
        -Wdisabled-optimization
        #-Weffc++
        -Winline
        -Winvalid-pch
        -Wmissing-declarations
        -Woverloaded-virtual
        -Wno-strict-overflow)

    if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
        target_compile_options(span_tests PRIVATE
            -Wdouble-promotion
            -Wtrampolines
            -Wzero-as-null-pointer-constant
            -Wuseless-cast
            -Wvector-operation-performance
            -Wsized-deallocation)
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
        target_compile_options(span_tests PRIVATE -pedantic)
    else()
        target_compile_options(span_tests PRIVATE
            -Wpedantic
            -ftemplate-backtrace-limit=0)
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.6)
        target_compile_options(span_tests PRIVATE -Wnoexcept)
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(span_tests PRIVATE
        -Weverything
        -ftemplate-backtrace-limit=0
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-padded
        -Wno-gnu-statement-expression)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(span_tests PRIVATE
        /std:c++latest
        /W4)
endif()
