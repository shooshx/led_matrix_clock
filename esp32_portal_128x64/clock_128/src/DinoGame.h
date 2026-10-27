#pragma once
#include "GamesState.h"

// Dino run, like the browser's no-internet game: jump over and duck under what comes, with emojis,
// coins, stars and hearts. played in landscape
namespace dino_art {

// ---- the dino, 16x14 ----
#define DINO_TOP \
    "........GGGGGGG.", \
    ".......GGWGGGGGG", \
    ".......GGGGGGGGG", \
    ".......GGGGG....", \
    ".......GGGGGGG..", \
    "G.....GGGGG.....", \
    "G...GGGGGGGGG...", \
    "GG.GGGGGGGG.G...", \
    "GGGGGGGGGGG.....", \
    ".GGGGGGGGGG.....", \
    "..GGGGGGGG......", \
    "...GGGGGGG......"

static const char* const DINO_RUN_A[] = { DINO_TOP, "....GG.GG.......", "....G...GG......" };
static const char* const DINO_RUN_B[] = { DINO_TOP, "....GG.GG.......", "....GG...G......" };
static const char* const DINO_JUMP[]  = { DINO_TOP, "....GG.GG.......", "....G....G......" };
#undef DINO_TOP
static const Sprite S_DINO_RUN_A = { 16, 14, DINO_RUN_A };
static const Sprite S_DINO_RUN_B = { 16, 14, DINO_RUN_B };
static const Sprite S_DINO_JUMP  = { 16, 14, DINO_JUMP };

// ducking, 20x9
#define DUCK_TOP \
    "G...........GGGGGGG.", \
    "GG........GGGGGWGGGG", \
    "GGGGGGGGGGGGGGGGGGGG", \
    ".GGGGGGGGGGGGGGG....", \
    "..GGGGGGGGGGGGGGGG..", \
    "...GGGGGGGGG..GG....", \
    "....GGGGGGGG........"

static const char* const DUCK_A[] = { DUCK_TOP, "....GG..GG..........", "....G....GG........." };
static const char* const DUCK_B[] = { DUCK_TOP, "....GG..GG..........", "....GG....G........." };
#undef DUCK_TOP
static const Sprite S_DUCK_A = { 20, 9, DUCK_A };
static const Sprite S_DUCK_B = { 20, 9, DUCK_B };

// ---- obstacles ----
static const char* const CACTUS_S[] = {
    "..CC..", "..CC..", "C.CC..", "C.CC.C", "C.CC.C", "CCCC.C",
    "..CCCC", "..CC..", "..CC..", "..CC..", "..CC..", "..CC..",
};
static const Sprite S_CACTUS_S = { 6, 12, CACTUS_S };

static const char* const CACTUS_B[] = {
    "...CC...", "..CCCC..", "..CCCC..", "C.CCCC..", "C.CCCC.C", "C.CCCC.C", "CCCCCC.C", ".CCCCC.C",
    "..CCCCCC", "..CCCC..", "..CCCC..", "..CCCC..", "..CCCC..", "..CCCC..", "..CCCC..", "..CCCC..",
};
static const Sprite S_CACTUS_B = { 8, 16, CACTUS_B };

// pterodactyl, wings up and down, 14x9
static const char* const BIRD_A[] = {
    "...P..........", "...PP.........", "...PPP........", "..PPPPP.......", "YPPWPPPPPPPPP.",
    "...PPPPPPPPPPP", ".....PPPPPPP..", "..............", "..............",
};
static const char* const BIRD_B[] = {
    "..............", "..............", "..............", "..............", "YPPWPPPPPPPPP.",
    "...PPPPPPPPPPP", "...PPPPPPP....", "...PPP........", "...PP.........",
};
static const Sprite S_BIRD_A = { 14, 9, BIRD_A };
static const Sprite S_BIRD_B = { 14, 9, BIRD_B };

// ---- emojis ----
static const char* const SMILEY[] = {
    "..YYYYY..", ".YYYYYYY.", "YYKYYYKYY", "YYYYYYYYY", "YKYYYYYKY", "YYKKKKKYY", ".YYYYYYY.", "..YYYYY..",
};
static const Sprite S_SMILEY = { 9, 8, SMILEY };

static const char* const POOP[] = {
    "....N....", "...NNN...", "..NNNNN..", ".NWKNWKN.", ".NNNNNNN.", "NNKNNNKNN", "NNNKKKNNN", "NNNNNNNNN",
};
static const Sprite S_POOP = { 9, 8, POOP };

static const char* const GHOST[] = {
    "..WWWWW..", ".WWWWWWW.", "WWKWWWKWW", "WWKWWWKWW", "WWWWWWWWW", "WWWKKKWWW", "WWWWWWWWW", "WWWWWWWWW", "W.WW.WW.W",
};
static const Sprite S_GHOST = { 9, 9, GHOST };

static const char* const BALL[] = {
    "..RRRR..", ".RRWWRR.", "RWWWWWWR", "BBBBBBBB", "BBBBBBBB", "RWWWWWWR", ".RRWWRR.", "..RRRR..",
};
static const Sprite S_BALL = { 8, 8, BALL };

#define ALIEN_TOP "..M.....M..", "...M...M...", "..MMMMMMM..", ".MM.MMM.MM.", "MMMMMMMMMMM", "M.MMMMMMM.M"
static const char* const ALIEN_A[] = { ALIEN_TOP, "M.M.....M.M", "...MM.MM..." };
static const char* const ALIEN_B[] = { ALIEN_TOP, "..M.....M..", ".M.......M." };
#undef ALIEN_TOP
static const Sprite S_ALIEN_A = { 11, 8, ALIEN_A };
static const Sprite S_ALIEN_B = { 11, 8, ALIEN_B };

static const char* const ROCKET_A[] = { "....RR......", "..WWWWWWWR.O", "WWWKWWWWWWOY", "..WWWWWWWR.O", "....RR......" };
static const char* const ROCKET_B[] = { "....RR......", "..WWWWWWWR.Y", "WWWKWWWWWWYO", "..WWWWWWWR.Y", "....RR......" };
static const Sprite S_ROCKET_A = { 12, 5, ROCKET_A };
static const Sprite S_ROCKET_B = { 12, 5, ROCKET_B };

// ---- to collect ----
static const char* const STAR[] = { "...Y...", "..YYY..", "YYYYYYY", ".YYYYY.", "..YYY..", ".YY.YY.", "YY...YY" };
static const Sprite S_STAR = { 7, 7, STAR };

static const char* const HEART[] = { ".RR.RR.", "RRRRRRR", "RRRRRRR", ".RRRRR.", "..RRR..", "...R..." };
static const Sprite S_HEART = { 7, 6, HEART };
static const char* const HEART_EMPTY[] = { ".DD.DD.", "D..D..D", "D.....D", ".D...D.", "..D.D..", "...D..." };
static const Sprite S_HEART_EMPTY = { 7, 6, HEART_EMPTY };

static const char* const COIN_A[] = { ".OOO.", "OYYOO", "OYOOO", "OOOOO", ".OOO." };
static const char* const COIN_B[] = { "..O..", ".OYO.", ".OYO.", ".OOO.", "..O.." };
static const Sprite S_COIN_A = { 5, 5, COIN_A };
static const Sprite S_COIN_B = { 5, 5, COIN_B };

// ---- scenery ----
static const char* const CLOUD[] = { "....DDD.....", "..DDDDDDD...", ".DDDDDDDDDD.", "DDDDDDDDDDDD" };
static const Sprite S_CLOUD = { 12, 4, CLOUD };
static const char* const MOON[] = { ".YYY..", "YYY...", "YY....", "YY....", "YYY...", ".YYY.." };
static const Sprite S_MOON = { 6, 6, MOON };

} // namespace dino_art


