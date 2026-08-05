cmake_minimum_required(VERSION 3.16)
project(p6_column_store LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(p6_column_store INTERFACE)
target_include_directories(p6_column_store INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(test_column_store tests/test_column_store.cpp)
target_link_libraries(test_column_store PRIVATE p6_column_store)

enable_testing()
add_test(NAME column_store_tests COMMAND test_column_store)
