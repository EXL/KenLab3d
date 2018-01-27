##########################################################################################
## The MIT License (MIT)
##
## Copyright (c) 2017 EXL
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included in all
## copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
## OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
## SOFTWARE.
##########################################################################################

cmake_minimum_required(VERSION 3.4.1)
project("KenLab3D")

option(ANDROID "Enable Android Build instead of Desktop" ON)

include_directories(${PROJECT_NAME})

set(SOURCES ${PROJECT_NAME}/graphx.c
            ${PROJECT_NAME}/init.c
            ${PROJECT_NAME}/subs.c
            ${PROJECT_NAME}/oldlab3d.c
            ${PROJECT_NAME}/lab3d.c
            ${PROJECT_NAME}/adlibemu.c
            ${PROJECT_NAME}/setup.c)

if(ANDROID)
    add_definitions(-DANDROID_NDK -DOPENGLES -DUSE_SDL2 -DGL_GLEXT_PROTOTYPES)

    # Glues
    set(GLUES "Glues")
    set(GLUES_SOURCES ${GLUES}/glues_error.c
                    ${GLUES}/glues_mipmap.c
                    ${GLUES}/glues_project.c
                    ${GLUES}/glues_quad.c
                    ${GLUES}/glues_registry.c)
    add_library(${GLUES} STATIC ${GLUES_SOURCES})

    # LodePNG
    set(LODEPNG "LodePNG")
    set(LODEPNG_SOURCES ${LODEPNG}/lodepng.c)
    add_library(${LODEPNG} STATIC ${LODEPNG_SOURCES})

    # SDL2
    set(SDL2 "SDL2")
    set(SDL2_SOURCES ${SDL2}/src/SDL_assert.c
                ${SDL2}/src/events/SDL_gesture.c
                ${SDL2}/src/events/SDL_keyboard.c
                ${SDL2}/src/events/SDL_mouse.c
                ${SDL2}/src/events/SDL_events.c
                ${SDL2}/src/events/SDL_touch.c
                ${SDL2}/src/events/SDL_quit.c
                ${SDL2}/src/events/SDL_clipboardevents.c
                ${SDL2}/src/events/SDL_windowevents.c
                ${SDL2}/src/events/SDL_dropevents.c
                ${SDL2}/src/loadso/dlopen/SDL_sysloadso.c
                ${SDL2}/src/atomic/SDL_atomic.c
                ${SDL2}/src/atomic/SDL_spinlock.c
                ${SDL2}/src/filesystem/android/SDL_sysfilesystem.c
                ${SDL2}/src/thread/pthread/SDL_syscond.c
                ${SDL2}/src/thread/pthread/SDL_systhread.c
                ${SDL2}/src/thread/pthread/SDL_syssem.c
                ${SDL2}/src/thread/pthread/SDL_sysmutex.c
                ${SDL2}/src/thread/pthread/SDL_systls.c
                ${SDL2}/src/thread/SDL_thread.c
                ${SDL2}/src/SDL_log.c
                ${SDL2}/src/dynapi/SDL_dynapi.c
                ${SDL2}/src/SDL_hints.c
                ${SDL2}/src/video/SDL_blit.c
                ${SDL2}/src/video/SDL_clipboard.c
                ${SDL2}/src/video/SDL_egl.c
                ${SDL2}/src/video/SDL_blit_auto.c
                ${SDL2}/src/video/SDL_blit_N.c
                ${SDL2}/src/video/SDL_blit_1.c
                ${SDL2}/src/video/SDL_pixels.c
                ${SDL2}/src/video/SDL_bmp.c
                ${SDL2}/src/video/SDL_blit_copy.c
                ${SDL2}/src/video/SDL_RLEaccel.c
                ${SDL2}/src/video/SDL_blit_slow.c
                ${SDL2}/src/video/SDL_shape.c
                ${SDL2}/src/video/SDL_blit_0.c
                ${SDL2}/src/video/SDL_surface.c
                ${SDL2}/src/video/SDL_stretch.c
                ${SDL2}/src/video/SDL_video.c
                ${SDL2}/src/video/SDL_fillrect.c
                ${SDL2}/src/video/SDL_blit_A.c
                ${SDL2}/src/video/android/SDL_androidkeyboard.c
                ${SDL2}/src/video/android/SDL_androidvideo.c
                ${SDL2}/src/video/android/SDL_androidevents.c
                ${SDL2}/src/video/android/SDL_androidmouse.c
                ${SDL2}/src/video/android/SDL_androidgl.c
                ${SDL2}/src/video/android/SDL_androidmessagebox.c
                ${SDL2}/src/video/android/SDL_androidwindow.c
                ${SDL2}/src/video/android/SDL_androidtouch.c
                ${SDL2}/src/video/android/SDL_androidclipboard.c
                ${SDL2}/src/video/SDL_rect.c
                ${SDL2}/src/SDL_error.c
                ${SDL2}/src/audio/SDL_mixer.c
                ${SDL2}/src/audio/SDL_audio.c
                ${SDL2}/src/audio/SDL_audiotypecvt.c
                ${SDL2}/src/audio/SDL_audiodev.c
                ${SDL2}/src/audio/SDL_wave.c
                ${SDL2}/src/audio/SDL_audiocvt.c
                ${SDL2}/src/audio/android/SDL_androidaudio.c
                ${SDL2}/src/core/android/SDL_android.c
                ${SDL2}/src/timer/unix/SDL_systimer.c
                ${SDL2}/src/timer/SDL_timer.c
                ${SDL2}/src/SDL.c
                ${SDL2}/src/stdlib/SDL_iconv.c
                ${SDL2}/src/stdlib/SDL_malloc.c
                ${SDL2}/src/stdlib/SDL_string.c
                ${SDL2}/src/stdlib/SDL_stdlib.c
                ${SDL2}/src/stdlib/SDL_getenv.c
                ${SDL2}/src/stdlib/SDL_qsort.c
                ${SDL2}/src/joystick/SDL_gamecontroller.c
                ${SDL2}/src/joystick/SDL_joystick.c
                ${SDL2}/src/joystick/android/SDL_sysjoystick.c
                ${SDL2}/src/cpuinfo/SDL_cpuinfo.c
                ${SDL2}/src/render/SDL_render.c
                ${SDL2}/src/render/software/SDL_blendpoint.c
                ${SDL2}/src/render/software/SDL_drawpoint.c
                ${SDL2}/src/render/software/SDL_blendfillrect.c
                ${SDL2}/src/render/software/SDL_drawline.c
                ${SDL2}/src/render/software/SDL_render_sw.c
                ${SDL2}/src/render/software/SDL_blendline.c
                ${SDL2}/src/render/software/SDL_rotate.c
                ${SDL2}/src/render/opengles/SDL_render_gles.c
                ${SDL2}/src/render/SDL_yuv_sw.c
                ${SDL2}/src/file/SDL_rwops.c)
    add_library(${SDL2} SHARED ${SDL2_SOURCES})
    target_link_libraries(${SDL2} dl GLESv1_CM GLESv2 log android)

    include_directories(${INCLUDE_DIRECTORIES} ${GLUES} ${LODEPNG} ${SDL2}/include)
    set(SOURCES ${SOURCES} ${PROJECT_NAME}/sdl2keyhelper.c
            ${PROJECT_NAME}/androidutils.c
            ${SDL2}/src/main/android/SDL_android_main.c)
    add_library(${PROJECT_NAME} SHARED ${SOURCES})
    target_link_libraries(${PROJECT_NAME} ${SDL2} ${LODEPNG} ${GLUES} log GLESv1_CM)
