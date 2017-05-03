/************************************************************************************
** The MIT License (MIT)
**
** Copyright (c) 2015-2016 Serg "EXL" Koles
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
************************************************************************************/

/*
 * androidutils.c
 *
 *  Created on: May 24, 2016
 *      Author: exl
 */

#include <jni.h> // For JNI

#include "lab3d.h"

void copyFileFromAssetsToInternalRWDirAndroid(const char *source_file, const char *desc1, const char *desc2) {
	char path1[256];
	char path2[256];
	snprintf(path1, sizeof(path1), "%s/%s", globalDataDir, source_file);
	snprintf(path2, sizeof(path2), "%s/%s", globalAndroidRWdir, source_file);

	SDL_RWops *io_in = SDL_RWFromFile(path1, desc1);
	if (!io_in) {
		TO_DEBUG_LOG("cFFATIRWDA: Error opening in file: %s.\n", path1);
	}

	SDL_RWops *io_out = SDL_RWFromFile(path2, desc1);
	if (io_out) {
		TO_DEBUG_LOG("cFFATIRWDA: File: %s found.\n", source_file);
		SDL_FreeRW(io_in);
		SDL_FreeRW(io_out);
		return;
	} else {
		TO_DEBUG_LOG("cFFATIRWDA: Copying file %s.\n", source_file);
	}
	SDL_FreeRW(io_out);

	io_out = SDL_RWFromFile(path2, desc2);
	if (!io_out) {
		TO_DEBUG_LOG("cFFATIRWDA: Error opening out file: %s.\n", path2);
	}

	long int size = SDL_RWsize(io_in);
	char *in = (char *)malloc(size);

	SDL_RWread(io_in, in, size, 1);
	SDL_RWwrite(io_out, in, size, 1);

	SDL_FreeRW(io_in);
	SDL_FreeRW(io_out);

	free(in);
}

int getVibarateDelayFromJNI() {
	JNIEnv *javaEnviron = SDL_AndroidGetJNIEnv();
	if (javaEnviron != NULL) {
		jclass clazz = (*javaEnviron)->FindClass(javaEnviron, "ru/exlmoto/kenlab3d/KenLab3DLauncherActivity$KenLab3DSettings");
		if (clazz == 0) {
			TO_DEBUG_LOG("Error JNI: Class ru/exlmoto/kenlab3d/KenLab3DLauncherActivity$KenLab3DSettings not found!");
			return 0;
		}

		jfieldID fieldID = (*javaEnviron)->GetStaticFieldID(javaEnviron, clazz, "s_VibroDelay", "I");
		if (fieldID == 0) {
			TO_DEBUG_LOG("Error JNI: fieldID is 0, field s_VibroDelay I not found!");
			return 0;
		}

		jint vibrateDelay = (*javaEnviron)->GetStaticIntField(javaEnviron, clazz, fieldID);
		TO_DEBUG_LOG("JNI: s_VibroDelay is: %d", (int)vibrateDelay);

		(*javaEnviron)->DeleteLocalRef(javaEnviron, clazz);

		return (int)vibrateDelay;
	}
	return 0;
}

void doVibrateFromJNI(int duration) {
	JNIEnv *javaEnviron = SDL_AndroidGetJNIEnv();
	if (javaEnviron != NULL) {
		jclass clazz = (*javaEnviron)->FindClass(javaEnviron, "ru/exlmoto/kenlab3d/KenLab3DActivity");
		if (clazz == 0) {
			TO_DEBUG_LOG("Error JNI: Class ru/exlmoto/kenlab3d/KenLab3DActivity not found!");
			return;
		}

		jmethodID methodId = (*javaEnviron)->GetStaticMethodID(javaEnviron, clazz, "doVibrate", "(I)V");
		if (methodId == 0) {
			LOGI("Error JNI: methodId is 0, method doVibrate (I)V not found!");
			return;
		}

		// Call Java-method
		(*javaEnviron)->CallStaticVoidMethod(javaEnviron, clazz, methodId, (jint)duration);
		(*javaEnviron)->DeleteLocalRef(javaEnviron, clazz);
	}
}

int getHiresSettingsValue() {
	JNIEnv *javaEnviron = SDL_AndroidGetJNIEnv();
	if (javaEnviron != NULL) {
		jclass clazz = (*javaEnviron)->FindClass(javaEnviron, "ru/exlmoto/kenlab3d/KenLab3DActivity");
		if (clazz == 0) {
			TO_DEBUG_LOG("Error JNI: Class ru/exlmoto/kenlab3d/KenLab3DActivity not found!");
			return 0;
		}

		jfieldID fieldID = (*javaEnviron)->GetStaticFieldID(javaEnviron, clazz, "m_hiResState", "Z");
		if (fieldID == 0) {
			TO_DEBUG_LOG("Error JNI: fieldID is 0, field m_hiResState Z not found!");
			return 0;
		}

		jboolean hiresState = (*javaEnviron)->GetStaticBooleanField(javaEnviron, clazz, fieldID);
		TO_DEBUG_LOG("JNI: m_hiResState is: %d", (int)hiresState);

		(*javaEnviron)->DeleteLocalRef(javaEnviron, clazz);

		return (int)hiresState;
	}
	return 0;
}

int getSoundSettingsValue() {
	JNIEnv *javaEnviron = SDL_AndroidGetJNIEnv();
	if (javaEnviron != NULL) {
		jclass clazz = (*javaEnviron)->FindClass(javaEnviron, "ru/exlmoto/kenlab3d/KenLab3DLauncherActivity$KenLab3DSettings");
		if (clazz == 0) {
			TO_DEBUG_LOG("Error JNI: Class ru/exlmoto/kenlab3d/KenLab3DLauncherActivity$KenLab3DSettings not found!");
			return 0;
		}

		jfieldID fieldID = (*javaEnviron)->GetStaticFieldID(javaEnviron, clazz, "s_Sound", "Z");
		if (fieldID == 0) {
			TO_DEBUG_LOG("Error JNI: fieldID is 0, field s_Sound Z not found!");
			return 0;
		}

		jboolean sound = (*javaEnviron)->GetStaticBooleanField(javaEnviron, clazz, fieldID);
		TO_DEBUG_LOG("JNI: s_Sound is: %d", (int)sound);

		(*javaEnviron)->DeleteLocalRef(javaEnviron, clazz);

		return (int)sound;
	}
	return 0;
}

int getMusicSettingsValue() {
	JNIEnv *javaEnviron = SDL_AndroidGetJNIEnv();
	if (javaEnviron != NULL) {
		jclass clazz = (*javaEnviron)->FindClass(javaEnviron, "ru/exlmoto/kenlab3d/KenLab3DLauncherActivity$KenLab3DSettings");
		if (clazz == 0) {
			TO_DEBUG_LOG("Error JNI: Class ru/exlmoto/kenlab3d/KenLab3DLauncherActivity$KenLab3DSettings not found!");
			return 0;
		}

		jfieldID fieldID = (*javaEnviron)->GetStaticFieldID(javaEnviron, clazz, "s_Music", "Z");
		if (fieldID == 0) {
			TO_DEBUG_LOG("Error JNI: fieldID is 0, field s_Music Z not found!");
			return 0;
		}

		jboolean music = (*javaEnviron)->GetStaticBooleanField(javaEnviron, clazz, fieldID);
		TO_DEBUG_LOG("JNI: s_Music is: %d", (int)music);

		(*javaEnviron)->DeleteLocalRef(javaEnviron, clazz);

		return (int)music;
	}
	return 0;
}
