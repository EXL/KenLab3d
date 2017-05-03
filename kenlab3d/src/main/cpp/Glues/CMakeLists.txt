cmake_minimum_required(VERSION 3.4.1)
project("Glues")

set(HEADERS glues_error.h
            glues.h
            glues_mipmap.h
            glues_project.h
            glues_quad.h
            glues_registry.h
            glu.h)

set(SOURCES glues_error.c
            glues_mipmap.c
            glues_project.c
            glues_quad.c
            glues_registry.c)

add_definitions(-DANDROID_NDK)
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
