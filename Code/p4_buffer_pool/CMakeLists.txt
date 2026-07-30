cmake_minimum_required(VERSION 3.16)
project(p4_buffer_pool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(p4_buffer_pool STATIC
  src/buffer_pool_manager.cpp
)
target_include_directories(p4_buffer_pool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(test_buffer_pool tests/test_buffer_pool.cpp)
target_link_libraries(test_buffer_pool PRIVATE p4_buffer_pool)

enable_testing()
add_test(NAME buffer_pool_tests COMMAND test_buffer_pool)
