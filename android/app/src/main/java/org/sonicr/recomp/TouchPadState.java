package org.sonicr.recomp;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

final class TouchPadState {
    static final int UP = 1, DOWN = 2, LEFT = 4, RIGHT = 8;
    static final int JUMP = 16, RUN = 32, LOOK_BACK = 64;
    static final int CAMERA_LEFT = 128, RECENTER = 256, CAMERA_RIGHT = 512;
    static final int DRIFT_LEFT = 1024, DRIFT_RIGHT = 2048, START = 4096;
    private static final int MOVE = 1, CAMERA = 2, DPAD = 3, BUTTON = 4;

    record Circle(float x, float y, float radius) {
        boolean contains(float px, float py) {
            return Math.hypot(px - x, py - y) <= radius;
        }
    }

    record Button(String label, String action, int bit, float x, float y,
                  float width, float height, boolean round) {
        boolean contains(float px, float py) {
            if (round) return Math.hypot(px - x, py - y) <= width * 0.58f;
            return Math.abs(px - x) <= width * 0.55f && Math.abs(py - y) <= height * 0.6f;
        }
    }

    private record Finger(int kind, float x, float y) {}
    final List<Button> buttons = new ArrayList<>();
    private final Map<Integer, Finger> fingers = new HashMap<>();
    private final boolean digital;
    Circle movement, camera, dpad;
    float unit;
    float moveX, moveY, cameraX, cameraY;
    int held;

    TouchPadState() { this(false); }

    TouchPadState(boolean digital) {
        this.digital = digital;
    }

    void resize(float width, float height, float density) {
        clear();
        buttons.clear();
        if (digital) {
            resizeDigital(width, height, density);
            return;
        }
        unit = Math.min(300 * density, Math.min(width / 2.5f, height / 1.2f));
        movement = new Circle(unit * 0.24f, height - unit * 0.48f, unit * 0.18f);
        dpad = new Circle(unit * 0.24f, height - unit * 0.14f, unit * 0.12f);
        camera = new Circle(width - unit * 0.50f, height - unit * 0.61f, unit * 0.13f);
        String[] labels = {"A", "B", "C", "X", "Y", "Z"};
        String[] actions = {"Jump / OK", "Run / Back", "Look back", "Orbit left", "Center", "Orbit right"};
        int[] bits = {JUMP, RUN, LOOK_BACK, CAMERA_LEFT, RECENTER, CAMERA_RIGHT};
        for (int i = 0; i < labels.length; i++) {
            int column = i % 3;
            float x = width - unit * (0.64f - column * 0.23f);
            float y = height - unit * (i < 3 ? 0.12f : 0.31f) - column * unit * 0.015f;
            float size = unit * (i < 3 ? 0.15f : 0.12f);
            buttons.add(new Button(labels[i], actions[i], bits[i], x, y, size, size, true));
        }
        buttons.add(new Button("L", "Drift", DRIFT_LEFT, unit * 0.24f,
            height - unit * 0.76f, unit * 0.35f, unit * 0.11f, false));
        buttons.add(new Button("R", "Drift", DRIFT_RIGHT, width - unit * 0.18f,
            height - unit * 0.53f, unit * 0.24f, unit * 0.11f, false));
        buttons.add(new Button("START", "Pause", START, width / 2,
            height - unit * 0.12f, unit * 0.30f, unit * 0.11f, false));
    }

    private void resizeDigital(float width, float height, float density) {
        unit = Math.min(480 * density, Math.min(width / 1.4f, height / 0.56f));
        float margin = 0.05f * unit;
        dpad = new Circle(margin + 0.17f * unit, height - margin - 0.17f * unit, 0.17f * unit);
        float radius = 0.068f * unit, gap = 2.45f * radius, rise = 0.18f * radius;
        float cx = width - margin - radius, cy = height - margin - radius - 2 * rise;
        String[] labels = {"A", "B", "C", "X", "Y", "Z"};
        String[] actions = {"Jump / OK", "Run / Back", "Look back", "Orbit left", "Center", "Orbit right"};
        int[] bits = {JUMP, RUN, LOOK_BACK, CAMERA_LEFT, RECENTER, CAMERA_RIGHT};
        float top = height;
        for (int i = 0; i < labels.length; i++) {
            int column = i % 3;
            float x = cx - (2 - column) * gap;
            float y = cy + (2 - column) * rise;
            float r = radius;
            if (i >= 3) {
                x += 0.45f * radius;
                y -= 2.25f * radius;
                r *= 0.72f;
            }
            buttons.add(new Button(labels[i], actions[i], bits[i], x, y, r * 2, r * 2, true));
            top = Math.min(top, y - r);
        }
        buttons.add(new Button("L", "Drift", DRIFT_LEFT, dpad.x(),
            dpad.y() - dpad.radius() - 0.095f * unit, 0.3f * unit, 0.1f * unit, false));
        buttons.add(new Button("R", "Drift", DRIFT_RIGHT, width - margin - 0.15f * unit,
            top - 0.095f * unit, 0.3f * unit, 0.1f * unit, false));
        buttons.add(new Button("START", "Pause", START, width / 2,
            height - margin - 0.35f * radius, 2.6f * radius, 0.9f * radius, false));
    }

    boolean down(int id, float x, float y) {
        if (fingers.containsKey(id)) return false;
        int kind;
        if (movement != null && movement.contains(x, y) && !owns(MOVE)) kind = MOVE;
        else if (camera != null && camera.contains(x, y) && !owns(CAMERA)) kind = CAMERA;
        else if (dpad.contains(x, y)) kind = DPAD;
        else if (buttonAt(x, y) != 0) kind = BUTTON;
        else return false;
        fingers.put(id, new Finger(kind, x, y));
        update();
        return true;
    }

    void move(int id, float x, float y) {
        Finger finger = fingers.get(id);
        if (finger == null) return;
        fingers.put(id, new Finger(finger.kind, x, y));
        update();
    }

    void up(int id) {
        fingers.remove(id);
        update();
    }

    void clear() {
        fingers.clear();
        held = 0;
        moveX = moveY = cameraX = cameraY = 0;
    }

    private boolean owns(int kind) {
        for (Finger finger : fingers.values()) if (finger.kind == kind) return true;
        return false;
    }

    private int buttonAt(float x, float y) {
        for (Button button : buttons) if (button.contains(x, y)) return button.bit;
        return 0;
    }

    private int directions(float x, float y, float deadzone) {
        double distance = Math.hypot(x, y);
        if (distance <= deadzone) return 0;
        int result = 0;
        if (x / distance > 0.3827) result |= RIGHT;
        if (x / distance < -0.3827) result |= LEFT;
        if (y / distance > 0.3827) result |= UP;
        if (y / distance < -0.3827) result |= DOWN;
        return result;
    }

    private void update() {
        held = 0;
        moveX = moveY = cameraX = cameraY = 0;
        for (Finger finger : fingers.values()) {
            if (finger.kind == BUTTON) {
                held |= buttonAt(finger.x, finger.y);
                continue;
            }
            Circle circle = finger.kind == MOVE ? movement : finger.kind == CAMERA ? camera : dpad;
            float x = (finger.x - circle.x) / circle.radius;
            float y = (circle.y - finger.y) / circle.radius;
            float magnitude = (float)Math.max(1, Math.hypot(x, y));
            x /= magnitude;
            y /= magnitude;
            if (finger.kind == CAMERA) {
                cameraX = x;
                cameraY = y;
            } else {
                held |= directions(x, y, 0.2f);
                if (finger.kind == MOVE) {
                    moveX = x;
                    moveY = y;
                }
            }
        }
    }
}
