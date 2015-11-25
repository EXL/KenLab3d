package ru.exlmoto.kenlab3d;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;

public class KenLab3DLauncherActivity extends Activity  {

	// DEFAULT SETTINGS CLASS
	public static class KenLab3DSettings {
		public static boolean s_TouchControls = false;
		public static boolean s_VibrationHaptics = true;
		public static boolean s_HiResTextures = true;
	}
	// END DEFAULT SETTINGS CLASS

	private CheckBox checkBoxTouchControls;
	private CheckBox checkBoxVibrationHaptics;
	private CheckBox checkBoxHiResTextures;

	private Button buttonAbout;
	private Button buttonReconfigure;
	private Button buttonRunOrSetup;

	private void fillLayoutBySettings() {
		// TODO: fill

		checkBoxTouchControls.setChecked(KenLab3DSettings.s_TouchControls);
		checkBoxVibrationHaptics.setChecked(KenLab3DSettings.s_VibrationHaptics);
		checkBoxHiResTextures.setChecked(KenLab3DSettings.s_HiResTextures);
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.activity_kenlab3dlauncher);

		checkBoxTouchControls = (CheckBox)findViewById(R.id.checkBoxTouchControls);
		checkBoxVibrationHaptics = (CheckBox)findViewById(R.id.checkBoxVibrationHaptics);
		checkBoxHiResTextures = (CheckBox)findViewById(R.id.checkBoxHiresTextures);

		buttonAbout = (Button)findViewById(R.id.buttonAbout);
		buttonReconfigure = (Button)findViewById(R.id.buttonReconfigure);
		buttonRunOrSetup = (Button)findViewById(R.id.buttonRun);

		fillLayoutBySettings();

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
			}

		});

		checkBoxHiResTextures.setOnCheckedChangeListener(new OnCheckedChangeListener() {

			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				KenLab3DSettings.s_HiResTextures = isChecked;
			}

		});

		buttonAbout.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View buttonView) {
				// TODO Auto-generated method stub
			}

		});

		buttonReconfigure.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View buttonView) {
				// TODO Auto-generated method stub
			}

		});

		buttonRunOrSetup.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View buttonView) {
				// TODO: Write Settings
				Intent intent = new Intent(buttonView.getContext(), KenLab3DActivity.class);
				startActivity(intent);
			}

		});
	}

	@Override
	protected void onDestroy() {
		// TODO: Write Settings
		super.onDestroy();
	}

//	@Override
//	public void onBackPressed() {
//		// TODO: Write Settings
//		//System.exit(0);
//	}
}
