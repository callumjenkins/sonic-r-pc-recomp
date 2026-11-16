package org.sonicr.recomp;

import android.content.Context;
import android.content.SharedPreferences;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;
import java.io.File;
import java.util.List;

final class ExperimentsView extends LinearLayout {
    private static final int[] TILES = {8, 16, 32};
    private static final int DEFAULT_TILES = 16;
    private final SharedPreferences preferences;
    private final CheckBox wideIsland;
    private final Spinner width;
    private final CheckBox hdTextures;
    private final Spinner hdPack;
    private final TextView noPacks;
    private final File hd;
    private List<String> packs;

    ExperimentsView(Context context, SharedPreferences preferences) {
        super(context);
        this.preferences = preferences;
        setOrientation(VERTICAL);
        TextView intro = new TextView(context);
        intro.setText("Experiments change the game itself.");
        addView(intro);
        wideIsland = new CheckBox(context);
        wideIsland.setText("Wider Resort Island");
        addView(wideIsland);
        LinearLayout choices = new LinearLayout(context);
        choices.setOrientation(VERTICAL);
        addView(choices);
        TextView note = new TextView(context);
        note.setText("Cuts Resort Island down the middle and opens a flat strip between the halves. " +
            "Roads and walls that cross the cut stretch over it. Takes effect from the next Resort Island race. " +
            "Ghosts and replays are off in Resort Island races while it is on.");
        choices.addView(note);
        TextView label = new TextView(context);
        label.setText("Strip width");
        choices.addView(label);
        width = new Spinner(context);
        width.setContentDescription("Choose strip width");
        ArrayAdapter<String> adapter = new ArrayAdapter<>(context, android.R.layout.simple_spinner_item,
            new String[]{"8 tiles · about 35 m", "16 tiles · about 70 m (default)", "32 tiles · about 140 m"});
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        width.setAdapter(adapter);
        width.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                preferences.edit().putInt("wideIslandTiles", TILES[position]).apply();
            }
            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });
        choices.addView(width);
        wideIsland.setOnCheckedChangeListener((view, checked) -> {
            preferences.edit().putBoolean("wideIsland", checked).apply();
            choices.setVisibility(checked ? View.VISIBLE : View.GONE);
        });
        hd = new File(context.getFilesDir(), "gamedata/HD");
        hdTextures = new CheckBox(context);
        hdTextures.setText("HD textures");
        addView(hdTextures);
        LinearLayout hdChoices = new LinearLayout(context);
        hdChoices.setOrientation(VERTICAL);
        addView(hdChoices);
        TextView hdNote = new TextView(context);
        hdNote.setText("Draws texture pages from a pack in the HD folder of your game data. " +
            "Takes effect from the next race.");
        hdChoices.addView(hdNote);
        TextView packLabel = new TextView(context);
        packLabel.setText("Pack");
        hdChoices.addView(packLabel);
        hdPack = new Spinner(context);
        hdPack.setContentDescription("Choose HD pack");
        hdPack.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                String remembered = HdPacks.remembered(packs, preferences.getString("hdPack", ""), packs.get(position));
                preferences.edit().putString("hdPack", remembered).apply();
            }
            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });
        hdChoices.addView(hdPack);
        noPacks = new TextView(context);
        noPacks.setText("Your game data has no HD packs. Import a ZIP with packs in its HD folder.");
        hdChoices.addView(noPacks);
        hdTextures.setOnCheckedChangeListener((view, checked) -> {
            preferences.edit().putBoolean("hdTextures", checked).apply();
            hdChoices.setVisibility(checked ? View.VISIBLE : View.GONE);
        });
        refresh();
        choices.setVisibility(wideIsland.isChecked() ? View.VISIBLE : View.GONE);
        hdChoices.setVisibility(hdTextures.isChecked() ? View.VISIBLE : View.GONE);
    }

    void refresh() {
        wideIsland.setChecked(preferences.getBoolean("wideIsland", false));
        int tiles = preferences.getInt("wideIslandTiles", DEFAULT_TILES);
        int index = 1;
        for (int i = 0; i < TILES.length; i++) if (TILES[i] == tiles) index = i;
        width.setSelection(index);
        hdTextures.setChecked(preferences.getBoolean("hdTextures", false));
        packs = HdPacks.list(hd);
        ArrayAdapter<String> packAdapter = new ArrayAdapter<>(getContext(), android.R.layout.simple_spinner_item, packs);
        packAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        hdPack.setAdapter(packAdapter);
        if (!packs.isEmpty()) hdPack.setSelection(packs.indexOf(HdPacks.choose(packs, preferences.getString("hdPack", ""))));
        hdPack.setVisibility(packs.isEmpty() ? View.GONE : View.VISIBLE);
        noPacks.setVisibility(packs.isEmpty() ? View.VISIBLE : View.GONE);
    }

    /** Tiles for the engine, where 0 means off. */
    int wideIslandTiles() {
        return wideIsland.isChecked() ? TILES[width.getSelectedItemPosition()] : 0;
    }

    /** The pack for the engine, where "" means off. */
    String hdPack() {
        return hdTextures.isChecked() ? HdPacks.choose(packs, preferences.getString("hdPack", "")) : "";
    }
}
