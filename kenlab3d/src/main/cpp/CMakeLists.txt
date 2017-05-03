cmake_minimum_required(VERSION 3.4.1)
project("KenLab3D")

option(ANDROID "Enable Android Build instead Desktop" ON)

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
            ${PROJECT_NAME}/setup.c)

if(ANDROID)
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
    add_library(libGlues STATIC IMPORTED)
    add_library(libLodePNG STATIC IMPORTED)
    add_library(libSDL2 SHARED IMPORTED)
    set_target_properties(libGlues PROPERTIES IMPORTED_LOCATION
                            ${GLUES_OUT_DIR}/${ANDROID_ABI}/libGlues.a)
    set_target_properties(libLodePNG PROPERTIES IMPORTED_LOCATION
                            ${LODEPNG_OUT_DIR}/${ANDROID_ABI}/libLodePNG.a)
    set_target_properties(libSDL2 PROPERTIES IMPORTED_LOCATION
                            ${SDL2_OUT_DIR}/${ANDROID_ABI}/libSDL2.so)
    include_directories(${INCLUDE_DIRECTORIES} ${GLUES_DIR} ${LODEPNG_DIR} ${SDL2_DIR}/include)

    add_definitions(-DANDROID_NDK -DOPENGLES -DUSE_SDL2)
    set(SOURCES ${SOURCES}
            ${PROJECT_NAME}/androidutils.c
            )
    add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})
    target_link_libraries(${PROJECT_NAME} libSDL2 libGlues libLodePNG log GLESv1_CM)
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    find_package(SDL)
    find_package(OpenGL)
    include_directories(${INCLUDE_DIRECTORIES} ${SDL_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
    target_link_libraries(${PROJECT_NAME} ${SDL_LIBRARY} ${OPENGL_LIBRARIES})
    add_custom_target(finalMessage ALL
        ${CMAKE_COMMAND} -E cmake_echo_color --green "Output directory: ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}, executable: ${PROJECT_NAME}."
        COMMENT "Final Message")
    add_dependencies(finalMessage ${PROJECT_NAME})
endif()
