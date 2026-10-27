#pragma once
#include "GamesState.h"

// Tetris, played in portrait: the standard rules with SRS rotation and wall kicks, the 7-bag randomizer,
// the next piece, levels every 10 lines and the usual scoring. no ghost piece.
// Left/Right move, Up rotates (Vol+/Vol- rotate too), Down drops faster, OK drops all the way
namespace tetris_art {
static const char* const ICON[] = {
    "MMMM.MMMM.MMMM",
    "MMMM.MMMM.MMMM",
    "MMMM.MMMM.MMMM",
    "..............",
    ".....MMMM.BBBB",
    ".....MMMM.BBBB",
    ".....MMMM.BBBB",
    "..............",
    "OOOO.YYYY.BBBB",
    "OOOO.YYYY.BBBB",
    "OOOO.YYYY.BBBB",
    "..............",
    "OOOO.YYYY.BBBB",
    "OOOO.YYYY.BBBB",
};
static const Sprite S_ICON = { 14, 14, ICON };
}

class TetrisGame : public Game
{
public:
    const char* name() const override { return "Tetris"; }
    const Sprite& icon() const override { return tetris_art::S_ICON; }
    bool portrait() const override { return true; }
    uint32_t high_score() const override { return m_hi; }

    void load(Preferences& pref) override {
        m_pref = &pref;
        m_hi = pref.getUInt("tetris_hi", 0);
    }

    void start() override {
        reset();
        m_phase = READY;
    }

    void on_hid(const HidEvent& ev) override {
        if (ev.type != HidEvent::KEY_DOWN)
            return;
        bool press = !ev.repeat;
        if (m_phase == READY || m_phase == OVER) {
            if (press && (ev.key == HID_KEY_ENTER || ev.key == HID_KEY_UP) && m_phase_frames > 15) {
                reset();
                m_phase = PLAYING;
                spawn();
            }
            return;
        }
        if (m_phase != PLAYING)
            return;
        switch (ev.key) {
        case HID_KEY_LEFT:  move(-1); break;   // auto-repeat moves on while held
        case HID_KEY_RIGHT: move(1); break;
        case HID_KEY_DOWN:
            if (fits(m_type, m_rot, m_x, m_y + 1)) {
                ++m_y;
                ++m_score;
                m_fall = 0;
            }
            break;
        case HID_KEY_UP: case HID_KEY_VOL_UP:
            if (press) rotate(1);
            break;
        case HID_KEY_VOL_DOWN:
            if (press) rotate(-1);
            break;
        case HID_KEY_ENTER:
            if (press) hard_drop();
            break;
        }
    }

    void tick() override {
        ++m_frame;
        ++m_phase_frames;
        if (m_phase == CLEARING) {
            if (m_phase_frames >= 9)
                finish_clear();
            return;
        }
        if (m_phase != PLAYING)
            return;
        if (fits(m_type, m_rot, m_x, m_y + 1)) {
            m_landed = false;
            if (++m_fall >= fall_frames()) {
                m_fall = 0;
                ++m_y;
            }
        }
        else {
            if (!m_landed) {
                m_landed = true;
                m_lock = 0;
            }
            if (++m_lock >= LOCK_FRAMES)
                lock();
        }
    }

