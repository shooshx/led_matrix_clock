#pragma once
#include <Preferences.h>
#include "base_utils.h"
#include "ModeIcons.h"
#include "ble_hid.h"

// Text typed with a keyboard, in the default font on a fixed grid of cells. it's drawn over the drawing of DrawTextState.
// Esc starts and stops editing. while editing there's a blinking cursor that the arrows move freely
// around the grid, every character has the color that was selected (PgUp/PgDn) when it was typed.
// the text is saved in flash. no scrolling, text that doesn't fit is dropped.
// the text is a grid of the landscape size. when the display is narrower (portrait) the lines are
// wrapped into the view, and rows that don't fit the view aren't shown
class TypeState : public IScreen
{
public:
    static const int CELL_W = 6; // default font is 5x7 with a space
    static const int CELL_H = 8;
    static const int COLS = DISPLAY_WIDTH / CELL_W;
    static const int ROWS = (DISPLAY_HEIGHT - MODE_ICON_SIZE) / CELL_H; // keep the bottom row for the mode icon
    static const unsigned long BLINK_MS = 500;
    static const unsigned long SAVE_DELAY_MS = 2000; // don't write the flash on every key

    struct Cell {
        char c;        // 0 is an empty cell
        uint8_t color; // index in hid_palette
    };

    Cell m_grid[ROWS][COLS];
    int m_row = 0;
    int m_col = 0;
    uint8_t m_color = 0;  // palette index for new text, also the color of drawing with the remote
    bool m_editing = false;
    uint32_t m_version = 0; // incremented when the text changes

private:
    // set right after a char was typed in the last cell of row m_full_row, the cursor is then logically after
    // that cell, even though it's shown at the start of the next row (or on that cell in the last row)
    bool m_row_filled = false;
    int m_full_row = 0;
    bool m_need_draw = true;
    bool m_blink_on = true;
    unsigned long m_blink_time = 0;
    unsigned long m_save_time = 0; // when to save, 0 if nothing to save
    Preferences m_pref;

public:
    TypeState() {
        memset(m_grid, 0, sizeof(m_grid));
    }

    void load() {
        m_pref.begin("typetext", false);
        if (m_pref.getBytesLength("grid") == sizeof(m_grid))
            m_pref.getBytes("grid", m_grid, sizeof(m_grid));
        m_color = m_pref.getUChar("color", 0) % HID_PALETTE_SIZE;
        for (int r = 0; r < ROWS; ++r)
            for (int c = 0; c < COLS; ++c)
                m_grid[r][c].color %= HID_PALETTE_SIZE;
    }

    void save() {
        m_pref.putBytes("grid", m_grid, sizeof(m_grid));
        m_pref.putUChar("color", m_color);
        m_save_time = 0;
    }

    void clear_text() {
        memset(m_grid, 0, sizeof(m_grid));
        m_row = 0;
        m_col = 0;
        m_row_filled = false;
        changed(true);
    }

    void set_color(uint8_t c) {
        m_color = c % HID_PALETTE_SIZE;
        changed(true);
    }

    void set_editing(bool v) {
        if (m_editing && !v && m_save_time != 0)
            save();
        m_editing = v;
        changed(false);
    }

    // called in loop, returns true if need to draw
    bool update() {
        unsigned long now = millis();
        if (m_save_time != 0 && (long)(now - m_save_time) >= 0)
            save();
        if (m_editing && now - m_blink_time >= BLINK_MS) {
            m_blink_on = !m_blink_on;
            m_blink_time = now;
            m_need_draw = true;
        }
        bool r = m_need_draw;
        m_need_draw = false;
        return r;
    }