else()
    option(SDL2_DESKTOP "Enable SDL2 Build instead of SDL" ON)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../assets/KenLabData/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    if(SDL2_DESKTOP)
        message(STATUS "Build w/ using SDL2 libraries!")
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
        set(TARGET_NAME "KenLab3D-SDL2")
        find_package(OpenGL REQUIRED)
        find_package(SDL2 REQUIRED)
        find_package(SDL2_image REQUIRED)
        add_definitions(-DUSE_SDL2)
        include_directories(${INCLUDE_DIRECTORIES} ${OPENGL_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${SDL2_IMAGE_INCLUDE_DIR})
        set(SOURCES ${SOURCES} ${PROJECT_NAME}/sdl2keyhelper.c)
        add_executable(${TARGET_NAME} ${SOURCES})
        target_link_libraries(${TARGET_NAME} ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARY} ${OPENGL_LIBRARIES} m)
    else()
        message(STATUS "Build w/ using SDL libraries!")
        set(TARGET_NAME "KenLab3D-SDL")
        find_package(OpenGL REQUIRED)
        find_package(SDL REQUIRED)
        find_package(SDL_image REQUIRED)
        add_executable(${TARGET_NAME} ${SOURCES})
        target_link_libraries(${TARGET_NAME} ${SDL_LIBRARY} ${SDL_IMAGE_LIBRARY} ${OPENGL_LIBRARIES} m)
    endif()
    add_custom_target(finalMessage ALL
        ${CMAKE_COMMAND} -E cmake_echo_color --green "Output directory: ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}, executable: ${TARGET_NAME}."
        COMMENT "Final Message")
    add_dependencies(finalMessage ${TARGET_NAME})
endif()
