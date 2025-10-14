cmake_minimum_required(VERSION 3.10)

project(RGSEngine)

find_package(SDL3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

add_executable(RGSEngine src/Engine.cpp)

target_link_libraries(RGSEngine PRIVATE SDL3::SDL3)
target_link_libraries(RGSEngine PRIVATE assimp::assimp)
target_link_libraries(RGSEngine PRIVATE glad::glad)