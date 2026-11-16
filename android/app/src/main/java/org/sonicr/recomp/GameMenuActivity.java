package org.sonicr.recomp;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.KeyEvent;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

public final class GameMenuActivity extends Activity {
    private ControlSettingsView settings;
    private boolean returnToLauncher;

    @Override
    public void onCreate(Bundle state) {
        super.onCreate(state);
        LinearLayout layout = SettingsPages.page(this);
        TextView title = new TextView(this);
        title.setText("Game menu");
        title.setTextSize(26);
        layout.addView(title);
        TextView explanation = new TextView(this);
        explanation.setText("Game paused. Touch and graphics changes apply when you resume.");
        layout.addView(explanation);
        Button resume = new Button(this);
        resume.setText("Resume game");
        resume.setOnClickListener(view -> finish());
        layout.addView(resume);
        for (String key : new String[]{"graphicsStatus", "cpuStatus", "wideIslandStatus", "hdTexturesStatus"}) {
            TextView status = new TextView(this);
            status.setText(getIntent().getStringExtra(key));
            layout.addView(status);
        }
        TextView progress = new TextView(this);
        progress.setText("Save progress using the game's Options → Load/Save Data menu before leaving a session.");
        layout.addView(progress);
        Button launcher = new Button(this);
        launcher.setText("Return to launcher");
        launcher.setOnClickListener(view -> {
            returnToLauncher = true;
            finish();
        });
        layout.addView(launcher);
        settings = new ControlSettingsView(this, true, getIntent().getIntExtra("cpuAvailability", 0), "Game", layout);
        settings.setFitsSystemWindows(true);
        setContentView(settings);
        resume.requestFocus();
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (event.getKeyCode() == KeyEvent.KEYCODE_BUTTON_SELECT ||
            event.getKeyCode() == KeyEvent.KEYCODE_MENU) {
            if (event.getAction() == KeyEvent.ACTION_UP && !event.isCanceled()) finish();
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    public void finish() {
        Intent result = new Intent();
        result.putExtra("touchVisible", settings.touchVisible());
        result.putExtra("touch3d", settings.touch3d());
        result.putExtra("renderHeight", settings.renderHeight());
        result.putExtra("smoothTextures", settings.smoothTextures());
        result.putExtra("cpuEnabled", settings.cpuEnabled());
        result.putExtra("cpuColours", settings.cpuColours());
        result.putExtra("cpuCharacters", settings.cpuCharacters());
        result.putExtra("wideIslandTiles", settings.wideIslandTiles());
        result.putExtra("hdPack", settings.hdPack());
        result.putExtra("returnToLauncher", returnToLauncher);
        setResult(RESULT_OK, result);
        super.finish();
    }
}
