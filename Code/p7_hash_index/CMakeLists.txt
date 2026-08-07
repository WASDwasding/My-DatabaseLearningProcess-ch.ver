cmake_minimum_required(VERSION 3.16)
project(p7_hash_index LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(p7_hash
  src/extendible_hash.cpp
)
target_include_directories(p7_hash PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(test_extendible_hash tests/test_extendible_hash.cpp)
target_link_libraries(test_extendible_hash PRIVATE p7_hash)

enable_testing()
add_test(NAME extendible_hash COMMAND test_extendible_hash)
