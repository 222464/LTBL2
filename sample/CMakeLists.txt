include_directories(.)

# Main sample source file
set(sample_src
    main.cpp)

# PugiXML source files
set(pugixml_src
    pugixml/pugixml.cpp)
include_directories("pugixml")

# TMX source files
set(tmx_src
    tmx/DebugShape.cpp
    tmx/MapLayer.cpp
    tmx/MapLoaderPrivate.cpp
    tmx/MapLoaderPublic.cpp
    tmx/MapObject.cpp
    tmx/TMXQuadTreeNode.cpp)
include_directories("tmx")

# Libraries are compiled as static libraries so that only
# one executable is needed to run the program
add_library(pugixml SHARED ${pugixml_src})
add_library(tmx SHARED ${tmx_src})

target_link_libraries(tmx pugixml)

add_executable(sample ${sample_src})
target_link_libraries(sample tmx ltbl2)

# Copy data files to the location of the sample app
file(COPY "${PROJECT_SOURCE_DIR}/data" DESTINATION "${PROJECT_BINARY_DIR}/bin")