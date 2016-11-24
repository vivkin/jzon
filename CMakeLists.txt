cmake_minimum_required(VERSION 3.6)
project(gason VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /wd4201 /D_CRT_SECURE_NO_WARNINGS")
endif()

include_directories("include")

add_executable(jzontests test/tests.cpp)
add_executable(jzonprint src/print.cpp)
add_executable(jzonstats src/stats.cpp)

enable_testing()
add_test(${PROJECT_NAME} jzontests)