    // input while editing
    void on_hid(const HidEvent& ev) {
        if (ev.type != HidEvent::KEY_DOWN)
            return;
        // only the key right after filling a row sees it
        bool row_filled = m_row_filled;
        m_row_filled = false;
        switch (ev.key) {
        case HID_KEY_ESC:
            if (!ev.repeat)
                set_editing(false);
            return;
        case HID_KEY_UP:    move_vertical(-1); return;
        case HID_KEY_DOWN:  move_vertical(1); return;
        case HID_KEY_LEFT:  move_to(m_row, m_col - 1); return;
        case HID_KEY_RIGHT: move_to(m_row, m_col + 1); return;
        case HID_KEY_HOME:  move_to(m_row, 0); return;
        case HID_KEY_END:   move_to(m_row, line_len(m_row)); return;
        case HID_KEY_PGUP:  m_color = palette_step(m_color, 1); changed(true); return;
        case HID_KEY_PGDN:  m_color = palette_step(m_color, -1); changed(true); return;
        case HID_KEY_ENTER:     enter(); return;
        case HID_KEY_BACKSPACE:
            if (row_filled) { // erase the char that filled the row
                m_row = m_full_row;
                m_col = COLS - 1;
                remove_cell(m_row, m_col);
                changed(true);
            }
            else
                backspace();
            return;
        case HID_KEY_DELETE:    del(); return;
        }
        if (ev.ch != 0) {
            if (row_filled && m_row == m_full_row) { // the last row is full, no room
                m_row_filled = true;
                return;
            }
            type_char(ev.ch);
        }
    }

    // draws the text over what's already on the display, with the cursor while editing
    void draw() override {
        draw_text(m_editing && m_blink_on);
    }

    void draw_text(bool with_cursor) {
        display.setFont(nullptr);
        display.setTextSize(1);
        ViewRow rows[MAX_VIEW_ROWS];
        int n = build_view(rows);
        int vc = view_cols(), vr_count = min(n, view_rows());
        for (int vr = 0; vr < vr_count; ++vr) {
            for (int c = 0; c < vc && rows[vr].start + c < COLS; ++c) {
                const Cell& cell = m_grid[rows[vr].line][rows[vr].start + c];
                if (cell.c == 0)
                    continue;
                uint16_t col = hid_palette[cell.color];
                display.drawChar(cell_x(c), cell_y(vr), cell.c, col, col, 1); // bg == color is transparent
            }
        }
        int cur = cursor_view_row(rows, n);
        if (with_cursor && cur < vr_count) // underline below the glyphs
            display.drawFastHLine(cell_x(m_col - rows[cur].start), cell_y(cur) + CELL_H - 1, CELL_W - 1, hid_palette[m_color]);
    }

private:
    // a row on the display: a part of a line, from column `start`, view_cols() wide
    struct ViewRow {
        int8_t line;
        int8_t start;
    };
    static const int MAX_VIEW_ROWS = ROWS * COLS;

    int view_cols() const { return min(COLS, (int)display.width() / CELL_W); }
    int view_rows() const { return (display.height() - MODE_ICON_SIZE) / CELL_H; }
    int cell_x(int c) const { return (display.width() - view_cols() * CELL_W) / 2 + c * CELL_W; }
    static int cell_y(int r) { return r * CELL_H; }

    // the lines wrapped into rows of the view width. a line takes as many rows as its text needs, at least one,
    // and as the cursor needs if it's past the text. returns the number of rows, also the ones that don't fit the view
    int build_view(ViewRow* rows) const {
        int vc = view_cols(), n = 0;
        for (int l = 0; l < ROWS; ++l) {
            int len = line_len(l);
            if (l == m_row)
                len = max(len, m_col + 1);
            int count = max(1, (len + vc - 1) / vc);
            for (int k = 0; k < count; ++k)
                rows[n++] = ViewRow{(int8_t)l, (int8_t)(k * vc)};
        }
        return n;
    }

    int cursor_view_row(const ViewRow* rows, int n) const {
        int vc = view_cols();
        for (int i = 0; i < n; ++i)
            if (rows[i].line == m_row && m_col >= rows[i].start && m_col < rows[i].start + vc)
                return i;
        return 0;
    }

