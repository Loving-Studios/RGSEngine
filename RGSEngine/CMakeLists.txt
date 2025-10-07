cmake_minimum_required(VERSION 3.10)

project(RGSEngine)

find_package(fmt CONFIG REQUIRED)

add_executable(RGSEngine helloworld.cpp)

target_link_libraries(RGSEngine PRIVATE fmt::fmt)