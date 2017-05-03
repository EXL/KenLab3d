LOCAL_PATH := $(call my-dir)

# Ken's Labyrinth Shared Library
include $(CLEAR_VARS)

DEF := -DANDROID_NDK -DOPENGLES -DUSE_SDL2

LOCAL_CFLAGS +=  -O3 -ffast-math -fomit-frame-pointer $(DEF)
LOCAL_CPPFLAGS += -O3 -frtti -ffast-math -fomit-frame-pointer $(DEF)

LOCAL_MODULE := KenLab3D

SDL_PATH := ../SDL2-2.0.4-compact

LOCAL_C_INCLUDES :=	$(LOCAL_PATH)/$(SDL_PATH)/include \
					$(LOCAL_PATH)/3rdparty/LodePNG \
					$(LOCAL_PATH)/KenLab3D \
					$(LOCAL_PATH)/3rdparty/Glues

# Add your application source files here...
LOCAL_SRC_FILES :=	$(SDL_PATH)/src/main/android/SDL_android_main.c \
					KenLab3D/adlibemu.c \
					KenLab3D/graphx.c \
					KenLab3D/init.c \
					KenLab3D/lab3d.c \
					KenLab3D/oldlab3d.c \
					KenLab3D/setup.c \
					KenLab3D/subs.c \
					KenLab3D/sdl2keyhelper.c \
					KenLab3D/androidutils.c

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_STATIC_LIBRARIES := Glues LodePNG

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)

# LodePNG Static Library
include $(CLEAR_VARS)

DEF := -DANDROID_NDK
LOCAL_CFLAGS +=  -O3 -ffast-math -fomit-frame-pointer $(DEF)
LOCAL_CPPFLAGS += -O3 -frtti -ffast-math -fomit-frame-pointer $(DEF)

LOCAL_MODULE    := LodePNG
LOCAL_SRC_FILES :=	3rdparty/LodePNG/lodepng.c

include $(BUILD_STATIC_LIBRARY)

# Glues Static Library
include $(CLEAR_VARS)

DEF := -DANDROID_NDK
LOCAL_CFLAGS +=  -O3 -ffast-math -fomit-frame-pointer $(DEF)
LOCAL_CPPFLAGS += -O3 -frtti -ffast-math -fomit-frame-pointer $(DEF)

LOCAL_MODULE    := Glues
LOCAL_SRC_FILES :=	3rdparty/Glues/glues_error.c \
					3rdparty/Glues/glues_mipmap.c \
					3rdparty/Glues/glues_project.c \
					3rdparty/Glues/glues_quad.c \
					3rdparty/Glues/glues_registry.c

include $(BUILD_STATIC_LIBRARY)