class DinoGame : public Game
{
public:
    const char* name() const override { return "Dino run"; }
    const Sprite& icon() const override { return dino_art::S_DINO_RUN_A; }
    bool portrait() const override { return false; }
    uint32_t high_score() const override { return m_hi; }

    void load(Preferences& pref) override {
        m_pref = &pref;
        m_hi = pref.getUInt("dino_hi", 0);
    }

    void start() override {
        reset();
        m_phase = READY;
    }

    void on_hid(const HidEvent& ev) override {
        bool down = ev.type == HidEvent::KEY_DOWN, up = ev.type == HidEvent::KEY_UP;
        bool jump_key = ev.key == HID_KEY_UP || ev.key == HID_KEY_ENTER;
        if (ev.key == HID_KEY_DOWN && (down || up))
            m_down_held = down; // auto-repeat keeps it down
        if (jump_key && up)
            m_up_held = false;
        if (!down || ev.repeat)
            return;
        switch (m_phase) {
        case READY:
            if (jump_key) {
                m_phase = PLAYING;
                jump();
            }
            break;
        case PLAYING:
            if (jump_key)
                jump();
            break;
        case OVER:
            if (jump_key && m_phase_frames > 20) {
                reset();
                m_phase = PLAYING;
            }
            break;
        default:
            break;
        }
    }

