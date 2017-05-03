cmake_minimum_required(VERSION 3.4.1)
project("LodePNG")

set(HEADERS lodepng.h)
set(SOURCES lodepng.c)

add_definitions(-DANDROID_NDK)
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
