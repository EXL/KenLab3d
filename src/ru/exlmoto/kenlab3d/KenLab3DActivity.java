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
	public static int m_vibrateDelay;

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
			// if settings/run
			ll.setBackgroundDrawable(getResources().getDrawable(R.drawable.overlay_controls));
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
