package ru.exlmoto.kenlab3d;

import java.io.File;

import android.app.Activity;
import android.app.Dialog;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.Toast;

public class KenLab3DLauncherActivity extends Activity  {

	public static boolean g_isStateGame = false;

	// DEFAULT SETTINGS CLASS
	public static class KenLab3DSettings {
		public static boolean s_TouchControls = true;
		public static boolean s_VibrationHaptics = true;
		public static boolean s_HiResTextures = true;
		public static boolean s_Sound = true;
		public static boolean s_Music = true;

		// Access from JNI
		public static int s_VibroDelay = 50;
	}
	// END DEFAULT SETTINGS CLASS

	private CheckBox checkBoxTouchControls;
	private CheckBox checkBoxVibrationHaptics;
	private CheckBox checkBoxHiResTextures;
	private CheckBox checkBoxSound;
	private CheckBox checkBoxMusic;

	private Button buttonAbout;
	private Button buttonReconfigure;
	private Button buttonRunOrSetup;

	private EditText editVibrateDelay;

	private Dialog aboutDialog;
	private Dialog rangeDialog;
	private ImageView coverArt;

	private SharedPreferences settingsStorage = null;

	private File settingsIniFile = null;

	private void readSettings() {
		KenLab3DSettings.s_TouchControls = settingsStorage.getBoolean("s_TouchControls", true);
		KenLab3DSettings.s_VibrationHaptics = settingsStorage.getBoolean("s_VibrationHaptics", true);
		KenLab3DSettings.s_HiResTextures = settingsStorage.getBoolean("s_HiResTextures", true);
		KenLab3DSettings.s_Sound = settingsStorage.getBoolean("s_Sound", true);
		KenLab3DSettings.s_Music = settingsStorage.getBoolean("s_Music", true);
		KenLab3DSettings.s_VibroDelay = settingsStorage.getInt("s_VibroDelay", 50);
	}

	private void writeSettings() {
		KenLab3DActivity.toDebugLog("Write Settings!");

		fillSettingsByLayout();

		SharedPreferences.Editor editor = settingsStorage.edit();
		editor.putBoolean("s_TouchControls", KenLab3DSettings.s_TouchControls);
		editor.putBoolean("s_VibrationHaptics", KenLab3DSettings.s_VibrationHaptics);
		editor.putBoolean("s_HiResTextures", KenLab3DSettings.s_HiResTextures);
		editor.putBoolean("s_Sound", KenLab3DSettings.s_Sound);
		editor.putBoolean("s_Music", KenLab3DSettings.s_Music);
		if (KenLab3DSettings.s_VibroDelay < 30 || KenLab3DSettings.s_VibroDelay > 300) {
			editor.putInt("s_VibroDelay", 50);
		} else {
			editor.putInt("s_VibroDelay", KenLab3DSettings.s_VibroDelay);
		}
		editor.commit();
	}

	private void fillSettingsByLayout() {
		KenLab3DSettings.s_TouchControls = checkBoxTouchControls.isChecked();
		KenLab3DSettings.s_VibrationHaptics = checkBoxVibrationHaptics.isChecked();
		KenLab3DSettings.s_HiResTextures = checkBoxHiResTextures.isChecked();
		KenLab3DSettings.s_Sound = checkBoxSound.isChecked();
		if (!KenLab3DSettings.s_Sound) {
			KenLab3DSettings.s_Music = false;
		} else {
			KenLab3DSettings.s_Music = checkBoxMusic.isChecked();
		}

		String _toParse = editVibrateDelay.getText().toString();
		KenLab3DSettings.s_VibroDelay = _toParse.compareTo("") == 0 ? 50 : Integer.parseInt(_toParse);
	}

	private void fillLayoutBySettings() {
		checkBoxTouchControls.setChecked(KenLab3DSettings.s_TouchControls);
		checkBoxVibrationHaptics.setChecked(KenLab3DSettings.s_VibrationHaptics);
		checkBoxHiResTextures.setChecked(KenLab3DSettings.s_HiResTextures);
		checkBoxSound.setChecked(KenLab3DSettings.s_Sound);
		if (!KenLab3DSettings.s_Sound) {
			checkBoxMusic.setEnabled(false);
			checkBoxMusic.setChecked(false);
		} else {
			checkBoxMusic.setChecked(KenLab3DSettings.s_Music);
		}
		editVibrateDelay.setText(Integer.toString(KenLab3DSettings.s_VibroDelay));
		editVibrateDelay.setEnabled(KenLab3DSettings.s_VibrationHaptics);
	}

	private void updateRunOrSetupButton() {
		if (settingsIniFile.exists()) {
			buttonRunOrSetup.setText(R.string.buttonRunKen);
			g_isStateGame = true;
		} else {
			buttonRunOrSetup.setText(R.string.buttonRunSetup);
			g_isStateGame = false;
		}
	}

