project(iot)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${iot_SOURCE_DIR}/ ${CMAKE_MODULE_PATH})
set(ALL_SRCS CACHE INTERNAL "description" FORCE)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
