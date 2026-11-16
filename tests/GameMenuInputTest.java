package org.sonicr.menutest;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Intent;
import android.os.Bundle;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.View;
import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Set;

public final class GameMenuInputTest extends Instrumentation {
    private boolean digital;
    @Override
    public void onCreate(Bundle arguments) {
        super.onCreate(arguments);
        digital = "digital".equals(arguments.getString("layout"));
        start();
    }

    @Override
    public void onStart() {
        Bundle result = new Bundle();
        try {
            Intent intent = new Intent().setClassName("org.sonicr.recomp", "org.sonicr.recomp.GameActivity");
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.putExtra("args", new String[]{new File(getTargetContext().getFilesDir(), "gamedata").getAbsolutePath()});
            intent.putExtra("touch3d", !digital);
            Activity game = startActivitySync(intent);
            SystemClock.sleep(3000);
            runOnMainSync(() -> {
                try {
                    View padView = (View)field(game, "touchPad");
                    Object pad = field(padView, "pad");
                    Object movement = field(pad, digital ? "dpad" : "movement");
                    float x = coordinate(movement, "x") + coordinate(movement, "radius") * 0.8f;
                    float y = coordinate(movement, "y") - coordinate(movement, "radius") * 0.3f;
                    Object button = ((java.util.List<?>)field(pad, "buttons")).get(1);
                    float bx = coordinate(button, "x"), by = coordinate(button, "y");
                    touch(padView, MotionEvent.ACTION_DOWN, new float[][]{{x, y}});
                    if (digital) {
                        touch(padView, MotionEvent.ACTION_POINTER_DOWN | (1 << 8), new float[][]{{x, y}, {bx, by}});
                        require((float)field(pad, "moveX") == 0 && (float)field(pad, "moveY") == 0,
                            "Digital pad leaves analogue axes released");
                    } else {
                        Object camera = field(pad, "camera");
                        float cx = coordinate(camera, "x") - coordinate(camera, "radius") * 0.7f;
                        float cy = coordinate(camera, "y") + coordinate(camera, "radius") * 0.3f;
                        touch(padView, MotionEvent.ACTION_POINTER_DOWN | (1 << 8), new float[][]{{x, y}, {cx, cy}});
                        touch(padView, MotionEvent.ACTION_POINTER_DOWN | (2 << 8), new float[][]{{x, y}, {cx, cy}, {bx, by}});
                        require((float)field(pad, "moveX") > 0.7f, "Stick has a held axis");
                        require((float)field(pad, "moveY") > 0.2f, "Movement has both axes held");
                        require((float)field(pad, "cameraX") < -0.6f && (float)field(pad, "cameraY") < -0.2f,
                            "Camera has both axes held");
                    }
                    require(((int)field(pad, "held") & 32) != 0, "Run button held with directional input");
                    require(!((Set<?>)field(game, "pressed")).isEmpty(), "Directional touch presses SDL keys");
                    Method openMenu = game.getClass().getDeclaredMethod("openMenu");
                    openMenu.setAccessible(true);
                    openMenu.invoke(game);
                    require((float)field(pad, "moveX") == 0 && (float)field(pad, "moveY") == 0,
                        "Opening menu releases movement axes before pause");
                    require((float)field(pad, "cameraX") == 0 && (float)field(pad, "cameraY") == 0,
                        "Opening menu leaves camera axes released");
                    require((int)field(pad, "held") == 0 && ((Set<?>)field(game, "pressed")).isEmpty(),
                        "Opening menu releases synthetic buttons before pause");
                } catch (ReflectiveOperationException error) {
                    throw new AssertionError(error);
                }
            });
            result.putString("stream", "Game menu input release passed\n");
            finish(-1, result);
        } catch (Throwable error) {
            result.putString("stream", error.toString() + "\n");
            finish(0, result);
        }
    }

    private static void touch(View pad, int action, float[][] points) {
        MotionEvent.PointerProperties[] properties = new MotionEvent.PointerProperties[points.length];
        MotionEvent.PointerCoords[] coordinates = new MotionEvent.PointerCoords[points.length];
        for (int i = 0; i < points.length; i++) {
            properties[i] = new MotionEvent.PointerProperties();
            properties[i].id = i;
            properties[i].toolType = MotionEvent.TOOL_TYPE_FINGER;
            coordinates[i] = new MotionEvent.PointerCoords();
            coordinates[i].x = points[i][0];
            coordinates[i].y = points[i][1];
        }
        MotionEvent event = MotionEvent.obtain(SystemClock.uptimeMillis(), SystemClock.uptimeMillis(), action,
            points.length, properties, coordinates, 0, 0, 1, 1, 0, 0, android.view.InputDevice.SOURCE_TOUCHSCREEN, 0);
        require(pad.onTouchEvent(event), "Touch accepted");
        event.recycle();
    }

    private static Object field(Object object, String name) throws ReflectiveOperationException {
        Field field = object.getClass().getDeclaredField(name);
        field.setAccessible(true);
        return field.get(object);
    }

    private static float coordinate(Object circle, String name) throws ReflectiveOperationException {
        Method accessor = circle.getClass().getDeclaredMethod(name);
        accessor.setAccessible(true);
        return (float)accessor.invoke(circle);
    }

    private static void require(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }
}
