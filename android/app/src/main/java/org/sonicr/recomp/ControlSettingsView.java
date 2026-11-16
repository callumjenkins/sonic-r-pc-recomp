package org.sonicr.recomp;

import android.content.Context;
import android.content.SharedPreferences;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Spinner;
import android.widget.ArrayAdapter;
import android.widget.AdapterView;
import android.view.View;

final class ControlSettingsView extends SettingsPages {
    private final SharedPreferences preferences;
    private final CheckBox movement;
    private final CheckBox camera;
    private final CheckBox touchVisible;
    private final CheckBox touch3d;
    private final CheckBox smoothTextures;
    private final Spinner resolution;
    private final CpuOpponentsView cpu;
    private final ExperimentsView experiments;
    private static final int[] HEIGHTS = {0, 480, 720, 1080, 1440, 2160};

    ControlSettingsView(Context context, boolean inGame, int cpuAvailability, String firstTitle, View firstPage) {
        super(context);
        preferences = context.getSharedPreferences("controls", Context.MODE_PRIVATE);
        addPage(firstTitle, firstPage);

        LinearLayout controls = page(context);
        touchVisible = option(controls, "Show touch controls", "touchVisible", true);
        touch3d = option(controls, "Saturn 3D Pad layout", "touch3d", false);
        if (inGame) note(controls, "Movement and camera changes apply when you next launch a game.");
        movement = option(controls, "Camera-relative movement", "movement", false);
        camera = option(controls, "Orbit camera", "camera", false);
        note(controls, "Camera-relative movement and orbit currently support Sonic in single-player. Leave those two off " +
            "for classic controls. The 3D Pad layout adds movement and camera sticks, a D-pad, six face buttons, and " +
            "shoulder buttons. Touch controls require the default keyboard bindings.");
        addPage("Controls", controls);

        LinearLayout graphics = page(context);
        note(graphics, "Internal render resolution");
        resolution = new Spinner(context);
        resolution.setContentDescription("Choose internal render resolution");
        ArrayAdapter<String> choices = new ArrayAdapter<>(context, android.R.layout.simple_spinner_item,
            new String[]{"Native display (default)", "480p · 640 × 480", "720p · 960 × 720",
                "1080p · 1440 × 1080", "1440p · 1920 × 1440", "2160p / 4K height · 2880 × 2160"});
        choices.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        resolution.setAdapter(choices);
        resolution.setSelection(resolutionIndex());
        resolution.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                preferences.edit().putInt("renderHeight", HEIGHTS[position]).apply();
            }
            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });
        graphics.addView(resolution);
        smoothTextures = option(graphics, "Smooth textures", "smoothTextures", false);
        note(graphics, (inGame ? "Graphics changes apply when you resume. " : "") +
            "Native already renders at screen resolution. 2160p supersamples on smaller screens and uses more GPU " +
            "power. Smoothing also softens HUD sprites and may show colored edges.");
        addPage("Graphics", graphics);

        LinearLayout opponents = page(context);
        cpu = new CpuOpponentsView(context, preferences, cpuAvailability);
        opponents.addView(cpu);
        addPage("Opponents", opponents);

        LinearLayout experimentsPage = page(context);
        experiments = new ExperimentsView(context, preferences);
        experimentsPage.addView(experiments);
        addPage("Experiments", experimentsPage);
    }

    private CheckBox option(LinearLayout page, String label, String key, boolean initial) {
        CheckBox box = new CheckBox(getContext());
        box.setText(label);
        box.setChecked(preferences.getBoolean(key, initial));
        box.setOnCheckedChangeListener((view, checked) ->
            preferences.edit().putBoolean(key, checked).apply());
        page.addView(box);
        return box;
    }

    private void note(LinearLayout page, String text) {
        TextView view = new TextView(getContext());
        view.setText(text);
        page.addView(view);
    }

    void refresh() {
        cpu.refresh();
        experiments.refresh();
        movement.setChecked(preferences.getBoolean("movement", false));
        camera.setChecked(preferences.getBoolean("camera", false));
        touchVisible.setChecked(preferences.getBoolean("touchVisible", true));
        touch3d.setChecked(preferences.getBoolean("touch3d", false));
        smoothTextures.setChecked(preferences.getBoolean("smoothTextures", false));
        resolution.setSelection(resolutionIndex());
    }

    private int resolutionIndex() {
        int height = preferences.getInt("renderHeight", 0);
        for (int i = 0; i < HEIGHTS.length; i++) if (HEIGHTS[i] == height) return i;
        return 0;
    }

    boolean cpuEnabled() { return cpu.enabled(); }
    boolean cpuColours() { return cpu.colours(); }
    int[] cpuCharacters() { return cpu.characters(); }
    int wideIslandTiles() { return experiments.wideIslandTiles(); }
    String hdPack() { return experiments.hdPack(); }

    boolean movementEnabled() { return movement.isChecked(); }
    boolean cameraEnabled() { return camera.isChecked(); }
    boolean touchVisible() { return touchVisible.isChecked(); }
    boolean touch3d() { return touch3d.isChecked(); }
    int renderHeight() { return HEIGHTS[resolution.getSelectedItemPosition()]; }
    boolean smoothTextures() { return smoothTextures.isChecked(); }
}
