cmake_minimum_required(VERSION 3.4.1)
project("KenLab3D")

include_directories(${PROJECT_NAME})

set(HEADERS ${PROJECT_NAME}/adlibemu.h
            ${PROJECT_NAME}/lab3d.h
            ${PROJECT_NAME}/sdl2keyhelper.h)

set(SOURCES ${PROJECT_NAME}/sdl2keyhelper.c
            ${PROJECT_NAME}/graphx.c
            ${PROJECT_NAME}/init.c
            ${PROJECT_NAME}/subs.c
            ${PROJECT_NAME}/oldlab3d.c
            ${PROJECT_NAME}/lab3d.c
            ${PROJECT_NAME}/adlibemu.c
            ${PROJECT_NAME}/setup.c
            ${PROJECT_NAME}/androidutils.c)

add_definitions(-DANDROID_NDK -DOPENGLES -DUSE_SDL2)

# 3rd Party Libraries
set(GLUES_DIR "Glues")
set(LODEPNG_DIR "LodePNG")
set(SDL2_DIR "SDL2-2.0.4-compact")

set(GLUES_OUT_DIR "${GLUES_DIR}/outputs")
set(LODEPNG_OUT_DIR "${LODEPNG_DIR}/outputs")
set(SDL2_OUT_DIR "${SDL2_DIR}/outputs")

file(MAKE_DIRECTORY ${GLUES_OUT_DIR})
file(MAKE_DIRECTORY ${LODEPNG_OUT_DIR})
file(MAKE_DIRECTORY ${SDL2_OUT_DIR})

add_subdirectory(${GLUES_DIR} ${GLUES_OUT_DIR})
add_subdirectory(${LODEPNG_DIR} ${LODEPNG_OUT_DIR})
add_subdirectory(${SDL2_DIR} ${SDL2_OUT_DIR})

add_library(lib_Glues STATIC IMPORTED)
add_library(lib_LodePNG STATIC IMPORTED)
add_library(lib_SDL2 SHARED IMPORTED)

set_target_properties(lib_Glues PROPERTIES IMPORTED_LOCATION
                        ${GLUES_OUT_DIR}/${ANDROID_ABI}/libGlues.a)
set_target_properties(lib_LodePNG PROPERTIES IMPORTED_LOCATION
                        ${LODEPNG_OUT_DIR}/${ANDROID_ABI}/libLodePNG.a)
set_target_properties(lib_SDL2 PROPERTIES IMPORTED_LOCATION
                        ${SDL2_OUT_DIR}/${ANDROID_ABI}/libSDL2.so)
include_directories(${INCLUDE_DIRECTORIES} ${GLUES_DIR} ${LODEPNG_DIR} ${SDL2_DIR}/include)

add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} lib_SDL2 lib_Glues lib_LodePNG log GLESv1_CM)

