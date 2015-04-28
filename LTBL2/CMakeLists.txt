cmake_minimum_required(VERSION 2.8)

project(LTBL2)

# Compiler-specific flags and definitions
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
endif()

include_directories("${PROJECT_SOURCE_DIR}/source")

# This is only required for the script to work in the version control
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")
 
find_package(SFML 2.2 REQUIRED system window graphics)
 
include_directories(${SFML_INCLUDE_DIR})
 
set( SOURCE_PATH "${PROJECT_SOURCE_DIR}/source" )
set( SOURCES
    "${SOURCE_PATH}/ltbl/Math.h"
    "${SOURCE_PATH}/ltbl/Math.cpp"  
    "${SOURCE_PATH}/ltbl/lighting/LightDirectionEmission.cpp"
    "${SOURCE_PATH}/ltbl/lighting/LightDirectionEmission.h"
    "${SOURCE_PATH}/ltbl/lighting/LightPointEmission.cpp"
    "${SOURCE_PATH}/ltbl/lighting/LightPointEmission.h"
    "${SOURCE_PATH}/ltbl/lighting/LightShape.cpp"
    "${SOURCE_PATH}/ltbl/lighting/LightShape.h"
    "${SOURCE_PATH}/ltbl/lighting/LightSystem.cpp"
    "${SOURCE_PATH}/ltbl/lighting/LightSystem.h"
    "${SOURCE_PATH}/ltbl/quadtree/DynamicQuadtree.cpp"
    "${SOURCE_PATH}/ltbl/quadtree/DynamicQuadtree.h"
    "${SOURCE_PATH}/ltbl/quadtree/Quadtree.cpp"
    "${SOURCE_PATH}/ltbl/quadtree/Quadtree.h"
    "${SOURCE_PATH}/ltbl/quadtree/QuadtreeNode.cpp"
    "${SOURCE_PATH}/ltbl/quadtree/QuadtreeNode.h"
    "${SOURCE_PATH}/ltbl/quadtree/QuadtreeOccupant.cpp"
    "${SOURCE_PATH}/ltbl/quadtree/QuadtreeOccupant.h"
    "${SOURCE_PATH}/ltbl/quadtree/StaticQuadtree.cpp"
    "${SOURCE_PATH}/ltbl/quadtree/StaticQuadtree.h"
)

add_library(LTBL2 SHARED ${SOURCES})
set_target_properties(LTBL2 PROPERTIES DEBUG_POSTFIX -d)

target_link_libraries(LTBL2 ${SFML_LIBRARIES})