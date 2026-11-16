package org.sonicr.recomp;

import android.content.Context;
import android.content.SharedPreferences;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;

final class CpuOpponentsView extends LinearLayout {
    private static final String[] CHARACTERS = {"Sonic", "Tails", "Knuckles", "Amy", "Eggman",
        "Metal Sonic", "Tails Doll", "Metal Knuckles", "Egg Robo", "Super Sonic"};
    private final SharedPreferences preferences;
    private final CheckBox enabled;
    private final CheckBox colours;
    private final Spinner[] characters = new Spinner[4];

    CpuOpponentsView(Context context, SharedPreferences preferences, int availability) {
        super(context);
        this.preferences = preferences;
        setOrientation(VERTICAL);
        enabled = new CheckBox(context);
        enabled.setText("Custom CPU opponents");
        addView(enabled);
        LinearLayout choices = new LinearLayout(context);
        choices.setOrientation(VERTICAL);
        addView(choices);
        TextView note = new TextView(context);
        note.setText("Applies to your next Grand Prix race. Choose four CPUs, with duplicates allowed. " +
            "Unlocks are checked in the loaded game. A locked saved choice uses Random unlocked.");
        choices.addView(note);
        for (int slot = 0; slot < characters.length; slot++) {
            final int index = slot;
            TextView label = new TextView(context);
            label.setText("CPU " + (slot + 1));
            choices.addView(label);
            Spinner spinner = new Spinner(context);
            spinner.setContentDescription("Choose CPU " + (slot + 1));
            String[] labels = new String[CHARACTERS.length + 1];
            labels[0] = "Random unlocked";
            for (int id = 0; id < CHARACTERS.length; id++)
                labels[id + 1] = CHARACTERS[id] +
                    (availability >= 0 && (availability & (1 << id)) == 0 ? " (locked)" : "");
            ArrayAdapter<String> adapter = new ArrayAdapter<String>(context,
                    android.R.layout.simple_spinner_item, labels) {
                @Override
                public boolean areAllItemsEnabled() { return false; }
                @Override
                public boolean isEnabled(int position) {
                    return position == 0 || availability < 0 || (availability & (1 << (position - 1))) != 0;
                }
                @Override
                public View getDropDownView(int position, View recycled, ViewGroup parent) {
                    View row = super.getDropDownView(position, recycled, parent);
                    row.setEnabled(isEnabled(position));
                    row.setAlpha(isEnabled(position) ? 1f : 0.45f);
                    return row;
                }
            };
            adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
            spinner.setAdapter(adapter);
            spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                    preferences.edit().putInt("cpu" + index, position - 1).apply();
                }
                @Override
                public void onNothingSelected(AdapterView<?> parent) {}
            });
            characters[slot] = spinner;
            choices.addView(spinner);
        }
        colours = new CheckBox(context);
        colours.setText("Colour duplicate racers");
        colours.setOnCheckedChangeListener((view, checked) ->
            preferences.edit().putBoolean("cpuColours", checked).apply());
        choices.addView(colours);
        enabled.setOnCheckedChangeListener((view, checked) -> {
            preferences.edit().putBoolean("cpuEnabled", checked).apply();
            choices.setVisibility(checked ? View.VISIBLE : View.GONE);
        });
        refresh();
        choices.setVisibility(enabled.isChecked() ? View.VISIBLE : View.GONE);
    }

    void refresh() {
        enabled.setChecked(preferences.getBoolean("cpuEnabled", false));
        colours.setChecked(preferences.getBoolean("cpuColours", false));
        for (int slot = 0; slot < characters.length; slot++) {
            int character = preferences.getInt("cpu" + slot, -1);
            characters[slot].setSelection(character >= -1 && character < CHARACTERS.length ? character + 1 : 0);
        }
    }

    boolean enabled() { return enabled.isChecked(); }
    boolean colours() { return colours.isChecked(); }
    int[] characters() {
        int[] result = new int[characters.length];
        for (int slot = 0; slot < characters.length; slot++)
            result[slot] = characters[slot].getSelectedItemPosition() - 1;
        return result;
    }
}
