LOCAL_PATH := $(call my-dir)

# Ken's Labyrinth Shared Library
include $(CLEAR_VARS)

DEF := -DANDROID_NDK -DOPENGLES -DUSE_SDL2

LOCAL_CFLAGS +=  -O3 -ffast-math -fomit-frame-pointer $(DEF)
LOCAL_CPPFLAGS += -O3 -frtti -ffast-math -fomit-frame-pointer $(DEF)

LOCAL_MODULE := KenLab3D

SDL_PATH := ../SDL2

LOCAL_C_INCLUDES :=	$(LOCAL_PATH)/$(SDL_PATH)/include \
					$(LOCAL_PATH)/../LodePNG \
					$(LOCAL_PATH)/../Glues \
					$(LOCAL_PATH)

# Add your application source files here...
LOCAL_SRC_FILES :=	$(SDL_PATH)/src/main/android/SDL_android_main.c \
					adlibemu.c \
					graphx.c \
					init.c \
					lab3d.c \
					oldlab3d.c \
					setup.c \
					subs.c \
					sdl2keyhelper.c \
					androidutils.c

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
LOCAL_SRC_FILES := ../LodePNG/lodepng.c

include $(BUILD_STATIC_LIBRARY)

# Glues Static Library
include $(CLEAR_VARS)

DEF := -DANDROID_NDK
LOCAL_CFLAGS +=  -O3 -ffast-math -fomit-frame-pointer $(DEF)
LOCAL_CPPFLAGS += -O3 -frtti -ffast-math -fomit-frame-pointer $(DEF)

LOCAL_MODULE    := Glues
LOCAL_SRC_FILES :=	../Glues/glues_error.c \
			../Glues/glues_mipmap.c \
			../Glues/glues_project.c \
			../Glues/glues_quad.c \
			../Glues/glues_registry.c

include $(BUILD_STATIC_LIBRARY)