    void tick() override {
        ++m_frame;
        ++m_phase_frames;
        update_particles();
        if (m_phase == READY) {
            move_clouds(0.3f);
            return;
        }
        if (m_phase == DYING) {
            if (m_phase_frames > 30)
                game_over();
            return;
        }
        if (m_phase == OVER)
            return;
        play_tick();
    }

    void draw() override {
        using namespace dino_art;
        draw_scenery();

        for (const Ent& e : m_ents)
            if (e.active)
                draw_ent(e);
        draw_dino();
        for (const Particle& p : m_parts)
            if (p.life > 0)
                display.drawPixel((int)p.x, (int)p.y, p.color);
        draw_hud();

        if (m_phase == READY) {
            draw_text_center(14, "DINO RUN", rgb565(255, 210, 0));
            if ((m_frame / 10) % 2 == 0)
                draw_text_center(26, "UP to jump", rgb565(255, 255, 255));
        }
        else if (m_phase == OVER) {
            char buf[20];
            draw_text_center(12, "GAME OVER", rgb565(255, 60, 60));
            snprintf(buf, sizeof(buf), "SCORE %u", (unsigned)score());
            draw_text_center(22, buf, rgb565(255, 255, 255));
            if (m_new_record && (m_frame / 5) % 2 == 0)
                draw_text_center(32, "NEW RECORD!", rainbow(m_frame));
            if (m_phase_frames > 20 && (m_frame / 10) % 2 == 0)
                draw_text_center(42, "UP to play", rgb565(60, 220, 60));
        }
    }

private:
    enum Phase { READY, PLAYING, DYING, OVER };
    enum Kind { CACTUS_S, CACTUS_B, BIRD, BALL, SMILEY, POOP, GHOST, ALIEN, ROCKET, COIN, STAR, HEART };
    struct Ent {
        bool active = false;
        Kind kind;
        float x, y;
        float vy;     // ball bounce
        float base_y; // floating ones move around it
        int age;
    };
    struct Particle {
        float x, y, vx, vy;
        int life = 0;
        uint16_t color;
    };

    static const int W = 128;
    static const int GROUND = 56;  // the ground line, things stand on GROUND - 1
    static const int DINO_X = 10;
    static constexpr float GRAVITY = 0.45f;
    static constexpr float JUMP_V = -5.6f;
    static constexpr float MAX_SPEED = 4.2f;
    static const int MAX_ENTS = 16;
    static const int MAX_PARTS = 48;
    static const int LIVES = 3;

    Preferences* m_pref = nullptr;
    uint32_t m_hi = 0;
    Phase m_phase = READY;
    int m_frame = 0;
    int m_phase_frames = 0;
    float m_score = 0;
    float m_speed = 1.6f;
    int m_lives = LIVES;
    int m_hurt = 0;       // frames of blinking after a hit, can't be hit again
    int m_star = 0;       // frames of star power
    float m_dy = 0;       // the dino's height, <= 0 is above the ground
    float m_vy = 0;
    bool m_up_held = false;
    bool m_down_held = false;
    float m_to_spawn = 60;
    float m_ground_ofs = 0;
    bool m_night = false;
    int m_milestone = 0;  // the last 100 reached
    int m_flash = 0;      // frames of the score flashing
    bool m_new_record = false;
    float m_clouds[3][2];
    Ent m_ents[MAX_ENTS];
    Particle m_parts[MAX_PARTS];

    uint32_t score() const { return (uint32_t)m_score; }
    bool on_ground() const { return m_dy >= 0 && m_vy == 0; }
    bool ducking() const { return on_ground() && m_down_held && (m_phase == PLAYING); }

