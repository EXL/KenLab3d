LOCAL_PATH := $(call my-dir)

# Ken's Labyrinth Shared Library
include $(CLEAR_VARS)

DEF := -DANDROID_NDK -DOPENGLES -DUSE_SDL2

LOCAL_CFLAGS +=  -O3 -ffast-math -fomit-frame-pointer $(DEF)
LOCAL_CPPFLAGS += -O3 -frtti -ffast-math -fomit-frame-pointer $(DEF)

LOCAL_MODULE := KenLab3D

SDL_PATH := ../SDL2-2.0.3

LOCAL_C_INCLUDES :=	$(LOCAL_PATH)/$(SDL_PATH)/include \
					$(LOCAL_PATH)/KenLab3D \
					$(LOCAL_PATH)/3rdparty/Glues

# Add your application source files here...
LOCAL_SRC_FILES :=	$(SDL_PATH)/src/main/android/SDL_android_main.c \
					$(LOCAL_PATH)/KenLab3D/adlibemu.c \
					$(LOCAL_PATH)/KenLab3D/graphx.c \
					$(LOCAL_PATH)/KenLab3D/init.c \
					$(LOCAL_PATH)/KenLab3D/lab3d.c \
					$(LOCAL_PATH)/KenLab3D/oldlab3d.c \
					$(LOCAL_PATH)/KenLab3D/setup.c \
					$(LOCAL_PATH)/KenLab3D/subs.c \
					$(LOCAL_PATH)/KenLab3D/sdl2keyhelper.c

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_STATIC_LIBRARIES := Glues

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)

# Tess Static Library
include $(CLEAR_VARS)

DEF := -DANDROID_NDK
LOCAL_CFLAGS +=  -O3 -ffast-math -fomit-frame-pointer $(DEF)
LOCAL_CPPFLAGS += -O3 -frtti -ffast-math -fomit-frame-pointer $(DEF)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/3rdparty/Glues

LOCAL_MODULE    := Tess
LOCAL_SRC_FILES :=	$(LOCAL_PATH)/3rdparty/Tess/dict.c \
					$(LOCAL_PATH)/3rdparty/Tess/geom.c \
					$(LOCAL_PATH)/3rdparty/Tess/memalloc.c \
					$(LOCAL_PATH)/3rdparty/Tess/mesh.c \
					$(LOCAL_PATH)/3rdparty/Tess/normal.c \
					$(LOCAL_PATH)/3rdparty/Tess/priorityq.c \
					$(LOCAL_PATH)/3rdparty/Tess/render.c \
					$(LOCAL_PATH)/3rdparty/Tess/sweep.c \
					$(LOCAL_PATH)/3rdparty/Tess/tess.c \
					$(LOCAL_PATH)/3rdparty/Tess/tessmono.c

include $(BUILD_STATIC_LIBRARY)

# Glues Static Library
include $(CLEAR_VARS)

DEF := -DANDROID_NDK
LOCAL_CFLAGS +=  -O3 -ffast-math -fomit-frame-pointer $(DEF)
LOCAL_CPPFLAGS += -O3 -frtti -ffast-math -fomit-frame-pointer $(DEF)

LOCAL_MODULE    := Glues
LOCAL_SRC_FILES :=	$(LOCAL_PATH)/3rdparty/Glues/glues_error.c \
					$(LOCAL_PATH)/3rdparty/Glues/glues_mipmap.c \
					$(LOCAL_PATH)/3rdparty/Glues/glues_project.c \
					$(LOCAL_PATH)/3rdparty/Glues/glues_quad.c \
					$(LOCAL_PATH)/3rdparty/Glues/glues_registry.c

include $(BUILD_STATIC_LIBRARY)