    void draw() override {
        draw_hud();
        display.drawRect(BX - 1, BY - 1, COLS * CELL + 2, ROWS * CELL + 2, rgb565(90, 90, 110));
        for (int r = 0; r < ROWS; ++r) {
            bool flash = m_phase == CLEARING && (m_clear_rows & (1u << r)) && (m_phase_frames / 2) % 2 == 0;
            for (int c = 0; c < COLS; ++c)
                if (m_board[r][c])
                    draw_cell(c, r, flash ? rgb565(255, 255, 255) : piece_color(m_board[r][c] - 1));
        }
        if (m_phase == PLAYING) {
            for (int i = 0; i < 4; ++i) {
                int c = m_x + SHAPES[m_type][m_rot][i][0], r = m_y + SHAPES[m_type][m_rot][i][1];
                if (r >= 0)
                    draw_cell(c, r, piece_color(m_type));
            }
        }
        if (m_phase == READY)
            draw_message("TETRIS", rgb565(170, 60, 255), "OK to", "start");
        else if (m_phase == OVER) {
            char buf[12];
            snprintf(buf, sizeof(buf), "%u", (unsigned)m_score);
            draw_message("GAME", rgb565(255, 60, 60), "OVER", buf);
            if (m_new_record && (m_frame / 5) % 2 == 0)
                draw_text_center(BY + 62, "NEW HI!", rainbow(m_frame));
            if (m_phase_frames > 15 && (m_frame / 10) % 2 == 0)
                draw_text_center(BY + 74, "OK again", rgb565(60, 220, 60));
        }
    }

private:
    enum Phase { READY, PLAYING, CLEARING, OVER };
    static const int COLS = 10;
    static const int ROWS = 20;
    static const int CELL = 5;
    static const int BX = 1;   // the board in display pixels, under the score
    static const int BY = 27;
    static const int LOCK_FRAMES = 15; // half a second on the ground before it locks
    static const int MAX_LOCK_RESETS = 15;
    enum { I, O, T, S, Z, J, L };