    void reset() {
        m_frame = 0;
        m_phase_frames = 0;
        m_score = 0;
        m_speed = 1.6f;
        m_lives = LIVES;
        m_hurt = 0;
        m_star = 0;
        m_dy = 0;
        m_vy = 0;
        m_up_held = false;
        m_down_held = false;
        m_to_spawn = 70;
        m_night = false;
        m_milestone = 0;
        m_flash = 0;
        m_new_record = false;
        for (Ent& e : m_ents)
            e.active = false;
        for (Particle& p : m_parts)
            p.life = 0;
        for (int i = 0; i < 3; ++i) {
            m_clouds[i][0] = 20 + i * 45;
            m_clouds[i][1] = 12 + (i % 2) * 8;
        }
    }

    void jump() {
        m_up_held = true;
        if (!on_ground())
            return;
        m_vy = JUMP_V;
        dust(DINO_X + 6, GROUND - 1, 3);
    }

    // ---- game play ----

    void play_tick() {
        float sp = 1.6f + m_score / 450.0f;
        m_speed = sp < MAX_SPEED ? sp : MAX_SPEED;
        m_score += m_speed * 0.15f;

        int ms = (int)m_score / 100;
        if (ms > m_milestone) {
            m_milestone = ms;
            m_flash = 30;
            sparkle(W - 16, 4, rgb565(255, 210, 0), 10);
            if (ms % 5 == 0) // day / night every 500
                m_night = !m_night;
        }
        if (m_flash > 0)
            --m_flash;
        if (m_hurt > 0)
            --m_hurt;
        if (m_star > 0) {
            --m_star;
            if (m_frame % 2 == 0)
                emit(DINO_X + random(0, 14), GROUND - 2 - random(0, 12) + (int)m_dy, rainbow(m_frame + random(0, 6)), 0, -0.3f, 10);
        }

        // the dino, a shorter jump when Up is released early, a faster fall with Down
        if (m_vy != 0 || m_dy < 0) {
            m_vy += GRAVITY * (m_down_held ? 2.5f : 1.0f);
            if (!m_up_held && m_vy < -2.0f)
                m_vy = -2.0f;
            m_dy += m_vy;
            if (m_dy >= 0) { // landed
                m_dy = 0;
                m_vy = 0;
                dust(DINO_X + 6, GROUND - 1, 4);
            }
        }

        m_ground_ofs += m_speed;
        move_clouds(m_speed * 0.15f);

        m_to_spawn -= m_speed;
        if (m_to_spawn <= 0)
            spawn();

        for (Ent& e : m_ents)
            if (e.active)
                move_ent(e);
        collide();
    }

    float rnd() { return random(0, 1000) / 1000.0f; }

    Ent* new_ent(Kind k, float x, float y) {
        for (Ent& e : m_ents) {
            if (e.active)
                continue;
            e.active = true;
            e.kind = k;
            e.x = x;
            e.y = y;
            e.base_y = y;
            e.vy = 0;
            e.age = 0;
            return &e;
        }
        return nullptr;
    }

