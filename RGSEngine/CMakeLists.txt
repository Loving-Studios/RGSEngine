cmake_minimum_required(VERSION 3.10)

project(RGSEngine)

find_package(SDL3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

file(GLOB SOURCES "src/*.cpp" "src/*.h")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "Source" FILES ${SOURCES})
add_executable(RGSEngine ${SOURCES})

target_link_libraries(RGSEngine PRIVATE SDL3::SDL3)
target_link_libraries(RGSEngine PRIVATE assimp::assimp)
target_link_libraries(RGSEngine PRIVATE glad::glad)
target_link_libraries(RGSEngine PRIVATE glm::glm)