    // [piece][rotation][cell] = {x, y} in its 4x4 box, y down. the SRS rotation states
    static constexpr int8_t SHAPES[7][4][4][2] = {
        { {{0,1},{1,1},{2,1},{3,1}}, {{2,0},{2,1},{2,2},{2,3}}, {{0,2},{1,2},{2,2},{3,2}}, {{1,0},{1,1},{1,2},{1,3}} }, // I
        { {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}} }, // O
        { {{1,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{2,1},{1,2}}, {{0,1},{1,1},{2,1},{1,2}}, {{1,0},{0,1},{1,1},{1,2}} }, // T
        { {{1,0},{2,0},{0,1},{1,1}}, {{1,0},{1,1},{2,1},{2,2}}, {{1,1},{2,1},{0,2},{1,2}}, {{0,0},{0,1},{1,1},{1,2}} }, // S
        { {{0,0},{1,0},{1,1},{2,1}}, {{2,0},{1,1},{2,1},{1,2}}, {{0,1},{1,1},{1,2},{2,2}}, {{1,0},{0,1},{1,1},{0,2}} }, // Z
        { {{0,0},{0,1},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{1,2}}, {{0,1},{1,1},{2,1},{2,2}}, {{1,0},{1,1},{0,2},{1,2}} }, // J
        { {{2,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{1,2},{2,2}}, {{0,1},{1,1},{2,1},{0,2}}, {{0,0},{1,0},{1,1},{1,2}} }, // L
    };
    // SRS wall kicks, {x, y up}: [from rotation][0 clockwise, 1 counter-clockwise][try]
    static constexpr int8_t KICKS[4][2][5][2] = {
        { {{0,0},{-1,0},{-1,1},{0,-2},{-1,-2}}, {{0,0},{1,0},{1,1},{0,-2},{1,-2}} },   // 0->1, 0->3
        { {{0,0},{1,0},{1,-1},{0,2},{1,2}},     {{0,0},{1,0},{1,-1},{0,2},{1,2}} },    // 1->2, 1->0
        { {{0,0},{1,0},{1,1},{0,-2},{1,-2}},    {{0,0},{-1,0},{-1,1},{0,-2},{-1,-2}} },// 2->3, 2->1
        { {{0,0},{-1,0},{-1,-1},{0,2},{-1,2}},  {{0,0},{-1,0},{-1,-1},{0,2},{-1,2}} }, // 3->0, 3->2
    };
    static constexpr int8_t KICKS_I[4][2][5][2] = {
        { {{0,0},{-2,0},{1,0},{-2,-1},{1,2}},   {{0,0},{-1,0},{2,0},{-1,2},{2,-1}} },  // 0->1, 0->3
        { {{0,0},{-1,0},{2,0},{-1,2},{2,-1}},   {{0,0},{2,0},{-1,0},{2,1},{-1,-2}} },  // 1->2, 1->0
        { {{0,0},{2,0},{-1,0},{2,1},{-1,-2}},   {{0,0},{1,0},{-2,0},{1,-2},{-2,1}} },  // 2->3, 2->1
        { {{0,0},{1,0},{-2,0},{1,-2},{-2,1}},   {{0,0},{-2,0},{1,0},{-2,-1},{1,2}} },  // 3->0, 3->2
    };

    Preferences* m_pref = nullptr;
    uint32_t m_hi = 0;
    Phase m_phase = READY;
    int m_frame = 0;
    int m_phase_frames = 0;
    uint8_t m_board[ROWS][COLS];  // 0 empty, else piece + 1
    int m_type = 0, m_rot = 0, m_x = 0, m_y = 0;
    int m_next = 0;
    uint8_t m_bag[7];
    int m_bag_pos = 7;
    uint32_t m_score = 0;
    int m_lines = 0;
    int m_fall = 0;          // frames since the last step down
    bool m_landed = false;
    int m_lock = 0;          // frames on the ground
    int m_lock_resets = 0;
    uint32_t m_clear_rows = 0;
    bool m_new_record = false;

    int level() const { return 1 + m_lines / 10; }

    // frames per row: 0.8 seconds at level 1, faster with every level
    int fall_frames() const {
        float ms = 800.0f * powf(0.83f, level() - 1);
        if (ms < 50)
            ms = 50;
        return max(1, (int)(ms / GamesState::FRAME_MS));
    }

    static uint16_t piece_color(int t) {
        static const uint16_t cols[7] = { rgb565(0, 220, 230), rgb565(240, 210, 0), rgb565(170, 40, 230),
                                          rgb565(40, 210, 40), rgb565(230, 30, 30), rgb565(30, 70, 255), rgb565(255, 130, 0) };
        return cols[t];
    }

    void reset() {
        memset(m_board, 0, sizeof(m_board));
        m_score = 0;
        m_lines = 0;
        m_bag_pos = 7;
        m_next = from_bag();
        m_frame = 0;
        m_phase_frames = 0;
        m_new_record = false;
    }

    // all 7 pieces in a random order, then again
    int from_bag() {
        if (m_bag_pos >= 7) {
            for (int i = 0; i < 7; ++i)
                m_bag[i] = i;
            for (int i = 6; i > 0; --i) {
                int j = random(0, i + 1);
                uint8_t t = m_bag[i]; m_bag[i] = m_bag[j]; m_bag[j] = t;
            }
            m_bag_pos = 0;
        }
        return m_bag[m_bag_pos++];
    }

    bool fits(int type, int rot, int x, int y) const {
        for (int i = 0; i < 4; ++i) {
            int c = x + SHAPES[type][rot][i][0], r = y + SHAPES[type][rot][i][1];
            if (c < 0 || c >= COLS || r >= ROWS)
                return false;
            if (r >= 0 && m_board[r][c])
                return false;
        }
        return true;
    }

    void spawn() {
        m_type = m_next;
        m_next = from_bag();
        m_rot = 0;
        m_x = 3;
        m_y = (m_type == I) ? -1 : 0;
        m_fall = 0;
        m_landed = false;
        m_lock_resets = 0;
        if (!fits(m_type, m_rot, m_x, m_y))
            game_over();
    }

    // moving or turning on the ground gives it more time before it locks, a limited number of times
    void moved() {
        if (m_landed && m_lock_resets < MAX_LOCK_RESETS) {
            m_lock = 0;
            ++m_lock_resets;
        }
    }

    void move(int dx) {
        if (fits(m_type, m_rot, m_x + dx, m_y)) {
            m_x += dx;
            moved();
        }
    }

    void rotate(int dir) {
        int to = (m_rot + (dir > 0 ? 1 : 3)) % 4;
        const int8_t (*kicks)[2] = (m_type == I) ? KICKS_I[m_rot][dir > 0 ? 0 : 1] : KICKS[m_rot][dir > 0 ? 0 : 1];
        if (m_type == O)
            return;
        for (int k = 0; k < 5; ++k) {
            int nx = m_x + kicks[k][0], ny = m_y - kicks[k][1]; // the table is y up
            if (fits(m_type, to, nx, ny)) {
                m_x = nx;
                m_y = ny;
                m_rot = to;
                moved();
                return;
            }
        }
    }

    void hard_drop() {
        while (fits(m_type, m_rot, m_x, m_y + 1)) {
            ++m_y;
            m_score += 2;
        }
        lock();
    }

    void lock() {
        bool above = true;
        for (int i = 0; i < 4; ++i) {
            int c = m_x + SHAPES[m_type][m_rot][i][0], r = m_y + SHAPES[m_type][m_rot][i][1];
            if (r >= 0) {
                m_board[r][c] = m_type + 1;
                above = false;
            }
        }
        if (above) { // locked out of the top
            game_over();
            return;
        }
        m_clear_rows = 0;
        for (int r = 0; r < ROWS; ++r) {
            bool full = true;
            for (int c = 0; c < COLS && full; ++c)
                full = m_board[r][c] != 0;
            if (full)
                m_clear_rows |= 1u << r;
        }
        if (m_clear_rows) {
            m_phase = CLEARING;
            m_phase_frames = 0;
        }
        else
            spawn();
    }

    void finish_clear() {
        int n = 0;
        for (int r = 0; r < ROWS; ++r) {
            if (!(m_clear_rows & (1u << r)))
                continue;
            ++n;
            for (int rr = r; rr > 0; --rr)
                memcpy(m_board[rr], m_board[rr - 1], COLS);
            memset(m_board[0], 0, COLS);
        }
        static const int points[5] = { 0, 100, 300, 500, 800 };
        m_score += points[n] * level();
        m_lines += n;
        m_clear_rows = 0;
        m_phase = PLAYING;
        spawn();
    }

    void game_over() {
        m_phase = OVER;
        m_phase_frames = 0;
        if (m_score > m_hi) {
            m_hi = m_score;
            m_new_record = true;
            if (m_pref)
                m_pref->putUInt("tetris_hi", m_hi);
        }
    }

    void draw_cell(int c, int r, uint16_t color) {
        int x = BX + c * CELL, y = BY + r * CELL;
        display.fillRect(x, y, CELL - 1, CELL - 1, color);
    }

    // the score big at the top, level and lines under it, the next piece on the right
    void draw_hud() {
        char buf[16];
        snprintf(buf, sizeof(buf), "%u", (unsigned)m_score);
        int size = (text_width(buf, 2) <= 64 - 14) ? 2 : 1;
        draw_text(0, size == 2 ? 1 : 4, buf, rgb565(255, 255, 255), size);
        snprintf(buf, sizeof(buf), "L%d", level());
        draw_text(0, 17, buf, rgb565(255, 210, 0));
        snprintf(buf, sizeof(buf), "%d", m_lines);
        draw_text(28, 17, buf, rgb565(120, 200, 255));
        if (m_phase == PLAYING || m_phase == CLEARING) {
            // the next piece, 2x2 pixels per cell
            for (int i = 0; i < 4; ++i)
                display.fillRect(52 + SHAPES[m_next][0][i][0] * 3, 3 + SHAPES[m_next][0][i][1] * 3, 2, 2, piece_color(m_next));
        }
    }

    void draw_message(const char* l1, uint16_t c1, const char* l2, const char* l3) {
        display.fillRect(BX + 2, BY + 30, COLS * CELL - 4, 28, 0);
        draw_text_center(BY + 32, l1, c1);
        draw_text_center(BY + 41, l2, rgb565(255, 255, 255));
        draw_text_center(BY + 50, l3, rgb565(255, 255, 255));
    }
};

constexpr int8_t TetrisGame::SHAPES[7][4][4][2];
constexpr int8_t TetrisGame::KICKS[4][2][5][2];
constexpr int8_t TetrisGame::KICKS_I[4][2][5][2];
