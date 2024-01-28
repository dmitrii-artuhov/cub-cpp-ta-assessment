cmake_minimum_required(VERSION 3.12)

project(any)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(TESTS_TARGET tests)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Werror) # -fno-rtti
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/W4 /WX)
endif()

set(ANY_SRCS 
    src/any.h
    src/any.cpp
    src/has-rtti.h
)

add_executable(${TESTS_TARGET} src/test.cpp)
