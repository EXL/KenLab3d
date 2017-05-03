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

option(ANDROID "Enable Android Build instead Desktop" ON)
option(SDL2_DESKTOP "Enable Android Build instead Desktop" ON)

include_directories(${PROJECT_NAME})

set(HEADERS ${PROJECT_NAME}/adlibemu.h
            ${PROJECT_NAME}/lab3d.h)

set(SOURCES ${PROJECT_NAME}/graphx.c
            ${PROJECT_NAME}/init.c
            ${PROJECT_NAME}/subs.c
            ${PROJECT_NAME}/oldlab3d.c
            ${PROJECT_NAME}/lab3d.c
            ${PROJECT_NAME}/adlibemu.c
            ${PROJECT_NAME}/setup.c)

if(ANDROID)
    add_definitions(-DANDROID_NDK -DOPENGLES -DUSE_SDL2 -DGL_GLEXT_PROTOTYPES)

    ## Glues
    set(GLUES "Glues")
    set(GLUES_HEADERS ${GLUES}/glues_error.h
                    ${GLUES}/glues.h
                    ${GLUES}/glues_mipmap.h
                    ${GLUES}/glues_project.h
                    ${GLUES}/glues_quad.h
                    ${GLUES}/glues_registry.h
                    ${GLUES}/glu.h)
    set(GLUES_SOURCES ${GLUES}/glues_error.c
                    ${GLUES}/glues_mipmap.c
                    ${GLUES}/glues_project.c
                    ${GLUES}/glues_quad.c
                    ${GLUES}/glues_registry.c)
    add_library(${GLUES} STATIC ${GLUES_SOURCES} ${GLUES_HEADERS})

    ## LodePNG
    set(LODEPNG "LodePNG")
    set(LODEPNG_HEADERS ${LODEPNG}/lodepng.h)
    set(LODEPNG_SOURCES ${LODEPNG}/lodepng.c)
    add_library(${LODEPNG} STATIC ${LODEPNG_SOURCES} ${LODEPNG_HEADERS})

    ## SDL2
    set(SDL2 "SDL2")
    set(SDL2_HEADERS ${SDL2}/src/events/scancodes_linux.h
                ${SDL2}/src/events/SDL_sysevents.h
                ${SDL2}/src/events/SDL_clipboardevents_c.h
                ${SDL2}/src/events/SDL_touch_c.h
                ${SDL2}/src/events/SDL_events_c.h
                ${SDL2}/src/events/blank_cursor.h
                ${SDL2}/src/events/SDL_dropevents_c.h
                ${SDL2}/src/events/SDL_gesture_c.h
                ${SDL2}/src/events/SDL_windowevents_c.h
                ${SDL2}/src/events/default_cursor.h
                ${SDL2}/src/events/SDL_keyboard_c.h
                ${SDL2}/src/events/SDL_mouse_c.h
                ${SDL2}/src/SDL_assert_c.h
                ${SDL2}/src/thread/SDL_thread_c.h
                ${SDL2}/src/thread/pthread/SDL_sysmutex_c.h
                ${SDL2}/src/thread/pthread/SDL_systhread_c.h
                ${SDL2}/src/thread/SDL_systhread.h
                ${SDL2}/src/dynapi/SDL_dynapi_procs.h
                ${SDL2}/src/dynapi/SDL_dynapi.h
                ${SDL2}/src/dynapi/SDL_dynapi_overrides.h
                ${SDL2}/src/SDL_internal.h
                ${SDL2}/src/video/SDL_sysvideo.h
                ${SDL2}/src/video/SDL_rect_c.h
                ${SDL2}/src/video/SDL_RLEaccel_c.h
                ${SDL2}/src/video/SDL_pixels_c.h
                ${SDL2}/src/video/SDL_shape_internals.h
                ${SDL2}/src/video/SDL_blit.h
                ${SDL2}/src/video/SDL_blit_slow.h
                ${SDL2}/src/video/SDL_blit_auto.h
                ${SDL2}/src/video/android/SDL_androidmessagebox.h
                ${SDL2}/src/video/android/SDL_androidwindow.h
                ${SDL2}/src/video/android/SDL_androidclipboard.h
                ${SDL2}/src/video/android/SDL_androidtouch.h
                ${SDL2}/src/video/android/SDL_androidevents.h
                ${SDL2}/src/video/android/SDL_androidvideo.h
                ${SDL2}/src/video/android/SDL_androidmouse.h
                ${SDL2}/src/video/android/SDL_androidkeyboard.h
                ${SDL2}/src/video/SDL_egl_c.h
                ${SDL2}/src/video/SDL_blit_copy.h
                ${SDL2}/src/libm/math_private.h
                ${SDL2}/src/libm/math_libm.h
                ${SDL2}/src/audio/SDL_wave.h
                ${SDL2}/src/audio/SDL_audio_c.h
                ${SDL2}/src/audio/SDL_audiodev_c.h
                ${SDL2}/src/audio/SDL_audiomem.h
                ${SDL2}/src/audio/android/SDL_androidaudio.h
                ${SDL2}/src/audio/SDL_sysaudio.h
                ${SDL2}/src/core/android/SDL_android.h
                ${SDL2}/src/timer/SDL_timer_c.h
                ${SDL2}/src/joystick/SDL_joystick_c.h
                ${SDL2}/src/joystick/SDL_sysjoystick.h
                ${SDL2}/src/joystick/SDL_gamecontrollerdb.h
                ${SDL2}/src/joystick/android/SDL_sysjoystick_c.h
                ${SDL2}/src/SDL_error_c.h
                ${SDL2}/src/render/SDL_sysrender.h
                ${SDL2}/src/render/SDL_yuv_sw_c.h
                ${SDL2}/src/render/software/SDL_blendpoint.h
                ${SDL2}/src/render/software/SDL_drawline.h
                ${SDL2}/src/render/software/SDL_blendfillrect.h
                ${SDL2}/src/render/software/SDL_render_sw_c.h
                ${SDL2}/src/render/software/SDL_rotate.h
                ${SDL2}/src/render/software/SDL_draw.h
                ${SDL2}/src/render/software/SDL_drawpoint.h
                ${SDL2}/src/render/software/SDL_blendline.h
                ${SDL2}/src/render/opengles/SDL_glesfuncs.h
                ${SDL2}/include/SDL_platform.h
                ${SDL2}/include/SDL_gesture.h
                ${SDL2}/include/close_code.h
                ${SDL2}/include/SDL_keycode.h
                ${SDL2}/include/SDL.h
                ${SDL2}/include/SDL_error.h
                ${SDL2}/include/SDL_revision.h
                ${SDL2}/include/SDL_filesystem.h
                ${SDL2}/include/SDL_surface.h
                ${SDL2}/include/SDL_audio.h
                ${SDL2}/include/SDL_types.h
                ${SDL2}/include/SDL_assert.h
                ${SDL2}/include/SDL_events.h
                ${SDL2}/include/SDL_stdinc.h
                ${SDL2}/include/SDL_messagebox.h
                ${SDL2}/include/SDL_loadso.h
                ${SDL2}/include/SDL_blendmode.h
                ${SDL2}/include/SDL_atomic.h
                ${SDL2}/include/SDL_opengles.h
                ${SDL2}/include/SDL_copying.h
                ${SDL2}/include/SDL_name.h
                ${SDL2}/include/SDL_rect.h
                ${SDL2}/include/begin_code.h
                ${SDL2}/include/SDL_cpuinfo.h
                ${SDL2}/include/SDL_touch.h
                ${SDL2}/include/SDL_mutex.h
                ${SDL2}/include/SDL_power.h
                ${SDL2}/include/SDL_pixels.h
                ${SDL2}/include/SDL_config.h
                ${SDL2}/include/SDL_scancode.h
                ${SDL2}/include/SDL_egl.h
                ${SDL2}/include/SDL_version.h
                ${SDL2}/include/SDL_config_android.h
                ${SDL2}/include/SDL_bits.h
                ${SDL2}/include/SDL_endian.h
                ${SDL2}/include/SDL_quit.h
                ${SDL2}/include/SDL_mouse.h
                ${SDL2}/include/SDL_hints.h
                ${SDL2}/include/SDL_log.h
                ${SDL2}/include/SDL_haptic.h
                ${SDL2}/include/SDL_main.h
                ${SDL2}/include/SDL_system.h
                ${SDL2}/include/SDL_syswm.h
                ${SDL2}/include/SDL_opengl_glext.h
                ${SDL2}/include/SDL_joystick.h
                ${SDL2}/include/SDL_render.h
                ${SDL2}/include/SDL_clipboard.h
                ${SDL2}/include/SDL_keyboard.h
                ${SDL2}/include/SDL_thread.h
                ${SDL2}/include/SDL_video.h
                ${SDL2}/include/SDL_gamecontroller.h
                ${SDL2}/include/SDL_opengl.h
                ${SDL2}/include/SDL_timer.h
                ${SDL2}/include/SDL_shape.h
                ${SDL2}/include/SDL_rwops.h)
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
    add_library(${SDL2} SHARED ${SDL2_SOURCES} ${SDL2_HEADERS})
    target_link_libraries(${SDL2} dl GLESv1_CM GLESv2 log android)

    include_directories(${INCLUDE_DIRECTORIES} ${GLUES} ${LODEPNG} ${SDL2}/include)
    set(HEADERS ${HEADERS} ${PROJECT_NAME}/sdl2keyhelper.h)
    set(SOURCES ${SOURCES} ${PROJECT_NAME}/sdl2keyhelper.c
            ${PROJECT_NAME}/androidutils.c
            ${SDL2}/src/main/android/SDL_android_main.c)
    add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})
    target_link_libraries(${PROJECT_NAME} ${SDL2} ${LODEPNG} ${GLUES} log GLESv1_CM)
else()
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
        set(HEADERS ${HEADERS} ${PROJECT_NAME}/sdl2keyhelper.h)
        set(SOURCES ${SOURCES} ${PROJECT_NAME}/sdl2keyhelper.c)
        add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS})
        target_link_libraries(${TARGET_NAME} ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARY} ${OPENGL_LIBRARIES} m)
    else()
        message(STATUS "Build w/ using SDL libraries!")
        set(TARGET_NAME "KenLab3D-SDL")
        find_package(OpenGL REQUIRED)
        find_package(SDL REQUIRED)
        find_package(SDL_image REQUIRED)
        add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS})
        target_link_libraries(${TARGET_NAME} ${SDL_LIBRARY} ${SDL_IMAGE_LIBRARY} ${OPENGL_LIBRARIES} m)
    endif()
    add_custom_target(finalMessage ALL
    	${CMAKE_COMMAND} -E cmake_echo_color --green "Output directory: ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}, executable: ${TARGET_NAME}."
    	COMMENT "Final Message")
    add_dependencies(finalMessage ${TARGET_NAME})
endif()