	private void showAboutDialog() {
		this.runOnUiThread(new Runnable() {

			@Override
			public void run() {
				aboutDialog.setContentView(R.layout.about_layout);
				aboutDialog.setCancelable(true);
				aboutDialog.setTitle(R.string.app_name);
				aboutDialog.show();
			}
		});
	}

	private void showErrorVibroRangeDialog() {
		this.runOnUiThread(new Runnable() {

			@Override
			public void run() {
				rangeDialog.setContentView(R.layout.range_error_layout);
				rangeDialog.setCancelable(true);
				rangeDialog.setTitle(R.string.errorString);
				rangeDialog.show();
			}
		});
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		KenLab3DActivity.toDebugLog("Start KenLab3DLauncher");

		aboutDialog = new Dialog(this);
		rangeDialog = new Dialog(this);

		settingsIniFile = new File(getFilesDir().getAbsolutePath() + "/settings.ini");

		settingsStorage = getSharedPreferences("ru.exlmoto.kenlab3d", MODE_PRIVATE);
		// Check the first run
		if (settingsStorage.getBoolean("firstrun", true)) {
			// The first run, fill GUI layout with default values
			settingsStorage.edit().putBoolean("firstrun", false).commit();
		} else {
			// Read settings from Shared Preferences
			readSettings();
		}

		setContentView(R.layout.activity_kenlab3dlauncher);

		checkBoxTouchControls = (CheckBox)findViewById(R.id.checkBoxTouchControls);
		checkBoxVibrationHaptics = (CheckBox)findViewById(R.id.checkBoxVibrationHaptics);
		checkBoxHiResTextures = (CheckBox)findViewById(R.id.checkBoxHiresTextures);
		checkBoxSound = (CheckBox)findViewById(R.id.checkBoxSound);
		checkBoxMusic = (CheckBox)findViewById(R.id.checkBoxMusic);

		buttonAbout = (Button)findViewById(R.id.buttonAbout);
		buttonReconfigure = (Button)findViewById(R.id.buttonReconfigure);
		buttonRunOrSetup = (Button)findViewById(R.id.buttonRun);

		editVibrateDelay = (EditText)findViewById(R.id.vibrateEdit);

		coverArt = (ImageView)findViewById(R.id.imageView);
		coverArt.setFocusable(true);
		coverArt.setFocusableInTouchMode(true);
		coverArt.requestFocus();

		fillLayoutBySettings();

		updateRunOrSetupButton();

		// Set Listeners
		checkBoxTouchControls.setOnCheckedChangeListener(new OnCheckedChangeListener() {

			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				KenLab3DSettings.s_TouchControls = isChecked;
			}

		});

		checkBoxVibrationHaptics.setOnCheckedChangeListener(new OnCheckedChangeListener() {

			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				KenLab3DSettings.s_VibrationHaptics = isChecked;
				editVibrateDelay.setEnabled(isChecked);
			}

		});

		checkBoxHiResTextures.setOnCheckedChangeListener(new OnCheckedChangeListener() {

			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				KenLab3DSettings.s_HiResTextures = isChecked;
			}

		});

		checkBoxSound.setOnCheckedChangeListener(new OnCheckedChangeListener() {

			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				KenLab3DSettings.s_Sound = isChecked;
				checkBoxMusic.setEnabled(isChecked);
				if (!isChecked && !checkBoxMusic.isEnabled()) {
					checkBoxMusic.setChecked(false);
				}
			}

		});

		checkBoxMusic.setOnCheckedChangeListener(new OnCheckedChangeListener() {

			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				KenLab3DSettings.s_Music = isChecked;
			}

		});

		buttonAbout.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View buttonView) {
				showAboutDialog();
			}

		});

		buttonReconfigure.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View buttonView) {
				Toast notificationTost;
				if (settingsIniFile.delete()) {
					notificationTost = Toast.makeText(getApplicationContext(), R.string.toastFileDeleted, Toast.LENGTH_LONG);
				} else {
					notificationTost = Toast.makeText(getApplicationContext(), R.string.toastFileNotFound, Toast.LENGTH_LONG);
				}
				updateRunOrSetupButton();

				notificationTost.show();
			}

		});

		buttonRunOrSetup.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View buttonView) {

				String _toParse = editVibrateDelay.getText().toString();
				int value = _toParse.compareTo("") == 0 ? 50 : Integer.parseInt(_toParse);

				if (value < 30 || value > 300) {
					showErrorVibroRangeDialog();
				} else {
					writeSettings();

					KenLab3DActivity.m_hiResState = KenLab3DSettings.s_HiResTextures;
					Intent intent = new Intent(buttonView.getContext(), KenLab3DActivity.class);
					startActivity(intent);
				}
			}

		});
	}

	@Override
	protected void onDestroy() {
		writeSettings();

		aboutDialog.dismiss();
		rangeDialog.dismiss();

		super.onDestroy();
	}
}
