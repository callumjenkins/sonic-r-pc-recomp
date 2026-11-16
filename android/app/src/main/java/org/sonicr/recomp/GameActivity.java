package org.sonicr.recomp;

import android.content.Intent;
import android.os.Bundle;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.widget.ImageButton;
import android.widget.FrameLayout;
import org.libsdl.app.SDLActivity;
import java.util.HashSet;
import java.util.Set;

public final class GameActivity extends SDLActivity {
    private final Set<Integer> pressed = new HashSet<>();
    private TouchPadView touchPad;
    private FrameLayout controls;
    private boolean touchVisible;
    private boolean touch3d;
    private boolean menuOpen;
    private static final int MENU_REQUEST = 2;
    private static native void nativeTouchAxes(float moveX, float moveY, float cameraX, float cameraY);
    private static native void nativeGraphics(int height, boolean smooth);
    private static native String nativeGraphicsStatus();
    private static native void nativeCpuOpponents(boolean enabled, boolean colours, int[] characters);
    private static native int nativeCpuAvailability();
    private static native String nativeCpuStatus();
    private static native void nativeWideIsland(int tiles);
    private static native String nativeWideIslandStatus();
    private static native void nativeHdTextures(String pack);
    private static native String nativeHdTexturesStatus();

    private static final int[] TOUCH_BITS = {
        TouchPadState.UP, TouchPadState.DOWN, TouchPadState.LEFT, TouchPadState.RIGHT,
        TouchPadState.JUMP, TouchPadState.RUN, TouchPadState.LOOK_BACK,
        TouchPadState.CAMERA_LEFT, TouchPadState.RECENTER, TouchPadState.CAMERA_RIGHT,
        TouchPadState.DRIFT_LEFT, TouchPadState.DRIFT_RIGHT, TouchPadState.START
    };
    private static final int[] TOUCH_KEYS = {
        KeyEvent.KEYCODE_DPAD_UP, KeyEvent.KEYCODE_DPAD_DOWN, KeyEvent.KEYCODE_DPAD_LEFT, KeyEvent.KEYCODE_DPAD_RIGHT,
        KeyEvent.KEYCODE_SPACE, KeyEvent.KEYCODE_A, KeyEvent.KEYCODE_1,
        KeyEvent.KEYCODE_Q, KeyEvent.KEYCODE_R, KeyEvent.KEYCODE_E,
        KeyEvent.KEYCODE_Z, KeyEvent.KEYCODE_X, KeyEvent.KEYCODE_ENTER
    };

    @Override
    protected String[] getLibraries() {
        return new String[]{"SDL2", "SDL2_mixer", "main"};
    }

