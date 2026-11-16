package org.sonicr.recomp;

import android.content.Context;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.StateListDrawable;
import android.os.Build;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import java.util.ArrayList;

/** A sidebar of page buttons beside the selected page, or a row above it on narrow screens. */
class SettingsPages extends LinearLayout {
    static final int SIDEBAR_MIN_WIDTH_DP = 600;
    private final ArrayList<Button> buttons = new ArrayList<>();
    private final ArrayList<View> pages = new ArrayList<>();
    private final LinearLayout tabs;
    private final ScrollView content;
    private ViewGroup tabScroller;
    private Boolean sidebar;
    private int selected = -1;

    SettingsPages(Context context) {
        super(context);
        tabs = new LinearLayout(context);
        content = new ScrollView(context);
        content.setFillViewport(true);
        layoutFor(getResources().getConfiguration().screenWidthDp);
    }

    void addPage(String title, View page) {
        int index = pages.size();
        Button button = new Button(getContext());
        button.setText(title);
        button.setAllCaps(false);
        button.setGravity(Gravity.CENTER_VERTICAL | Gravity.START);
        button.setBackground(tabBackground());
        button.setPadding(dp(16), dp(10), dp(16), dp(10));
        button.setOnClickListener(view -> select(index));
        buttons.add(button);
        pages.add(page);
        tabs.addView(button, tabParams());
        if (selected < 0) select(0);
    }

    void select(int index) {
        if (index < 0 || index >= pages.size()) return;
        selected = index;
        for (int i = 0; i < buttons.size(); i++) {
            Button button = buttons.get(i);
            button.setSelected(i == index);
            if (Build.VERSION.SDK_INT >= 30) button.setStateDescription(i == index ? "Selected" : null);
        }
        content.removeAllViews();
        content.addView(pages.get(index));
        content.scrollTo(0, 0);
    }

    int selectedPage() { return selected; }

    @Override
    protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        super.onSizeChanged(width, height, oldWidth, oldHeight);
        int widthDp = (int)(width / getResources().getDisplayMetrics().density);
        if ((widthDp >= SIDEBAR_MIN_WIDTH_DP) != sidebar) post(() -> layoutFor(widthDp));
    }

    private void layoutFor(int widthDp) {
        boolean wide = widthDp >= SIDEBAR_MIN_WIDTH_DP;
        if (sidebar != null && sidebar == wide) return;
        sidebar = wide;
        removeAllViews();
        if (tabScroller != null) tabScroller.removeAllViews();
        setOrientation(wide ? HORIZONTAL : VERTICAL);
        tabs.setOrientation(wide ? VERTICAL : HORIZONTAL);
        tabs.setPadding(dp(8), dp(8), dp(8), dp(8));
        for (Button button : buttons) button.setLayoutParams(tabParams());
        if (wide) {
            ScrollView column = new ScrollView(getContext());
            unfocusable(column);
            column.addView(tabs);
            tabScroller = column;
            addView(column, new LayoutParams(dp(176), LayoutParams.MATCH_PARENT));
            addView(content, new LayoutParams(0, LayoutParams.MATCH_PARENT, 1));
        } else {
            HorizontalScrollView row = new HorizontalScrollView(getContext());
            row.setHorizontalScrollBarEnabled(false);
            unfocusable(row);
            row.addView(tabs);
            tabScroller = row;
            addView(row, new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
            addView(content, new LayoutParams(LayoutParams.MATCH_PARENT, 0, 1));
        }
    }

    private static void unfocusable(ViewGroup scroller) {
        scroller.setFocusable(false);
        scroller.setDefaultFocusHighlightEnabled(false);
    }

    private LinearLayout.LayoutParams tabParams() {
        boolean wide = sidebar == null || sidebar;
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
            wide ? LayoutParams.MATCH_PARENT : LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
        params.setMargins(0, 0, wide ? 0 : dp(4), wide ? dp(4) : 0);
        return params;
    }

    private StateListDrawable tabBackground() {
        StateListDrawable states = new StateListDrawable();
        states.addState(new int[]{android.R.attr.state_focused}, rounded(0xFF3E6F8E));
        states.addState(new int[]{android.R.attr.state_pressed}, rounded(0xFF3E6F8E));
        states.addState(new int[]{android.R.attr.state_selected}, rounded(0xFF24506B));
        states.addState(new int[]{}, rounded(0x00000000));
        return states;
    }

    private GradientDrawable rounded(int colour) {
        GradientDrawable shape = new GradientDrawable();
        shape.setColor(colour);
        shape.setCornerRadius(dp(8));
        return shape;
    }

    int dp(int value) {
        return (int)(value * getResources().getDisplayMetrics().density);
    }

    static LinearLayout page(Context context) {
        LinearLayout page = new LinearLayout(context);
        page.setOrientation(VERTICAL);
        int padding = (int)(16 * context.getResources().getDisplayMetrics().density);
        page.setPadding(padding, padding, padding, padding);
        return page;
    }
}
