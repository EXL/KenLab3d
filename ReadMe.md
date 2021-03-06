Ken's Labyrinth
=============

Ken's Labyrinth is a first-person shooter DOS game, released in 1993 by Epic MegaGames (now called Epic Games). It was mostly coded by Ken Silverman, who went on to design the Build engine that was used for rendering a first-person viewpoint in Apogee Software's Duke Nukem 3D. It consists of three episodes, the first of which was released as shareware - [Wikipedia](https://en.wikipedia.org/wiki/Ken%27s_Labyrinth).

![Ken's Labyrinth running on Motorola Droid 4](images/Ken_motorola_droid_4.jpg)

This is my port of Ken's Labyrinth on Android OS with using SDL2 library and rendering the videocontext of the game with using OpenGL ES. Added touch controls and some game engine improvements.

![Ken's Labyrinth Android Screen](images/Ken_android_screen.jpg)

[Ken's Labyrinth port on Motorola Droid 4 video on YouTube](http://youtu.be/vnNax1uNLfo)

## Download

You can download APK-package for Android OS from the [releases](https://github.com/EXL/KenLab3d/releases) section.

## Build instructions

For example, GNU/Linux:

* Install the latest [Android SDK](https://developer.android.com/sdk/) and [Android NDK](https://developer.android.com/ndk/);

* Clone repository into deploy directory;

```sh
cd ~/Deploy/
git clone https://github.com/EXL/KenLab3d KenLab3dAndroid
```

* Build the APK-package into deploy directory with Gradle building script;

```sh
cd ~/Deploy/KenLab3dAndroid/
ANDROID_HOME="/opt/android-sdk/" ./gradlew assembleDebug
```

* Install KenLab3D APK-package on your Android device via adb;

```sh
cd ~/Deploy/KenLab3dAndroid/
/opt/android-sdk/platform-tools/adb install -r kenlab3d/build/outputs/apk/kenlab3d-debug.apk
```

* Run and enjoy!

You can also open this project in Android Studio IDE and build the APK-package by using this program.

## More information

Please read [Porting Guide (In Russian)](http://exlmoto.ru/kenlab3d-droid) for more info about porting Ken's Labyrinth to Android OS.