    @Override
    protected String[] getArguments() {
        String[] arguments = getIntent().getStringArrayExtra("args");
        return arguments == null ? new String[0] : arguments;
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (event.getKeyCode() == KeyEvent.KEYCODE_BACK ||
            event.getKeyCode() == KeyEvent.KEYCODE_MENU ||
            event.getKeyCode() == KeyEvent.KEYCODE_BUTTON_SELECT) {
            if (event.getAction() == KeyEvent.ACTION_UP && !event.isCanceled()) openMenu();
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    public void onCreate(Bundle state) {
        super.onCreate(state);
        if (mBrokenLibraries || mLayout == null) return;
        nativeGraphics(getIntent().getIntExtra("renderHeight", 0), getIntent().getBooleanExtra("smoothTextures", false));
        nativeCpuOpponents(getIntent().getBooleanExtra("cpuEnabled", false),
            getIntent().getBooleanExtra("cpuColours", false), getIntent().getIntArrayExtra("cpuCharacters"));
        nativeWideIsland(getIntent().getIntExtra("wideIslandTiles", 0));
        nativeHdTextures(getIntent().getStringExtra("hdPack"));
        FrameLayout overlay = new FrameLayout(this);
        mLayout.addView(overlay, new android.widget.RelativeLayout.LayoutParams(-1, -1));
        controls = new FrameLayout(this);
        overlay.addView(controls, new FrameLayout.LayoutParams(-1, -1));
        touchVisible = getIntent().getBooleanExtra("touchVisible", true);
        touch3d = getIntent().getBooleanExtra("touch3d", false);
        rebuildControls();
        ImageButton menu = new ImageButton(this);
        menu.setImageResource(R.drawable.ic_menu);
        menu.setBackgroundResource(R.drawable.menu_button_background);
        menu.setPadding(dp(12), dp(12), dp(12), dp(12));
        menu.setTooltipText("Open game menu");
        menu.setContentDescription("Open game menu");
        menu.setOnClickListener(view -> openMenu());
        FrameLayout.LayoutParams menuParams = new FrameLayout.LayoutParams(dp(48), dp(48), Gravity.TOP | Gravity.RIGHT);
        menuParams.setMargins(dp(8), dp(8), dp(8), 0);
        overlay.addView(menu, menuParams);
    }

    @Override
    public void onBackPressed() {
        openMenu();
    }

    private void openMenu() {
        if (menuOpen || mBrokenLibraries || controls == null) return;
        releaseTouch();
        menuOpen = true;
        Intent menu = new Intent(this, GameMenuActivity.class);
        menu.putExtra("graphicsStatus", nativeGraphicsStatus());
        menu.putExtra("cpuStatus", nativeCpuStatus());
        menu.putExtra("cpuAvailability", nativeCpuAvailability());
        menu.putExtra("wideIslandStatus", nativeWideIslandStatus());
        menu.putExtra("hdTexturesStatus", nativeHdTexturesStatus());
        startActivityForResult(menu, MENU_REQUEST);
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request != MENU_REQUEST) return;
        menuOpen = false;
        if (result != RESULT_OK || data == null) return;
        if (data.getBooleanExtra("returnToLauncher", false)) {
            finish();
            return;
        }
        touchVisible = data.getBooleanExtra("touchVisible", touchVisible);
        touch3d = data.getBooleanExtra("touch3d", touch3d);
        nativeGraphics(data.getIntExtra("renderHeight", 0), data.getBooleanExtra("smoothTextures", false));
        nativeCpuOpponents(data.getBooleanExtra("cpuEnabled", false),
            data.getBooleanExtra("cpuColours", false), data.getIntArrayExtra("cpuCharacters"));
        nativeWideIsland(data.getIntExtra("wideIslandTiles", 0));
        nativeHdTextures(data.getStringExtra("hdPack"));
        rebuildControls();
    }

    private void rebuildControls() {
        releaseTouch();
        touchPad = null;
        controls.removeAllViews();
        controls.setVisibility(touchVisible ? View.VISIBLE : View.GONE);
        if (!touchVisible) return;
        touchPad = new TouchPadView(this, this::touchInput, !touch3d);
        controls.addView(touchPad, new FrameLayout.LayoutParams(-1, -1));
    }

    private void touchInput(int buttons, float moveX, float moveY, float cameraX, float cameraY) {
        nativeTouchAxes(moveX, moveY, cameraX, cameraY);
        for (int i = 0; i < TOUCH_BITS.length; i++) {
            int key = TOUCH_KEYS[i];
            boolean held = (buttons & TOUCH_BITS[i]) != 0;
            if (held && pressed.add(key)) onNativeKeyDown(key);
            else if (!held && pressed.remove(key)) onNativeKeyUp(key);
        }
    }

    private int dp(int value) {
        return (int)(value * getResources().getDisplayMetrics().density);
    }

    @Override
    protected void onPause() {
        releaseTouch();
        super.onPause();
    }

    private void releaseTouch() {
        if (touchPad != null) touchPad.release();
        if (!mBrokenLibraries) nativeTouchAxes(0, 0, 0, 0);
        for (int key : pressed) onNativeKeyUp(key);
        pressed.clear();
    }

    @Override
    public void onWindowFocusChanged(boolean focused) {
        if (!focused) releaseTouch();
        super.onWindowFocusChanged(focused);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        android.os.Process.killProcess(android.os.Process.myPid());
    }
}
