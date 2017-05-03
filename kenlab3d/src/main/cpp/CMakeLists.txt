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
    add_definitions(-DANDROID_NDK -DOPENGLES -DUSE_SDL2)
    set(SOURCES ${SOURCES}
            ${PROJECT_NAME}/androidutils.c
            )
    add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})
    target_link_libraries(${PROJECT_NAME} log GLESv1_CM)
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