    // up / down on the display, in portrait this can be within the same line
    void move_vertical(int d) {
        ViewRow rows[MAX_VIEW_ROWS];
        int n = build_view(rows);
        int cur = cursor_view_row(rows, n);
        int target = cur + d;
        if (target >= 0 && target < n && target < view_rows()) {
            int view_col = m_col - rows[cur].start;
            m_row = rows[target].line;
            m_col = min(rows[target].start + view_col, COLS - 1);
        }
        changed(false);
    }

    // restart the blink so that the cursor is visible after it moves, and redraw
    void changed(bool need_save) {
        m_blink_on = true;
        m_blink_time = millis();
        m_need_draw = true;
        if (need_save) {
            m_save_time = millis() + SAVE_DELAY_MS;
            ++m_version;
        }
    }

    // the cursor moves freely in the grid, also past the end of the text
    void move_to(int r, int c) {
        m_row = constrain(r, 0, ROWS - 1);
        m_col = constrain(c, 0, COLS - 1);
        changed(false);
    }

    // index after the last non-empty cell of the row
    int line_len(int r) const {
        int len = COLS;
        while (len > 0 && m_grid[r][len - 1].c == 0)
            --len;
        return len;
    }

    void clear_row(int r) {
        memset(m_grid[r], 0, sizeof(m_grid[r]));
    }

    // removes the cell at (r, c), the rest of the row moves left
    void remove_cell(int r, int c) {
        memmove(&m_grid[r][c], &m_grid[r][c + 1], (COLS - c - 1) * sizeof(Cell));
        m_grid[r][COLS - 1] = Cell{0, 0};
    }

    // removes row r, the rows below move up
    void remove_row(int r) {
        for (int i = r; i < ROWS - 1; ++i)
            memcpy(m_grid[i], m_grid[i + 1], sizeof(m_grid[i]));
        clear_row(ROWS - 1);
    }

    // appends row r+1 to row r at column `at` and removes row r+1, what doesn't fit is dropped
    void join_next(int r, int at) {
        for (int c = 0; c < COLS && at + c < COLS; ++c)
            m_grid[r][at + c] = m_grid[r + 1][c];
        remove_row(r + 1);
    }

    void type_char(char ch) {
        Cell* row = m_grid[m_row];
        // insert, the last cell of the row is dropped
        memmove(&row[m_col + 1], &row[m_col], (COLS - m_col - 1) * sizeof(Cell));
        row[m_col] = Cell{ch, m_color};
        if (m_col < COLS - 1)
            ++m_col;
        else {
            m_row_filled = true;
            m_full_row = m_row;
            if (m_row < ROWS - 1) { // at the end of the row continue in the next one
                ++m_row;
                m_col = 0;
            }
        }
        changed(true);
    }

    void enter() {
        // split the row at the cursor, only if there's an empty row at the bottom to push down
        if (m_row < ROWS - 1 && line_len(ROWS - 1) == 0) {
            for (int i = ROWS - 1; i > m_row + 1; --i)
                memcpy(m_grid[i], m_grid[i - 1], sizeof(m_grid[i]));
            clear_row(m_row + 1);
            for (int c = m_col; c < COLS; ++c) {
                m_grid[m_row + 1][c - m_col] = m_grid[m_row][c];
                m_grid[m_row][c] = Cell{0, 0};
            }
        }
        move_to(m_row + 1, 0);
        changed(true);
    }

    void backspace() {
        if (m_col > 0) {
            --m_col;
            remove_cell(m_row, m_col);
        }
        else if (m_row > 0) {
            int prev_len = line_len(m_row - 1);
            join_next(m_row - 1, prev_len);
            m_row -= 1;
            m_col = min(prev_len, COLS - 1);
        }
        changed(true);
    }

    void del() {
        if (m_col < line_len(m_row))
            remove_cell(m_row, m_col);
        else if (m_row < ROWS - 1)
            join_next(m_row, m_col);
        changed(true);
    }
};
