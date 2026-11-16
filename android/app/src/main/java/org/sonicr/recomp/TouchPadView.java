package org.sonicr.recomp;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.view.MotionEvent;
import android.view.View;

final class TouchPadView extends View {
    interface Listener {
        void input(int buttons, float moveX, float moveY, float cameraX, float cameraY);
    }

    private final TouchPadState pad;
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Listener listener;

    TouchPadView(Context context, Listener listener) {
        this(context, listener, false);
    }

    TouchPadView(Context context, Listener listener, boolean digital) {
        super(context);
        pad = new TouchPadState(digital);
        this.listener = listener;
        setContentDescription(digital ? "Saturn digital touch pad" : "Saturn 3D touch pad");
        setFocusable(false);
    }

    void release() {
        pad.clear();
        publish();
    }

    @Override
    protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        pad.resize(width, height, getResources().getDisplayMetrics().density);
        publish();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int action = event.getActionMasked();
        int index = event.getActionIndex();
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            boolean accepted = pad.down(event.getPointerId(index), event.getX(index), event.getY(index));
            if (action == MotionEvent.ACTION_DOWN && !accepted) return false;
        } else if (action == MotionEvent.ACTION_MOVE) {
            for (int i = 0; i < event.getPointerCount(); i++) {
                pad.move(event.getPointerId(i), event.getX(i), event.getY(i));
            }
        } else if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) {
            pad.up(event.getPointerId(index));
            if (action == MotionEvent.ACTION_UP) performClick();
        } else if (action == MotionEvent.ACTION_CANCEL) {
            pad.clear();
        }
        publish();
        return true;
    }

    @Override
    public boolean performClick() {
        super.performClick();
        return true;
    }

    private void publish() {
        listener.input(pad.held, pad.moveX, pad.moveY, pad.cameraX, pad.cameraY);
        invalidate();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (pad.dpad == null) return;
        if (pad.movement == null) {
            drawDigital(canvas);
            return;
        }
        stick(canvas, pad.movement, pad.moveX, pad.moveY, "MOVE");
        stick(canvas, pad.camera, pad.cameraX, pad.cameraY, "CAMERA");
        var d = pad.dpad;
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(0x9030414b);
        float arm = d.radius() * 0.36f;
        canvas.drawRoundRect(d.x() - arm, d.y() - d.radius(), d.x() + arm, d.y() + d.radius(), arm / 3, arm / 3, paint);
        canvas.drawRoundRect(d.x() - d.radius(), d.y() - arm, d.x() + d.radius(), d.y() + arm, arm / 3, arm / 3, paint);
        text(canvas, "↑", d.x(), d.y() - d.radius() * 0.67f, pad.unit * 0.055f);
        text(canvas, "↓", d.x(), d.y() + d.radius() * 0.67f, pad.unit * 0.055f);
        text(canvas, "←", d.x() - d.radius() * 0.67f, d.y(), pad.unit * 0.055f);
        text(canvas, "→", d.x() + d.radius() * 0.67f, d.y(), pad.unit * 0.055f);
        for (var button : pad.buttons) {
            paint.setColor((pad.held & button.bit()) != 0 ? 0xc0559c9b : 0x9030414b);
            if (button.round()) {
                canvas.drawCircle(button.x(), button.y(), button.width() / 2, paint);
            } else {
                canvas.drawRoundRect(button.x() - button.width() / 2, button.y() - button.height() / 2,
                    button.x() + button.width() / 2, button.y() + button.height() / 2,
                    button.height() / 2, button.height() / 2, paint);
            }
            text(canvas, button.label(), button.x(), button.y() - pad.unit * 0.012f, pad.unit * 0.047f);
            text(canvas, button.action(), button.x(), button.y() + pad.unit * 0.029f, pad.unit * 0.025f);
        }
    }

    private void drawDigital(Canvas canvas) {
        var d = pad.dpad;
        float r = d.radius(), arm = 0.62f * r;
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(0x4d000000);
        canvas.drawCircle(d.x(), d.y(), 1.08f * r, paint);
        paint.setColor(0x8c212126);
        canvas.drawRoundRect(d.x() - r, d.y() - arm / 2, d.x() + r, d.y() + arm / 2,
            0.15f * arm, 0.15f * arm, paint);
        canvas.drawRoundRect(d.x() - arm / 2, d.y() - r, d.x() + arm / 2, d.y() + r,
            0.15f * arm, 0.15f * arm, paint);
        int[] bits = {TouchPadState.UP, TouchPadState.DOWN, TouchPadState.LEFT, TouchPadState.RIGHT};
        float[] dx = {0, 0, -1, 1}, dy = {-1, 1, 0, 0};
        for (int i = 0; i < bits.length; i++) {
            float x = d.x() + dx[i] * 0.62f * r, y = d.y() + dy[i] * 0.62f * r;
            if ((pad.held & bits[i]) != 0) {
                paint.setColor(0xd9515158);
                canvas.drawRoundRect(x - arm / 2, y - arm / 2, x + arm / 2, y + arm / 2,
                    0.15f * arm, 0.15f * arm, paint);
            }
            paint.setColor(0x59ffffff);
            canvas.drawCircle(x + dx[i] * 0.12f * r, y + dy[i] * 0.12f * r, 0.09f * r, paint);
        }
        for (var button : pad.buttons) {
            boolean held = (pad.held & button.bit()) != 0;
            int color = button.round() ? 0x212126 : button.bit() == TouchPadState.START ? 0x45454d : 0x5c5c63;
            float corner = button.height() * (button.bit() == TouchPadState.START ? 0.5f : 0.3f);
            paint.setColor(0x4d000000);
            if (button.round()) canvas.drawCircle(button.x(), button.y(), button.width() * 0.55f, paint);
            else roundedButton(canvas, button, corner + 4, 4);
            paint.setColor((held ? 0xd9000000 : 0x8c000000) | color);
            if (held) {
                int red = Math.min(255, (int)(((color >> 16) & 255) * 1.3f + 38));
                int green = Math.min(255, (int)(((color >> 8) & 255) * 1.3f + 38));
                int blue = Math.min(255, (int)((color & 255) * 1.3f + 38));
                paint.setColor(0xd9000000 | red << 16 | green << 8 | blue);
            }
            if (button.round()) canvas.drawCircle(button.x(), button.y(), button.width() / 2, paint);
            else roundedButton(canvas, button, corner, 0);
            paint.setColor(held ? 0xffebebf0 : 0xccebebf0);
            paint.setTextAlign(Paint.Align.CENTER);
            paint.setTextSize(button.round() ? button.width() * 0.4f : button.height() *
                (button.bit() == TouchPadState.START ? 0.42f : 0.5f));
            canvas.drawText(button.label(), button.x(), button.y() - (paint.ascent() + paint.descent()) / 2, paint);
        }
    }

    private void roundedButton(Canvas canvas, TouchPadState.Button button, float corner, float outset) {
        canvas.drawRoundRect(button.x() - button.width() / 2 - outset, button.y() - button.height() / 2 - outset,
            button.x() + button.width() / 2 + outset, button.y() + button.height() / 2 + outset,
            corner, corner, paint);
    }

    private void stick(Canvas canvas, TouchPadState.Circle circle, float x, float y, String label) {
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(0x7030414b);
        canvas.drawCircle(circle.x(), circle.y(), circle.radius(), paint);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(pad.unit * 0.006f);
        paint.setColor(0xaaa9d9d2);
        canvas.drawCircle(circle.x(), circle.y(), circle.radius(), paint);
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(0xb0a9d9d2);
        canvas.drawCircle(circle.x() + x * circle.radius() * 0.65f,
            circle.y() - y * circle.radius() * 0.65f, circle.radius() * 0.35f, paint);
        text(canvas, label, circle.x(), circle.y() - circle.radius() - pad.unit * 0.025f, pad.unit * 0.03f);
    }

    private void text(Canvas canvas, String text, float x, float y, float size) {
        paint.setColor(0xfff2f2e8);
        paint.setTextSize(size);
        paint.setTextAlign(Paint.Align.CENTER);
        canvas.drawText(text, x, y - (paint.ascent() + paint.descent()) / 2, paint);
    }
}
