cmake_minimum_required(VERSION 3.16)
project(p8_bplustree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(p8_bpt
  src/b_plus_tree.cpp
)
target_include_directories(p8_bpt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(test_b_plus_tree tests/test_b_plus_tree.cpp)
target_link_libraries(test_b_plus_tree PRIVATE p8_bpt)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  enable_testing()
  add_test(NAME b_plus_tree COMMAND test_b_plus_tree)
endif()
