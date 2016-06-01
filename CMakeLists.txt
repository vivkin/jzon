cmake_minimum_required(VERSION 2.8.12)
enable_testing()
project(jzon)

set(CMAKE_CXX_STANDARD 11)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

add_executable(jzontests tests.cpp)
add_executable(jzonprint print.cpp)
add_executable(jzonstats stats.cpp)

add_test(${PROJECT_NAME} jzontests)
