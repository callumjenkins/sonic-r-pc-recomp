package org.sonicr.recomp;

public final class TouchPadStateTest {
    private static TouchPadState pad() {
        TouchPadState pad = new TouchPadState();
        pad.resize(900, 400, 1);
        return pad;
    }

    private static void near(float expected, float actual) {
        assert Math.abs(expected - actual) < 0.0001f : expected + " != " + actual;
    }

    private static void stickTravel() {
        TouchPadState p = pad();
        var s = p.movement;
        assert p.down(1, s.x(), s.y());
        assert p.held == 0;
        p.move(1, s.x() + s.radius() * 0.5f, s.y());
        near(0.5f, p.moveX);
        near(0, p.moveY);
        assert p.held == TouchPadState.RIGHT;
        p.move(1, s.x() + s.radius() * 2, s.y() - s.radius() * 2);
        near((float)Math.sqrt(0.5), p.moveX);
        near((float)Math.sqrt(0.5), p.moveY);
        assert p.held == (TouchPadState.UP | TouchPadState.RIGHT);
        p.move(1, s.x() - s.radius(), s.y());
        assert p.held == TouchPadState.LEFT;
        near(-1, p.moveX);
        p.move(1, s.x(), s.y() + s.radius());
        assert p.held == TouchPadState.DOWN;
        near(-1, p.moveY);
        p.move(1, s.x() + s.radius() * 0.199f, s.y());
        assert p.held == 0;
        p.move(1, s.x() + s.radius() * 0.201f, s.y());
        assert p.held == TouchPadState.RIGHT;
        p.up(1);
        assert p.held == 0;
        near(0, p.moveX);
    }

    private static void cameraAndActions() {
        TouchPadState p = pad();
        var c = p.camera;
        assert p.down(1, c.x(), c.y());
        assert !p.down(2, c.x(), c.y());
        p.move(1, c.x() - c.radius(), c.y());
        near(-1, p.cameraX);
        near(0, p.cameraY);
        assert p.held == 0;
        p.move(1, c.x(), c.y() - c.radius() / 2);
        near(0, p.cameraX);
        near(0.5f, p.cameraY);
        p.move(1, c.x(), c.y() + c.radius());
        near(-1, p.cameraY);
        p.up(1);
        near(0, p.cameraY);
        int[] expected = {TouchPadState.JUMP, TouchPadState.RUN, TouchPadState.LOOK_BACK,
            TouchPadState.CAMERA_LEFT, TouchPadState.RECENTER, TouchPadState.CAMERA_RIGHT,
            TouchPadState.DRIFT_LEFT, TouchPadState.DRIFT_RIGHT, TouchPadState.START};
        for (int i = 0; i < expected.length; i++) {
            var b = p.buttons.get(i);
            assert p.down(2, b.x(), b.y());
            assert p.held == expected[i] : b.label();
            p.move(2, 450, 20);
            assert p.held == 0;
            p.up(2);
        }
        var a = p.buttons.get(0);
        var b = p.buttons.get(1);
        assert p.down(3, a.x(), a.y());
        p.move(3, b.x(), b.y());
        assert p.held == TouchPadState.RUN;
        p.clear();
    }

    private static void independentFingers() {
        TouchPadState p = pad();
        var a = p.buttons.get(0);
        assert p.down(7, p.movement.x(), p.movement.y() - p.movement.radius());
        assert p.down(3, p.camera.x() + p.camera.radius(), p.camera.y());
        assert p.down(9, a.x(), a.y());
        assert (p.held & TouchPadState.JUMP) != 0;
        near(1, p.moveY);
        near(1, p.cameraX);
        p.up(9);
        assert p.held == TouchPadState.UP;
        near(1, p.cameraX);
        p.up(7);
        near(0, p.moveY);
        near(1, p.cameraX);
        p.clear();
        near(0, p.cameraX);
        assert p.held == 0;
    }

    private static void ownershipAndDuplicates() {
        TouchPadState p = pad();
        assert p.down(1, p.movement.x(), p.movement.y());
        assert !p.down(2, p.movement.x(), p.movement.y());
        assert !p.down(1, p.camera.x(), p.camera.y());
        var a = p.buttons.get(0);
        p.move(1, a.x(), a.y());
        assert (p.held & TouchPadState.JUMP) == 0;
        assert p.down(2, a.x(), a.y());
        assert p.down(3, a.x(), a.y());
        p.up(2);
        assert (p.held & TouchPadState.JUMP) != 0;
        p.up(3);
        assert (p.held & TouchPadState.JUMP) == 0;
        p.clear();
        assert p.down(4, p.dpad.x(), p.dpad.y() - p.dpad.radius() * 0.6f);
        assert p.down(6, p.dpad.x(), p.dpad.y() - p.dpad.radius() * 0.6f);
        p.up(4);
        assert p.held == TouchPadState.UP;
        assert p.down(5, p.movement.x(), p.movement.y() - p.movement.radius());
        p.up(5);
        assert p.held == TouchPadState.UP;
        assert p.down(5, p.movement.x() + p.movement.radius() * 0.5f,
            p.movement.y() - p.movement.radius() * 0.4f);
        assert p.down(7, p.camera.x() + p.camera.radius() * 0.6f,
            p.camera.y() + p.camera.radius() * 0.5f);
        assert p.moveX != 0 && p.moveY != 0 && p.cameraX != 0 && p.cameraY != 0;
        p.resize(800, 400, 1);
        assert p.held == 0;
        near(0, p.moveX);
        near(0, p.moveY);
        near(0, p.cameraX);
        near(0, p.cameraY);
        assert !p.down(8, 400, 20);
    }

