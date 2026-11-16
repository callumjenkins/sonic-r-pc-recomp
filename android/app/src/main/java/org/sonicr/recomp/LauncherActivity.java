package org.sonicr.recomp;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.view.WindowInsets;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.io.File;
import java.io.InputStream;
import java.util.ArrayList;

public final class LauncherActivity extends Activity {
    private TextView status;
    private Button play;
    private Button importButton;
    private ControlSettingsView settings;
    private static volatile boolean importing;
    private static volatile String importFailure;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final Runnable pollImport = new Runnable() {
        @Override
        public void run() {
            refresh();
            if (importing) handler.postDelayed(this, 500);
        }
    };

    @Override
    public void onCreate(Bundle state) {
        super.onCreate(state);
        if (!importing) {
            try {
                GameDataImporter.recover(dataDirectory());
            } catch (java.io.IOException error) {
                importFailure = error.getMessage();
            }
        }
        LinearLayout layout = SettingsPages.page(this);
        TextView title = new TextView(this);
        title.setText("Sonic R");
        title.setTextSize(30);
        layout.addView(title);
        TextView instructions = new TextView(this);
        instructions.setText("Import your Sonic R game-data ZIP, then play with a controller or the on-screen controls. " +
            "The other pages hold controls, graphics, opponents, and experiments.");
        layout.addView(instructions);
        importButton = new Button(this);
        importButton.setText("Import game data");
        importButton.setOnClickListener(view -> {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("application/zip");
            startActivityForResult(intent, 1);
        });
        layout.addView(importButton);
        play = new Button(this);
        play.setText("Play");
        play.setOnClickListener(view -> {
            ArrayList<String> arguments = new ArrayList<>();
            if (settings.movementEnabled()) arguments.add("--modern-movement");
            if (settings.cameraEnabled()) arguments.add("--orbit-camera");
            arguments.add(dataDirectory().getAbsolutePath());
            Intent intent = new Intent(this, GameActivity.class);
            intent.putExtra("args", arguments.toArray(new String[0]));
            intent.putExtra("touch3d", settings.touch3d());
            intent.putExtra("touchVisible", settings.touchVisible());
            intent.putExtra("renderHeight", settings.renderHeight());
            intent.putExtra("smoothTextures", settings.smoothTextures());
            intent.putExtra("cpuEnabled", settings.cpuEnabled());
            intent.putExtra("cpuColours", settings.cpuColours());
            intent.putExtra("cpuCharacters", settings.cpuCharacters());
            intent.putExtra("wideIslandTiles", settings.wideIslandTiles());
            intent.putExtra("hdPack", settings.hdPack());
            startActivity(intent);
        });
        layout.addView(play);
        status = new TextView(this);
        layout.addView(status);
        settings = new ControlSettingsView(this, false, -1, "Home", layout);
        if (Build.VERSION.SDK_INT >= 30) {
            settings.setOnApplyWindowInsetsListener((view, insets) -> {
                android.graphics.Insets bars = insets.getInsets(
                    WindowInsets.Type.systemBars() | WindowInsets.Type.displayCutout());
                view.setPadding(bars.left, bars.top, bars.right, bars.bottom);
                return insets;
            });
        } else {
            settings.setFitsSystemWindows(true);
        }
        setContentView(settings);
        refresh();
    }

    private File dataDirectory() {
        return new File(getFilesDir(), "gamedata");
    }

    private void refresh() {
        boolean ready = GameDataImporter.isReady(dataDirectory());
        play.setEnabled(ready && !importing);
        importButton.setEnabled(!importing);
        status.setText(importing ? "Importing game data…" : importFailure != null ? importFailure :
            ready ? "Game data ready." : "Import game data to begin.");
    }

    @Override
    protected void onResume() {
        super.onResume();
        settings.refresh();
        handler.removeCallbacks(pollImport);
        handler.post(pollImport);
    }

    @Override
    protected void onPause() {
        handler.removeCallbacks(pollImport);
        super.onPause();
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request != 1 || result != RESULT_OK || data == null || data.getData() == null) return;
        if (importing) return;
        Uri uri = data.getData();
        importing = true;
        importFailure = null;
        refresh();
        handler.removeCallbacks(pollImport);
        handler.post(pollImport);
        new Thread(() -> {
            String failure = null;
            try (InputStream input = getContentResolver().openInputStream(uri)) {
                if (input == null) throw new java.io.IOException("Cannot open the selected ZIP.");
                GameDataImporter.importZip(input, dataDirectory());
            } catch (Exception error) {
                failure = error.getMessage();
                if (failure == null) failure = "The game data could not be imported.";
            }
            importFailure = failure;
            importing = false;
            runOnUiThread(this::refresh);
        }, "game-data-import").start();
    }
}
