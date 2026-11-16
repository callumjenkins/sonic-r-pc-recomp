package org.sonicr.menutest;

import android.app.Instrumentation;
import android.app.UiAutomation;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.SystemClock;
import android.view.KeyEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.Map;

public final class AndroidMenuTest extends Instrumentation {
    private UiAutomation ui;
    private static final String[] OPTIONS = {"movement", "camera", "touchVisible", "touch3d", "renderHeight", "smoothTextures", "cpuEnabled", "cpuColours", "cpu0", "cpu1", "cpu2", "cpu3", "wideIsland", "wideIslandTiles", "hdTextures", "hdPack"};

    @Override
    public void onCreate(Bundle arguments) {
        super.onCreate(arguments);
        start();
    }

    @Override
    public void onStart() {
        ui = getUiAutomation();
        SharedPreferences preferences = getTargetContext().getSharedPreferences("controls", 0);
        Map<String, ?> original = preferences.getAll();
        Bundle result = new Bundle();
        int resultCode = 0;
        try {
            SharedPreferences.Editor initialize = preferences.edit();
            for (String key : OPTIONS) initialize.remove(key);
            require(initialize.commit(), "Initialize test preferences");
            Intent launcher = new Intent().setClassName("org.sonicr.recomp", "org.sonicr.recomp.LauncherActivity");
            launcher.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
            getTargetContext().startActivity(launcher);
            page("Controls");
            checked("Show touch controls", true);
            checked("Saturn 3D Pad layout", false);
            checked("Camera-relative movement", false);
            checked("Orbit camera", false);
            page("Opponents");
            checked("Custom CPU opponents", false);
            page("Experiments");
            checked("Wider Resort Island", false);
            absent("Choose strip width");
            checked("HD textures", false);
            absent("Choose HD pack");
            shot("pages-launcher");
            page("Home");
            click("Play");
            waitFor("Open game menu");
            SystemClock.sleep(8000);
            String session = gamePid();
            require(!session.isEmpty(), "Game process started");
            waitFor("Saturn digital touch pad");
            shot("digital");
            canceledKey(KeyEvent.KEYCODE_BACK);
            waitFor("Open game menu");
            absent("Game menu");
            require(gamePid().equals(session), "Canceled Back preserves the game");

            click("Open game menu");
            waitFor("Wider Resort Island: off");
            page("Opponents");
            checked("Custom CPU opponents", false);
            click("Custom CPU opponents");
            checked("Colour duplicate racers", false);
            waitFor("Random unlocked");
            click("Choose CPU 1");
            AccessibilityNodeInfo locked = waitFor("Super Sonic (locked)");
            require(!locked.isEnabled(), "Super Sonic remains locked on a fresh game");
            key(KeyEvent.KEYCODE_BACK);
            click("Choose CPU 1");
            click("Sonic");
            click("Choose CPU 2");
            click("Sonic");
            click("Colour duplicate racers");
            page("Game");
            click("Resume game");
            waitFor("Open game menu");
            click("Open game menu");
            page("Opponents");
            checked("Custom CPU opponents", true);
            checked("Colour duplicate racers", true);
            require(preferences.getInt("cpu0", -1) == 0 && preferences.getInt("cpu1", -1) == 0,
                "Duplicate CPU choices persist on resume");
            waitFor("Colour duplicate racers").performAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SHOW_ON_SCREEN.getId());
            SystemClock.sleep(400);
            shot("cpu-menu");
            click("Custom CPU opponents");
            page("Graphics");
            checked("Smooth textures", false);
            waitFor("Native display (default)");
            resolution("2160p / 4K height · 2880 × 2160");
            click("Smooth textures");
            page("Game");
            click("Resume game");
            waitFor("Open game menu");
            SystemClock.sleep(3000);
            shot("graphics-2160p");
            click("Open game menu");
            waitFor("Rendering 2880 x 2160; smooth textures");
            require(gamePid().equals(session), "2160p preserves the engine process");
            shot("graphics-menu");
            page("Graphics");
            resolution("720p · 960 × 720");
            page("Game");
            click("Resume game");
            waitFor("Open game menu");
            SystemClock.sleep(1000);
            click("Open game menu");
            waitFor("Rendering 960 x 720; smooth textures");
            page("Graphics");
            resolution("Native display (default)");
            click("Smooth textures");
            page("Game");
            click("Resume game");
            waitFor("Open game menu");
            SystemClock.sleep(1000);
            click("Open game menu");
            waitForNativeStatus();
            page("Graphics");
            resolution("720p · 960 × 720");
            click("Smooth textures");
            page("Game");
            click("Resume game");
            waitFor("Open game menu");

            key(KeyEvent.KEYCODE_BACK);
            waitFor("Game menu");
            require(gamePid().equals(session), "Back preserves the engine process");
            page("Controls");
            checked("Show touch controls", true);
            click("Show touch controls");
            page("Game");
            click("Resume game");
            waitFor("Open game menu");
            absent("Saturn digital touch pad");
            absent("Saturn 3D touch pad");
            shot("hidden");

            click("Open game menu");
            waitFor("Game menu");
            page("Controls");
            checked("Show touch controls", false);
            click("Show touch controls");
            click("Saturn 3D Pad layout");
            key(KeyEvent.KEYCODE_BACK);
            waitFor("Saturn 3D touch pad");
            require(gamePid().equals(session), "Layout changes preserve the engine process");
            shot("3d");

            key(KeyEvent.KEYCODE_BUTTON_SELECT);
            waitFor("Game menu");
            page("Controls");
            checked("Camera-relative movement", false);
            checked("Orbit camera", false);
            click("Show touch controls");
            page("Game");
            click("Resume game");
            waitFor("Open game menu");
            absent("Saturn 3D touch pad");
            key(KeyEvent.KEYCODE_MENU);
            waitFor("Game menu");
            page("Controls");
            checked("Show touch controls", false);
            click("Show touch controls");
            click("Saturn 3D Pad layout");
            key(KeyEvent.KEYCODE_BUTTON_SELECT);
            waitFor("Saturn digital touch pad");

            click("Open game menu");
            waitFor("Game menu");
            page("Controls");
            click("Camera-relative movement");
            click("Show touch controls");
            checked("Orbit camera", false);
            shot("menu");
            page("Game");
            click("Return to launcher");
            waitFor("Home");
            page("Controls");
            checked("Camera-relative movement", true);
            checked("Orbit camera", false);
            checked("Show touch controls", false);
            checked("Saturn 3D Pad layout", false);
            page("Graphics");
            checked("Smooth textures", true);
            waitFor("720p · 960 × 720");
            page("Opponents");
            checked("Custom CPU opponents", false);
            click("Custom CPU opponents");
            checked("Colour duplicate racers", true);
            require(preferences.getInt("cpu0", -1) == 0 && preferences.getInt("cpu1", -1) == 0,
                "CPU settings survive returning to the launcher");
            waitFor("Sonic");
            waitFor("Random unlocked");
            click("Custom CPU opponents");
            waitForGameExit();
            page("Home");
            click("Play");
            waitFor("Open game menu");
            SystemClock.sleep(3000);
            String relaunched = gamePid();
            require(!relaunched.isEmpty() && !relaunched.equals(session), "Relaunch starts a fresh engine process");
            absent("Saturn digital touch pad");
            key(KeyEvent.KEYCODE_BACK);
            waitFor("Game menu");
            waitFor("Rendering 960 x 720; smooth textures");
            page("Controls");
            checked("Show touch controls", false);
            checked("Camera-relative movement", true);
            click("Camera-relative movement");
            click("Orbit camera");
            click("Show touch controls");
            click("Saturn 3D Pad layout");
            page("Game");
            click("Return to launcher");
            waitFor("Home");
            page("Controls");
            checked("Camera-relative movement", false);
            checked("Orbit camera", true);
            checked("Show touch controls", true);
            checked("Saturn 3D Pad layout", true);
            waitForGameExit();
            page("Home");
            click("Play");
            waitFor("Saturn 3D touch pad");
            key(KeyEvent.KEYCODE_BACK);
            waitFor("Game menu");
            page("Controls");
            checked("Camera-relative movement", false);
            checked("Orbit camera", true);
            page("Game");
            click("Return to launcher");
            waitFor("Home");
            waitForGameExit();
            experiments(preferences);
            hdTextures(preferences);
            cpuRace(preferences);
            result.putString("stream", "Android menu checks passed\n");
            resultCode = -1;
        } catch (Throwable error) {
            try { shot("failure"); } catch (Exception ignored) {}
            result.putString("stream", error.toString() + "\n");
        } finally {
            SharedPreferences.Editor restore = preferences.edit();
            for (String key : OPTIONS) {
                if (original.get(key) instanceof Integer) restore.putInt(key, (Integer)original.get(key));
                else if (original.containsKey(key)) restore.putBoolean(key, (Boolean)original.get(key));
                else restore.remove(key);
            }
            if (!restore.commit()) {
                resultCode = 0;
                result.putString("stream", "Could not restore control preferences\n");
            }
        }
        finish(resultCode, result);
    }

    private void cpuRace(SharedPreferences preferences) throws Exception {
        SharedPreferences.Editor fixture = preferences.edit();
        fixture.putBoolean("cpuEnabled", true).putBoolean("cpuColours", true);
        fixture.putBoolean("movement", false).putBoolean("camera", false);
        fixture.putInt("renderHeight", 480).putBoolean("smoothTextures", false);
        for (int slot = 0; slot < 4; slot++) fixture.putInt("cpu" + slot, 0);
        require(fixture.commit(), "Prepare unlocked Sonic CPU roster");
        Intent launcher = new Intent().setClassName("org.sonicr.recomp", "org.sonicr.recomp.LauncherActivity");
        launcher.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        getTargetContext().startActivity(launcher);
        page("Opponents");
        waitChecked("Custom CPU opponents", true);
        waitChecked("Colour duplicate racers", true);
        page("Home");
        click("Play");
        waitFor("Open game menu");
        enterGrandPrix();
        shot("cpu-race");
        String session = gamePid();
        click("Open game menu");
        waitFor("CPU roster: Sonic, Sonic, Sonic, Sonic; clone colours");
        page("Opponents");
        for (int slot = 1; slot <= 4; slot++) {
            click("Choose CPU " + slot);
            click("Tails");
        }
        page("Game");
        click("Resume game");
        waitFor("Open game menu");
        click("Open game menu");
        waitFor("CPU roster: Sonic, Sonic, Sonic, Sonic; clone colours");
        click("Resume game");
        retryGrandPrix();
        click("Open game menu");
        waitFor("CPU roster: Tails, Tails, Tails, Tails; clone colours");
        require(gamePid().equals(session), "CPU roster changes on retry in the same engine process");
        page("Opponents");
        click("Custom CPU opponents");
        page("Game");
        click("Resume game");
        retryGrandPrix();
        click("Open game menu");
        waitFor("CPU roster: Original");
        require(gamePid().equals(session), "Disabling CPU mode restores the stock roster on retry");
        click("Return to launcher");
        waitFor("Home");
        waitForGameExit();
    }

    private void experiments(SharedPreferences preferences) throws Exception {
        page("Experiments");
        click("Wider Resort Island");
        waitFor("16 tiles · about 70 m (default)");
        require(preferences.getBoolean("wideIsland", false) && preferences.getInt("wideIslandTiles", 0) == 16,
            "Wider Resort Island defaults to 16 tiles");
        shot("experiments");
        try {
            require(ui.executeShellCommand("wm size 2400x1080") != null, "Widen the display");
            SystemClock.sleep(3000);
            waitFor("Strip width");
            shot("pages-wide");
        } finally {
            ui.executeShellCommand("wm size reset");
            SystemClock.sleep(2000);
        }
        page("Home");
        click("Play");
        waitFor("Open game menu");
        click("Open game menu");
        waitFor("Wider Resort Island: 16 tiles from the next Resort Island race");
        page("Experiments");
        click("Choose strip width");
        click("32 tiles · about 140 m");
        page("Game");
        click("Resume game");
        waitFor("Open game menu");
        click("Open game menu");
        waitFor("Wider Resort Island: 32 tiles from the next Resort Island race");
        shot("experiments-menu");
        click("Resume game");
        enterGrandPrix();
        shot("wide-race");
        click("Open game menu");
        waitFor("Wider Resort Island: 32 tiles on this course");
        page("Experiments");
        click("Wider Resort Island");
        page("Game");
        click("Resume game");
        waitFor("Open game menu");
        click("Open game menu");
        waitFor("Wider Resort Island: 32 tiles on this course, off from the next race");
        click("Return to launcher");
        waitFor("Home");
        waitForGameExit();
        page("Experiments");
        checked("Wider Resort Island", false);
    }

    private void hdTextures(SharedPreferences preferences) throws Exception {
        page("Experiments");
        click("HD textures");
        require(preferences.getBoolean("hdTextures", false), "HD textures switch persists");
        waitFor("Inverted");
        click("Choose HD pack");
        click("Nearest");
        require("Nearest".equals(preferences.getString("hdPack", "")), "HD pack choice persists");
        shot("hd-packs");
        page("Home");
        click("Play");
        waitFor("Open game menu");
        enterGrandPrix();
        shot("hd-race");
        click("Open game menu");
        // Resort Island's track, character, and ground pages are in the test packs, and the minimap's page is not.
        waitFor("HD pack Nearest, 11 pages replaced");
        page("Experiments");
        click("Choose HD pack");
        click("Inverted");
        page("Game");
        click("Resume game");
        waitFor("Open game menu");
        click("Open game menu");
        waitFor("HD pack Inverted from the next race, 11 pages of Nearest in use now");
        page("Experiments");
        click("HD textures");
        page("Game");
        click("Resume game");
        waitFor("Open game menu");
        click("Open game menu");
        waitFor("HD textures off from the next race, 11 pages replaced now");
        click("Return to launcher");
        waitFor("Home");
        waitForGameExit();
        page("Experiments");
        checked("HD textures", false);
    }

    private void page(String name) {
        // A click can reach the previous window while an activity is still replacing it.
        for (int attempt = 0; attempt < 3; attempt++) {
            click(name);
            if (waitFor(name).isSelected()) return;
            SystemClock.sleep(1000);
        }
        throw new AssertionError(name + " page selected");
    }

    private void enterGrandPrix() {
        SystemClock.sleep(10000);
        key(KeyEvent.KEYCODE_ENTER);
        SystemClock.sleep(2500);
        for (int step = 0; step < 4; step++) {
            key(KeyEvent.KEYCODE_SPACE);
            SystemClock.sleep(2500);
        }
        SystemClock.sleep(18000);
    }

    private void retryGrandPrix() {
        // Resume, Retry, and Quit are stacked. Down moves onto Retry, and Start confirms it.
        key(KeyEvent.KEYCODE_ENTER);
        key(KeyEvent.KEYCODE_DPAD_DOWN);
        key(KeyEvent.KEYCODE_ENTER);
        SystemClock.sleep(18000);
        waitFor("Open game menu");
    }

    private AccessibilityNodeInfo find(String label) {
        AccessibilityNodeInfo root = ui.getRootInActiveWindow();
        if (root == null) return null;
        return find(root, label);
    }

    private void resolution(String label) {
        click("Choose internal render resolution");
        click(label);
    }

    private void waitForNativeStatus() {
        long deadline = SystemClock.uptimeMillis() + 10000;
        do {
            AccessibilityNodeInfo root = ui.getRootInActiveWindow();
            if (root != null && nativeStatus(root)) return;
            SystemClock.sleep(100);
        } while (SystemClock.uptimeMillis() < deadline);
        throw new AssertionError("Native display with game texture filtering did not resume");
    }

    private boolean nativeStatus(AccessibilityNodeInfo node) {
        if (node.getText() != null && node.getText().toString().matches(
                "Rendering [0-9]+ x [0-9]+ \\(Native display\\); game texture filtering")) return true;
        for (int i = 0; i < node.getChildCount(); i++) {
            AccessibilityNodeInfo child = node.getChild(i);
            if (child != null && nativeStatus(child)) return true;
        }
        return false;
    }

    private AccessibilityNodeInfo find(AccessibilityNodeInfo node, String label) {
        if (label.equalsIgnoreCase(node.getText() == null ? "" : node.getText().toString()) ||
            label.contentEquals(node.getContentDescription() == null ? "" : node.getContentDescription())) return node;
        for (int i = 0; i < node.getChildCount(); i++) {
            AccessibilityNodeInfo child = node.getChild(i);
            if (child == null) continue;
            AccessibilityNodeInfo found = find(child, label);
            if (found != null) return found;
        }
        return null;
    }

    private AccessibilityNodeInfo waitFor(String label) {
        long deadline = SystemClock.uptimeMillis() + 10000;
        do {
            AccessibilityNodeInfo node = find(label);
            if (node != null) return node;
            AccessibilityNodeInfo root = ui.getRootInActiveWindow();
            if (root != null) scroll(root);
            SystemClock.sleep(100);
        } while (SystemClock.uptimeMillis() < deadline);
        throw new AssertionError("Missing UI: " + label);
    }

    private boolean scroll(AccessibilityNodeInfo node) {
        if (node.isScrollable() && node.performAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD)) return true;
        for (int i = 0; i < node.getChildCount(); i++) {
            AccessibilityNodeInfo child = node.getChild(i);
            if (child != null && scroll(child)) return true;
        }
        return false;
    }

    private void click(String label) {
        Bundle status = new Bundle();
        status.putString("stream", "Click " + label + "\n");
        sendStatus(0, status);
        AccessibilityNodeInfo node = waitFor(label);
        require(node.isEnabled() && node.performAction(AccessibilityNodeInfo.ACTION_CLICK), "Click " + label);
        SystemClock.sleep(400);
    }

    private void checked(String label, boolean expected) {
        require(waitFor(label).isChecked() == expected, label + " expected " + expected);
    }

    private void waitChecked(String label, boolean expected) {
        long deadline = SystemClock.uptimeMillis() + 10000;
        do {
            AccessibilityNodeInfo node = find(label);
            if (node != null && node.isChecked() == expected) return;
            SystemClock.sleep(100);
        } while (SystemClock.uptimeMillis() < deadline);
        throw new AssertionError(label + " expected " + expected);
    }

    private void absent(String label) {
        require(find(label) == null, "Unexpected UI: " + label);
    }

    private void key(int code) {
        long time = SystemClock.uptimeMillis();
        require(ui.injectInputEvent(new KeyEvent(time, time, KeyEvent.ACTION_DOWN, code, 0), true), "Key down");
        SystemClock.sleep(100);
        require(ui.injectInputEvent(new KeyEvent(time, SystemClock.uptimeMillis(), KeyEvent.ACTION_UP, code, 0), true), "Key up");
        SystemClock.sleep(500);
    }

    private void canceledKey(int code) {
        long time = SystemClock.uptimeMillis();
        require(ui.injectInputEvent(new KeyEvent(time, time, KeyEvent.ACTION_DOWN, code, 0), true), "Canceled key down");
        require(ui.injectInputEvent(new KeyEvent(time, SystemClock.uptimeMillis(), KeyEvent.ACTION_UP, code, 0, 0,
            -1, 0, KeyEvent.FLAG_CANCELED), true), "Canceled key up");
        SystemClock.sleep(500);
    }

    private String gamePid() throws Exception {
        try (var descriptor = ui.executeShellCommand("pidof org.sonicr.recomp:game");
             var input = new FileInputStream(descriptor.getFileDescriptor())) {
            return new String(input.readAllBytes(), StandardCharsets.UTF_8).trim();
        }
    }

    private void waitForGameExit() throws Exception {
        long deadline = SystemClock.uptimeMillis() + 10000;
        while (!gamePid().isEmpty() && SystemClock.uptimeMillis() < deadline) SystemClock.sleep(100);
        require(gamePid().isEmpty(), "Game process exits on Return to launcher");
    }

    private void shot(String name) throws Exception {
        Bitmap bitmap = ui.takeScreenshot();
        require(bitmap != null, "Screenshot available");
        try (var output = new FileOutputStream(new File(getTargetContext().getCacheDir(), "menu-" + name + ".png"))) {
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, output);
        }
        bitmap.recycle();
    }

    private static void require(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }
}