    // the next obstacle, more kinds as the score goes up
    void spawn() {
        struct Choice { Kind k; int weight; int min_score; };
        static const Choice choices[] = {
            { CACTUS_S, 30, 0 }, { CACTUS_B, 18, 0 }, { POOP, 16, 0 }, { BALL, 14, 0 }, { GHOST, 10, 0 },
            { BIRD, 22, 100 }, { SMILEY, 14, 100 },
            { ALIEN, 14, 250 }, { ROCKET, 10, 300 },
        };
        int total = 0;
        for (const Choice& c : choices)
            if (score() >= (uint32_t)c.min_score)
                total += c.weight;
        int r = random(0, total);
        Kind k = CACTUS_S;
        for (const Choice& c : choices) {
            if (score() < (uint32_t)c.min_score)
                continue;
            if (r < c.weight) {
                k = c.k;
                break;
            }
            r -= c.weight;
        }

        float x = W + 2;
        float gap = 55 + m_speed * 16 + random(0, 60);  // always enough room to land between them
        Ent* e = nullptr;
        switch (k) {
        case CACTUS_S:
            new_ent(CACTUS_S, x, GROUND - 12);
            if (score() >= 150 && random(0, 3) == 0) { // a group
                new_ent(random(0, 2) ? CACTUS_S : CACTUS_B, x + 7, GROUND - (random(0, 2) ? 12 : 16));
                gap += 10;
            }
            break;
        case CACTUS_B:
            new_ent(CACTUS_B, x, GROUND - 16);
            break;
        case POOP:
            new_ent(POOP, x, GROUND - 8);
            break;
        case BALL:
            e = new_ent(BALL, x, GROUND - 8);
            if (e) e->vy = -3.5f;
            break;
        case GHOST:
            new_ent(GHOST, x, GROUND - 11);
            break;
        case BIRD: {
            // low: jump, middle: duck, high: only in the way of a jump
            static const int heights[] = { GROUND - 9, GROUND - 16, GROUND - 30 };
            new_ent(BIRD, x, heights[random(0, 3)]);
            break;
        }
        case SMILEY:
            new_ent(SMILEY, x, GROUND - 15);  // its bob stays above a ducking dino
            break;
        case ALIEN:
            new_ent(ALIEN, x, GROUND - 8);
            break;
        case ROCKET:
            // comes from further away, fast, with a warning
            new_ent(ROCKET, x + 60, random(0, 2) ? GROUND - 12 : GROUND - 6);
            gap += 50;
            break;
        default:
            break;
        }

        // something to collect in the gap, up where it needs a jump
        float mid = x + gap / 2;
        int c = random(0, 100);
        if (m_lives < LIVES && c < 6)
            new_ent(HEART, mid, 28);
        else if (c < 10)
            new_ent(STAR, mid, 26);
        else if (c < 45) {
            int n = random(3, 6);
            for (int i = 0; i < n; ++i) // an arc
                new_ent(COIN, mid - n * 4 + i * 8, 32 - (int)(6 * sinf(3.14159f * (i + 0.5f) / n)));
        }
        m_to_spawn = gap;
    }

    void move_ent(Ent& e) {
        ++e.age;
        float v = m_speed;
        switch (e.kind) {
        case ROCKET:
            v *= 1.8f;
            break;
        case GHOST:
            v *= 0.8f;
            e.y = e.base_y + 1.5f * sinf(e.age * 0.15f);
            break;
        case SMILEY:
            e.y = e.base_y + 1.0f * sinf(e.age * 0.25f);
            break;
        case BALL:
            e.vy += 0.25f;
            e.y += e.vy;
            if (e.y >= GROUND - 8) {
                e.y = GROUND - 8;
                e.vy = -3.5f;
            }
            break;
        case ALIEN: { // hops
            int t = e.age % 36;
            e.y = e.base_y - (t < 12 ? sinf(3.14159f * t / 12) * 10 : 0);
            break;
        }
        case STAR: case HEART: case COIN:
            e.y = e.base_y + sinf(e.age * 0.2f);
            break;
        default:
            break;
        }
        e.x -= v;
        if (e.x < -20)
            e.active = false;
    }

    struct Box { int x0, y0, x1, y1; };
    static bool overlap(const Box& a, const Box& b) {
        return a.x0 <= b.x1 && b.x0 <= a.x1 && a.y0 <= b.y1 && b.y0 <= a.y1;
    }

    // a bit smaller than the pixels, so that close calls count as passed
    Box dino_box() const {
        if (ducking())
            return { DINO_X + 2, GROUND - 7, DINO_X + 17, GROUND - 2 };
        int top = GROUND - 14 + (int)m_dy;
        return { DINO_X + 3, top + 1, DINO_X + 12, top + 12 };
    }

    Box ent_box(const Ent& e) const {
        int x = (int)e.x, y = (int)e.y;
        switch (e.kind) {
        case CACTUS_S: return { x + 1, y + 1, x + 4, y + 11 };
        case CACTUS_B: return { x + 1, y + 1, x + 6, y + 15 };
        case BIRD:     return { x + 1, y + 3, x + 12, y + 7 };
        case BALL:     return { x + 1, y + 1, x + 6, y + 6 };
        case SMILEY:   return { x + 1, y + 1, x + 7, y + 6 };
        case POOP:     return { x + 1, y + 2, x + 7, y + 7 };
        case GHOST:    return { x + 1, y + 1, x + 7, y + 8 };
        case ALIEN:    return { x + 1, y + 1, x + 9, y + 6 };
        case ROCKET:   return { x + 1, y + 1, x + 10, y + 3 };
        case COIN:     return { x - 1, y - 1, x + 5, y + 5 };  // bigger, easy to collect
        case STAR:     return { x - 1, y - 1, x + 7, y + 7 };
        case HEART:    return { x - 1, y - 1, x + 7, y + 6 };
        }
        return { x, y, x, y };
    }

