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

package ru.exlmoto.kenlab3d;

import org.libsdl.app.SDLActivity;

import ru.exlmoto.kenlab3d.KenLab3DLauncherActivity.KenLab3DSettings;
import android.content.Context;
import android.os.Bundle;
import android.os.Vibrator;
import android.util.Log;
import android.view.ViewGroup.LayoutParams;
import android.widget.LinearLayout;

public class KenLab3DActivity extends SDLActivity {

	private static final String APP_TAG = "KenLab3D_App";

	private static Vibrator m_vibrator;

	// Access from JNI
	public static boolean m_hiResState;

	public static void toDebugLog(String debugMessage) {
		Log.d(APP_TAG, "=== " + debugMessage);
	}

	@SuppressWarnings("deprecation")
	@Override
	public void onCreate(Bundle savedInstanceState) {
		toDebugLog("Start SDL Activity from KenLab3DActivity");
		super.onCreate(savedInstanceState);

		toDebugLog("Setting Vibration");
		m_vibrator = (Vibrator)getSystemService(Context.VIBRATOR_SERVICE);
		
		if (KenLab3DSettings.s_TouchControls) {
			LinearLayout ll = new LinearLayout(this);
			if (KenLab3DLauncherActivity.g_isStateGame) {
				ll.setBackgroundDrawable(getResources().getDrawable(R.drawable.overlay_controls_game));
			} else {
				ll.setBackgroundDrawable(getResources().getDrawable(R.drawable.overlay_controls_settings));
			}
			addContentView(ll, new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT,
					LayoutParams.MATCH_PARENT));
		}
	}

	// JNI-method
	public static void doVibrate(int duration) {
		if (KenLab3DSettings.s_VibrationHaptics) {
			m_vibrator.vibrate(duration);
		}
	}
}