    private static void layoutSizes() {
        for (float[] size : new float[][]{{900, 400, 1}, {640, 360, 3}, {1200, 900, 2}, {2400, 1080, 2.75f}}) {
            TouchPadState p = new TouchPadState();
            p.resize(size[0], size[1], size[2]);
            for (var c : new TouchPadState.Circle[]{p.movement, p.camera, p.dpad}) {
                assert c.x() - c.radius() >= 0;
                assert c.y() - c.radius() >= 0;
                assert c.x() + c.radius() <= size[0];
                assert c.y() + c.radius() <= size[1];
            }
            for (var b : p.buttons) {
                assert b.x() - b.width() / 2 >= 0;
                assert b.x() + b.width() / 2 <= size[0];
                assert b.y() - b.height() / 2 >= 0;
                assert b.y() + b.height() / 2 <= size[1];
                assert p.down(0, b.x(), b.y());
                assert p.held == b.bit();
                p.clear();
            }
            var circles = new TouchPadState.Circle[]{p.movement, p.camera, p.dpad};
            for (int i = 0; i < circles.length; i++) {
                for (int j = i + 1; j < circles.length; j++) {
                    var a = circles[i];
                    var b = circles[j];
                    assert Math.hypot(a.x() - b.x(), a.y() - b.y()) > a.radius() + b.radius();
                }
            }
            for (var b : p.buttons) {
                for (var c : circles) {
                    float dx = Math.max(0, Math.abs(b.x() - c.x()) - b.width() * 0.58f);
                    float dy = Math.max(0, Math.abs(b.y() - c.y()) - b.height() * 0.6f);
                    assert Math.hypot(dx, dy) > c.radius();
                }
            }
        }
    }

    private static void digitalPad() {
        for (float[] size : new float[][]{{900, 400, 1}, {640, 360, 1}, {2400, 1080, 2.625f}, {800, 700, 2}}) {
            TouchPadState p = new TouchPadState(true);
            p.resize(size[0], size[1], size[2]);
            assert p.movement == null && p.camera == null;
            near(p.unit * 0.17f, p.dpad.radius());
            near(p.unit * 0.22f, p.dpad.x());
            assert p.buttons.size() == 9;
            for (int i = 0; i < p.buttons.size(); i++) {
                var first = p.buttons.get(i);
                for (int j = i + 1; j < p.buttons.size(); j++) {
                    var second = p.buttons.get(j);
                    assert Math.abs(first.x() - second.x()) >= (first.width() + second.width()) / 2 ||
                        Math.abs(first.y() - second.y()) >= (first.height() + second.height()) / 2;
                }
            }
            for (var button : p.buttons) {
                assert button.x() - button.width() / 2 >= 0;
                assert button.x() + button.width() / 2 <= size[0];
                assert button.y() - button.height() / 2 >= 0;
                assert button.y() + button.height() / 2 <= size[1];
                assert p.down(1, button.x(), button.y());
                assert p.held == button.bit() : button.label();
                p.clear();
            }
            var a = p.buttons.get(0);
            var b = p.buttons.get(1);
            var x = p.buttons.get(3);
            assert x.y() < a.y() && x.width() < a.width();
            assert b.x() > a.x() && b.y() < a.y();
            var d = p.dpad;
            assert p.down(1, d.x() - d.radius() / 2, d.y() - d.radius() / 2);
            assert p.held == (TouchPadState.UP | TouchPadState.LEFT);
            assert !p.down(1, a.x(), a.y());
            assert p.held == (TouchPadState.UP | TouchPadState.LEFT);
            assert p.down(2, a.x(), a.y());
            assert p.down(3, a.x(), a.y());
            assert p.down(4, b.x(), b.y());
            p.up(2);
            assert p.held == (TouchPadState.UP | TouchPadState.LEFT | TouchPadState.JUMP | TouchPadState.RUN);
            p.move(1, size[0] / 2, d.y());
            assert p.held == (TouchPadState.RIGHT | TouchPadState.JUMP | TouchPadState.RUN);
            near(0, p.moveX); near(0, p.moveY); near(0, p.cameraX); near(0, p.cameraY);
            p.up(3);
            p.up(4);
            p.move(1, a.x(), a.y());
            assert p.held == TouchPadState.RIGHT;
            near(0, p.moveX); near(0, p.moveY);
            p.clear();
            assert p.held == 0;
            assert p.down(1, a.x(), a.y());
            p.resize(size[0], size[1], size[2]);
            assert p.held == 0;
        }
    }

    public static void main(String[] args) {
        digitalPad();
        stickTravel();
        independentFingers();
        cameraAndActions();
        ownershipAndDuplicates();
        layoutSizes();
        System.out.println("Touch pad state passed");
    }
}