    void collide() {
        Box d = dino_box();
        for (Ent& e : m_ents) {
            if (!e.active || !overlap(d, ent_box(e)))
                continue;
            int cx = (int)e.x + 3, cy = (int)e.y + 3;
            switch (e.kind) {
            case COIN:
                m_score += 25;
                sparkle(cx, cy, rgb565(255, 210, 0), 6);
                e.active = false;
                break;
            case STAR:
                m_star = 180;
                m_score += 50;
                sparkle(cx, cy, rgb565(255, 255, 255), 14);
                e.active = false;
                break;
            case HEART:
                if (m_lives < LIVES)
                    ++m_lives;
                sparkle(cx, cy, rgb565(255, 60, 90), 12);
                e.active = false;
                break;
            default: // an obstacle
                if (m_star > 0) { // smashed
                    m_score += 15;
                    sparkle(cx, cy, rainbow(m_frame), 14);
                    e.active = false;
                }
                else if (m_hurt == 0) {
                    --m_lives;
                    sparkle(DINO_X + 8, GROUND - 8 + (int)m_dy, rgb565(255, 40, 40), 14);
                    if (m_lives <= 0) {
                        m_phase = DYING;
                        m_phase_frames = 0;
                        return;
                    }
                    m_hurt = 60;
                }
                break;
            }
        }
    }

    void game_over() {
        m_phase = OVER;
        m_phase_frames = 0;
        if (score() > m_hi) {
            m_hi = score();
            m_new_record = true;
            if (m_pref)
                m_pref->putUInt("dino_hi", m_hi);
        }
    }

    // ---- effects ----

    void emit(int x, int y, uint16_t color, float vx, float vy, int life) {
        for (Particle& p : m_parts) {
            if (p.life > 0)
                continue;
            p.x = x;
            p.y = y;
            p.vx = vx;
            p.vy = vy;
            p.life = life;
            p.color = color;
            return;
        }
    }
    void sparkle(int x, int y, uint16_t color, int n) {
        for (int i = 0; i < n; ++i) {
            float a = rnd() * 6.283f, s = 0.5f + rnd() * 1.5f;
            emit(x, y, color, cosf(a) * s, sinf(a) * s, 10 + random(0, 10));
        }
    }
    void dust(int x, int y, int n) {
        for (int i = 0; i < n; ++i)
            emit(x - random(0, 6), y, rgb565(150, 120, 90), -0.3f - rnd() * 0.8f, -0.2f - rnd() * 0.4f, 8 + random(0, 6));
    }
    void update_particles() {
        for (Particle& p : m_parts) {
            if (p.life <= 0)
                continue;
            --p.life;
            p.x += p.vx;
            p.y += p.vy;
            p.vy += 0.05f;
        }
    }
    void move_clouds(float v) {
        for (auto& c : m_clouds) {
            c[0] -= v;
            if (c[0] < -14) {
                c[0] = W + random(0, 40);
                c[1] = 10 + random(0, 14);
            }
        }
    }

    // ---- drawing ----

    void draw_scenery() {
        using namespace dino_art;
        if (m_night) {
            display.fillRect(0, 0, W, GROUND, rgb565(0, 0, 28));
            static const uint8_t stars[][2] = { {8,14}, {23,30}, {37,11}, {52,22}, {66,9}, {79,33}, {92,18}, {107,28}, {119,12}, {45,38}, {12,40}, {84,42} };
            for (int i = 0; i < 12; ++i)
                if ((m_frame / 8 + i) % 5 != 0) // twinkle
                    display.drawPixel(stars[i][0], stars[i][1], rgb565(200, 200, 230));
            draw_sprite(S_MOON, 100, 10);
        }
        for (auto& c : m_clouds)
            draw_sprite(S_CLOUD, (int)c[0], (int)c[1], m_night ? rgb565(40, 40, 70) : -1);

        display.drawFastHLine(0, GROUND, W, rgb565(170, 130, 80));
        // pebbles that move with the ground
        int ofs = (int)m_ground_ofs;
        for (int x = 0; x < W; ++x) {
            uint32_t h = (uint32_t)(x + ofs) * 2654435761u;
            if ((h >> 24) % 9 == 0)
                display.drawPixel(x, GROUND + 2 + (h >> 20) % 5, rgb565(110, 85, 60));
        }
    }

