cmake_minimum_required(VERSION 2.8)

project(LTBL2)

include_directories("${PROJECT_SOURCE_DIR}/source")

# This is only required for the script to work in the version control
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")
 
find_package(SFML 2.2 REQUIRED system window graphics)
 
include_directories(${SFML_INCLUDE_DIR})
 
add_executable(CHTMGPU "${PROJECT_SOURCE_DIR}/source/Main.cpp")

target_link_libraries(CHTMGPU ${SFML_LIBRARIES})