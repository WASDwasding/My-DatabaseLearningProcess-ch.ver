cmake_minimum_required(VERSION 3.16)
project(p3_page LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(p3_page STATIC
  src/table_page.cpp
)
target_include_directories(p3_page PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(test_table_page tests/test_table_page.cpp)
target_link_libraries(test_table_page PRIVATE p3_page)

enable_testing()
add_test(NAME table_page_tests COMMAND test_table_page)