    void draw_ent(const Ent& e) {
        using namespace dino_art;
        int x = (int)e.x, y = (int)e.y;
        bool alt = (e.age / 6) % 2;
        if (x >= W) { // not here yet, only the rocket's warning
            if (e.kind == ROCKET && (m_frame / 4) % 2 == 0)
                draw_text(W - 6, y - 2, "!", rgb565(255, 40, 40));
            return;
        }
        switch (e.kind) {
        case CACTUS_S: draw_sprite(S_CACTUS_S, x, y); break;
        case CACTUS_B: draw_sprite(S_CACTUS_B, x, y); break;
        case BIRD:     draw_sprite(alt ? S_BIRD_B : S_BIRD_A, x, y); break;
        case BALL:     draw_sprite(S_BALL, x, y); break;
        case SMILEY:   draw_sprite(S_SMILEY, x, y); break;
        case POOP:     draw_sprite(S_POOP, x, y); break;
        case GHOST:    draw_sprite(S_GHOST, x, y); break;
        case ALIEN:    draw_sprite(alt ? S_ALIEN_B : S_ALIEN_A, x, y); break;
        case ROCKET:
            draw_sprite((m_frame / 2) % 2 ? S_ROCKET_B : S_ROCKET_A, x, y);
            break;
        case COIN:     draw_sprite((e.age / 5) % 2 ? S_COIN_B : S_COIN_A, x, y); break;
        case STAR:     draw_sprite(S_STAR, x, y, (e.age / 3) % 2 ? -1 : (int)rgb565(255, 255, 255)); break;
        case HEART:    draw_sprite(S_HEART, x, y); break;
        }
    }

    void draw_dino() {
        using namespace dino_art;
        if (m_hurt > 0 && (m_frame / 3) % 2 == 0)
            return; // blinking
        int color = m_star > 0 ? (int)rainbow(m_frame) : -1;
        bool dead = m_phase == DYING || m_phase == OVER;
        if (ducking()) {
            draw_sprite((m_frame / 4) % 2 ? S_DUCK_B : S_DUCK_A, DINO_X, GROUND - 9, color);
            return;
        }
        int top = GROUND - 14 + (int)m_dy;
        const Sprite* s = &S_DINO_RUN_A;
        if (dead || m_phase == READY || !on_ground())
            s = &S_DINO_JUMP;
        else if ((m_frame / 4) % 2)
            s = &S_DINO_RUN_B;
        draw_sprite(*s, DINO_X, top, color);
        if (dead) { // x eye
            uint16_t r = rgb565(255, 40, 40);
            display.drawPixel(DINO_X + 8, top, r);
            display.drawPixel(DINO_X + 10, top, r);
            display.drawPixel(DINO_X + 9, top + 1, r);
            display.drawPixel(DINO_X + 8, top + 2, r);
            display.drawPixel(DINO_X + 10, top + 2, r);
        }
        else if (m_phase == READY && (m_frame / 15) % 6 == 0) // blinks while waiting
            display.drawPixel(DINO_X + 9, top + 1, sprite_color('G'));
    }

    void draw_hud() {
        using namespace dino_art;
        for (int i = 0; i < LIVES; ++i)
            draw_sprite(i < m_lives ? S_HEART : S_HEART_EMPTY, 1 + i * 8, 1);
        char buf[16];
        snprintf(buf, sizeof(buf), "HI %05u", (unsigned)m_hi);
        draw_text(38, 1, buf, rgb565(110, 110, 130));
        if (m_flash == 0 || (m_flash / 3) % 2 == 0) {
            snprintf(buf, sizeof(buf), "%05u", (unsigned)score());
            draw_text(W - text_width(buf) - 1, 1, buf, m_flash > 0 ? rgb565(255, 210, 0) : rgb565(255, 255, 255));
        }
        if (m_star > 0) // star power left
            display.drawFastHLine(1, 9, m_star / 8, rainbow(m_frame));
    }
};
