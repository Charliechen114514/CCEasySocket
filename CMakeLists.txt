cmake_minimum_required(VERSION 3.22)
project(CChatRoom LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "Ready to Build Chat Rooms")

add_subdirectory(library)

add_subdirectory(test)