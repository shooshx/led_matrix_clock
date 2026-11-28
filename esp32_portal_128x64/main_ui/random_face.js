"use strict";

// ---- the Random mode of a clock face ----
// every aspect of the face is by chance: which texts show the time and the date, their fonts, sizes, colors,
// outlines, where they are, and the background effect. what has to stay: the time and the date are shown,
// fully on the panel, not over each other, legible, and in their order: the texts are placed in reading
// order (lines from the top, left to right in a line), hour before minute before second, weekday before
// day / month / year.
// the texts are measured with the panel's fonts (TextMeasure) over a few sample times, so that they fit
// whatever the time shows.

const RND_INTERVAL_DEF = 300        // seconds between faces
const RND_INTERVAL_MIN = 1
const RND_INTERVAL_MAX = 24 * 3600
const RND_ATTEMPTS = 80             // random faces tried before the simple fallback one
const RND_MIN_DIGIT_H = 5           // the time's digits at least this high, smaller isn't legible

function rnd_int(a, b) { return a + Math.floor(Math.random() * (b - a + 1)) }  // a..b
function rnd_pick(arr) { return arr[Math.floor(Math.random() * arr.length)] }
function rnd_chance(p) { return Math.random() < p }
// one of the keys, by their weights
function rnd_weighted(w) {
    const keys = Object.keys(w)
    let t = Math.random() * keys.reduce((s, k)=>s + w[k], 0)
    for (const k of keys) {
        t -= w[k]
        if (t < 0)
            return k
    }
    return keys[keys.length - 1]
}

// h 0-360, s and v 0-1, as {r, g, b} 0-255
function hsv_rgb(h, s, v) {
    h = ((h % 360) + 360) % 360
    const c = v * s, x = c * (1 - Math.abs((h / 60) % 2 - 1)), m = v - c
    const [r, g, b] = h < 60 ? [c, x, 0] : h < 120 ? [x, c, 0] : h < 180 ? [0, c, x] :
                      h < 240 ? [0, x, c] : h < 300 ? [x, 0, c] : [c, 0, x]
    return {r:Math.round((r + m) * 255), g:Math.round((g + m) * 255), b:Math.round((b + m) * 255)}
}
function hue_dist(a, b) {
    const d = Math.abs(((a - b) % 360 + 360) % 360)
    return Math.min(d, 360 - d)
}

// times for the texts as wide and high as they get. every digit in every place of the hour (0-23), minute,
// second (0-59), day (1-31), month (1-12) and year (2020-2039), and every weekday: in some fonts a digit
// goes a pixel further than the others
function rnd_sample_dates() {
    const days = [30, 31, 12, 23, 14, 5, 16, 27, 18, 9]
    const months = [10, 12, 11, 3, 4, 5, 6, 7, 8, 9]
    const dates = []
    for (let i = 0; i < 10; ++i) {
        const hour = i <= 3 ? 20 + i : (i % 2 ? 10 + i : i)  // 20-23, then 4, 15, 6, 17, 8, 19
        const min = (i % 6) * 10 + i
        const sec = ((i + 3) % 6) * 10 + (9 - i)
        const year = 2020 + i + (i % 2) * 10
        dates.push(new Date(year, months[i] - 1, days[i], hour, min, sec))
    }
    for (let d = 22; d <= 28; ++d)  // a week, all the weekdays
        dates.push(new Date(2026, 10, d, 0, 0, 0))
    return dates
}
const RND_SAMPLE_DATES = rnd_sample_dates()

// what text blocks draw: their box relative to their x, y, with the fonts of the panel (the wasm gfx).
// drawn on a big display away from its edges, the fonts that draw from the baseline go above y
class TextMeasure
{
    constructor() {
        this.SIZE = 400
        this.AT = 150
        this.box = null
        const collector = { setPixel: (x, y, r, g, b)=>{
            const bx = this.box
            if (bx === null)
                this.box = {x0:x, y0:y, x1:x, y1:y}
            else {
                bx.x0 = Math.min(bx.x0, x)
                bx.y0 = Math.min(bx.y0, y)
                bx.x1 = Math.max(bx.x1, x)
                bx.y1 = Math.max(bx.y1, y)
            }
        }}
        this.gfx = Module.gfx_init_display(this.SIZE, this.SIZE)
        // kept in members: the display only has pointers to them
        this.drawer = new Module.JsDraw(collector)
        this.halo = new Module.HaloDraw(this.drawer, this.SIZE, this.SIZE)
        this.gfx.set_drawer(this.drawer)
        this.drawers = {drawer:this.drawer, halo:this.halo}
        this.cache = new Map()
    }

    // the box {x0, y0, x1, y1, w, h} of a block of the layout at x = y = 0, over the sample times. null if it
    // draws nothing (a font without these characters). the same block with the same look is measured once
    measure(layout, it) {
        const b = it.block
        const key = it.label + "," + b.font_index.v + "," + b.double_size.v + "," + b.outline.v + "," + layout.show_day.v
        if (this.cache.has(key))
            return this.cache.get(key)
        const ox = b.x.v, oy = b.y.v, oshow = b.show.v
        b.x.v = this.AT
        b.y.v = this.AT
        b.show.v = 1
        let all = null
        for (const set of this.values_of(layout, b)) {
            set()
            this.box = null
            b.draw(this.gfx, this.drawers)
            this.gfx.finish()  // the halo drawer's kept text
            const bx = this.box
            if (bx === null)
                continue
            if (all === null)
                all = {x0:bx.x0, y0:bx.y0, x1:bx.x1, y1:bx.y1}
            else {
                all.x0 = Math.min(all.x0, bx.x0)
                all.y0 = Math.min(all.y0, bx.y0)
                all.x1 = Math.max(all.x1, bx.x1)
                all.y1 = Math.max(all.y1, bx.y1)
            }
        }
        b.x.v = ox
        b.y.v = oy
        b.show.v = oshow
        let r = null
        if (all !== null)
            r = {x0:all.x0 - this.AT, y0:all.y0 - this.AT, x1:all.x1 - this.AT, y1:all.y1 - this.AT,
                 w:all.x1 - all.x0 + 1, h:all.y1 - all.y0 + 1}
        this.cache.set(key, r)
        return r
    }
}

// what a block shows, each a function that sets it for a draw:
// - a field: every value it can show, each digit can go a pixel further in some fonts
// - the time h:mm[:ss]: every hour (the left), every last digit (the right). the slots fix what's between
// - the date line: the sample dates (the heights, every weekday), and its widest and its leftmost
TextMeasure.prototype.values_of = function(layout, b) {
    const out = []
    const range = (lo, hi)=>Array.from({length: hi - lo + 1}, (_, i)=>lo + i)
    const FIELD = { hour:[0, 23], min:[0, 59], sec:[0, 59], weekday:[0, 6], day:[1, 31], month:[1, 12], year:[2020, 2039] }
    if (b instanceof FieldTextBlock) {
        const [lo, hi] = FIELD[b.kind]
        for (const v of range(lo, hi))
            out.push(()=>b.set_value(v))
    }
    else if (b instanceof TimeTextBlock) {
        const with_sec = (b === layout.time)
        for (const h of range(0, 23))
            out.push(()=>b.set_time(h, 0, 0, with_sec))
        for (const u of range(0, 9))
            out.push(()=>b.set_time(0, 50 + u, 50 + u, with_sec))
    }
    else {
        for (const d of RND_SAMPLE_DATES)
            out.push(()=>layout.set_time(d))
        out.push(()=>{ b.text = this.widest_date(layout, b) })
        out.push(()=>{ b.text = this.leftmost_date(layout, b) })
    }
    return out
}

// the date line ([weekday] d/m/yyyy) as wide as it gets in its font: its parts are as wide as they get
// separately, a date has them only together by chance
// the right edge is the advance of what's before the year (the cursor moves by each part's advance), and
// then how far the year's pixels go: the last glyph can go past its advance, by a different amount per digit
TextMeasure.prototype.widest_date = function(layout, b) {
    b.set_font(this.gfx)
    const widest = (cands)=>cands.reduce((best, c)=>this.gfx.calc_str_width(c) > this.gfx.calc_str_width(best) ? c : best)
    const nums = (lo, hi)=>Array.from({length: hi - lo + 1}, (_, i)=>"" + (lo + i))
    // the year whose pixels go furthest right, drawn alone
    let year = "2026", year_x1 = -1e9
    for (const y of nums(2020, 2039)) {
        b.text = y
        this.box = null
        b.draw(this.gfx, this.drawers)
        this.gfx.finish()
        if (this.box !== null && this.box.x1 > year_x1) {
            year_x1 = this.box.x1
            year = y
        }
    }
    const t = layout.show_day.v ? widest(DAY_NAMES) + " " : ""
    return t + widest(nums(1, 31)) + "/" + widest(nums(1, 12)) + "/" + year
}

// the date line that goes furthest left: it starts with the day (1-31, no leading 0) whose pixels go furthest
// left. with the weekday it starts with that, all of them are in the samples
TextMeasure.prototype.leftmost_date = function(layout, b) {
    let day = "1", day_x0 = 1e9
    for (let d = 1; d <= 31; ++d) {
        b.text = "" + d
        this.box = null
        b.draw(this.gfx, this.drawers)
        this.gfx.finish()
        if (this.box !== null && this.box.x0 < day_x0) {
            day_x0 = this.box.x0
            day = "" + d
        }
    }
    return (layout.show_day.v ? "Sun " : "") + day + "/1/2026"
}

let g_text_measure = null
function text_measure() {
    if (g_text_measure === null)
        g_text_measure = new TextMeasure()
    return g_text_measure
}

// a saturation by chance, mostly a strong one, sometimes from lo up (the paler ones are nice too)
function rnd_saturation(lo) {
    return rnd_chance(0.7) ? 0.85 + Math.random() * 0.15 : lo + Math.random() * (1 - lo)
}

// the background effect of a random face, shaped like ClockFace's (draw_effect() takes it)
function random_effect() {
    // none and the simple ones more often, the full-screen ones (aurora, fire, radar, plasma, lava, tunnel) less
    const weights = {}
    weights[FX_NONE] = 4; weights[FX_BORDER] = 3; weights[FX_SLIDE] = 2
    for (const e of [FX_TWINKLE, FX_FIREFLIES, FX_RAIN, FX_SNOW, FX_RIPPLES, FX_WARP])
        weights[e] = 1.5
    for (const e of [FX_AURORA, FX_FIRE, FX_RADAR, FX_PLASMA, FX_LAVA, FX_TUNNEL, FX_CUBE])
        weights[e] = 1
    const kind = parseInt(rnd_weighted(weights))
    const hue = rnd_int(0, 359)
    const s = Math.random()  // 2-64, the small ones more often
    const fx = {
        fx: {v: kind},
        fx_size: {v: 2 + Math.round(s * s * (FX_SIZE_MAX - 2))},
        fx_speed: {v: rnd_int(1, 10)},
        fx_pulse: {v: fx_can_pulse(kind) && rnd_chance(0.5) ? 1 : 0},
        // mostly straight across, sometimes slanted
        fx_angle: {v: rnd_chance(0.5) ? (rnd_chance(0.5) ? 0 : 90) : rnd_int(0, 179)},
        hue: hue,
    }
    fx.fx_color = hsv_rgb(hue, rnd_saturation(0.6), 0.5 + Math.random() * 0.5)
    return fx
}

// the item of a layout's text block, {label, block, ...}
function item_of_layout(layout, block) {
    return layout.items.find((it)=>it.block === block)
}

// a random face for a panel of w x h: {layout, fx}. layout is a ClockLayout that isn't saved
function make_random_face(w, h)
{
    const m = text_measure()
    for (let attempt = 0; attempt < RND_ATTEMPTS; ++attempt) {
        // the later attempts go for smaller texts, more of them fit
        const f = try_random_face(w, h, m, attempt / RND_ATTEMPTS)
        if (f !== null)
            return f
    }
    return fallback_random_face(w, h, m)
}

// a face by chance, null if what came out doesn't fit
function try_random_face(w, h, m, shrink)
{
    const layout = new ClockLayout("rnd", {})
    const item_of = (b)=>item_of_layout(layout, b)
    const fx = random_effect()
    const fonts = font_opts().map((o)=>parseInt(o.value))

    // the texts of the time and of the date, each in its order
    const time = []
    const tk = rnd_weighted({hms:3, hm:3, fields:3})
    if (tk == 'hms')
        time.push(item_of(layout.time))                      // Time HH:MM:SS
    else if (tk == 'hm')
        time.push(item_of(layout.time_hm))                      // Time HH:MM
    else {
        time.push(item_of(layout.hour), item_of(layout.min))     // Hour, Minute
        if (rnd_chance(0.5))
            time.push(item_of(layout.sec))                  // Second
    }
    const date = []
    if (rnd_chance(0.6)) {
        date.push(item_of(layout.date))                      // the date line
        layout.show_day.v = rnd_chance(0.5) ? 1 : 0     // with the weekday
    }
    else {
        if (rnd_chance(0.5))
            date.push(item_of(layout.weekday))                  // Weekday
        date.push(item_of(layout.day), item_of(layout.month), item_of(layout.year))  // Day, Month, Year
    }

    // the look: a font for each group (sometimes for each text), 2x, colors, outlines
    const pick_font = ()=>{
        if (rnd_chance(shrink))
            return -1  // the default font is small and legible
        return rnd_pick(fonts)
    }
    const scheme = rnd_weighted({one:2, groups:3, each:2})
    const base_hue = rnd_int(0, 359)
    const away_from_fx = (hue)=>{
        if (fx.fx.v == FX_NONE)
            return hue
        // not the effect's color, the text stands out from it
        return hue_dist(hue, fx.hue) < 60 ? hue + 180 : hue
    }
    const color_of = (group_index, item_index)=>{
        let hue = base_hue
        if (scheme == 'groups')
            hue += group_index * rnd_pick([120, 150, 180])
        else if (scheme == 'each')
            hue += item_index * rnd_int(40, 90)
        return hsv_rgb(away_from_fx(hue), rnd_saturation(0.3), 1.0)
    }
    // with an effect behind them the texts have a dark outline, legible over it. otherwise sometimes
    const outline = fx.fx.v != FX_NONE || rnd_chance(0.3)
    const outline_color = rnd_chance(0.5) ? {r:0, g:0, b:0} : hsv_rgb(base_hue + 180, 0.8, 0.25)

    let item_index = 0
    for (const [gi, group] of [time, date].entries()) {
        const group_font = pick_font()
        const dbl = rnd_chance((gi == 0 ? 0.45 : 0.15) * (1 - shrink))
        for (const it of group) {
            const b = it.block
            b.font_index.v = rnd_chance(0.25) ? pick_font() : group_font
            b.double_size.v = dbl ? 1 : 0
            const c = color_of(gi, item_index++)
            b.color.r = c.r
            b.color.g = c.g
            b.color.b = c.b
            b.outline.v = outline ? 1 : 0
            b.outline_color.r = outline_color.r
            b.outline_color.g = outline_color.g
            b.outline_color.b = outline_color.b
        }
    }

    // measured: drawn, legible, not wider than the panel
    const boxes = new Map()
    for (const it of time.concat(date)) {
        const bx = m.measure(layout, it)
        if (bx === null || bx.w > w || bx.h > h)
            return null
        boxes.set(it, bx)
    }
    for (const it of time) {
        if (boxes.get(it).h < RND_MIN_DIGIT_H * (it.block.double_size.v ? 2 : 1))
            return null
    }

    // in reading order: mostly the time first
    const seq = rnd_chance(0.8) ? time.concat(date) : date.concat(time)
    // into lines: a break between texts by chance, more likely between the time and the date
    const lines = [[seq[0]]]
    for (let i = 1; i < seq.length; ++i) {
        const same_group = time.includes(seq[i]) == time.includes(seq[i - 1])
        if (rnd_chance(same_group ? 0.3 : 0.75))
            lines.push([seq[i]])
        else
            lines[lines.length - 1].push(seq[i])
    }
    // the spaces between texts in a line, and lines that are too wide broken further
    const gap = rnd_int(2, 7)
    const line_w = (line)=>line.reduce((s, it)=>s + boxes.get(it).w, 0) + gap * (line.length - 1)
    for (let i = 0; i < lines.length; ++i) {
        // the texts at its end go to a new line after it, that is checked next
        const carry = []
        while (line_w(lines[i]) > w) {
            if (lines[i].length == 1)
                return null  // a single text wider than the panel
            carry.unshift(lines[i].pop())
        }
        if (carry.length > 0)
            lines.splice(i + 1, 0, carry)
    }
    const line_h = (line)=>Math.max(...line.map((it)=>boxes.get(it).h))
    const heights = lines.map(line_h)
    const total = heights.reduce((s, x)=>s + x, 0)
    if (total + (lines.length - 1) > h)
        return null

    // the room that's left between the lines, at the top and at the bottom, by chance
    const free = h - total - (lines.length - 1)
    const weights = []
    for (let i = 0; i <= lines.length; ++i)
        weights.push(Math.random() + (i == 0 || i == lines.length ? rnd_pick([0, 0.5, 1]) : 0.2))
    const wsum = weights.reduce((s, x)=>s + x, 0)
    const spaces = weights.map((x)=>Math.floor(free * x / wsum))
    const v_align = rnd_pick(['bottom', 'center', 'top'])
    const h_align = rnd_pick(['left', 'center', 'right', 'any', 'center'])

    let y = spaces[0]
    lines.forEach((line, li)=>{
        const lw = line_w(line), lh = heights[li]
        const room = w - lw
        let x = h_align == 'left' ? 0 : h_align == 'right' ? room : h_align == 'center' ? Math.floor(room / 2) : rnd_int(0, room)
        for (const it of line) {
            const bx = boxes.get(it)
            const top = y + (v_align == 'top' ? 0 : v_align == 'center' ? Math.floor((lh - bx.h) / 2) : lh - bx.h)
            // the block's x, y is where its box starts less the box's offset from them
            it.block.x.v = x - bx.x0
            it.block.y.v = top - bx.y0
            it.block.show.v = 1
            x += bx.w + gap
        }
        y += lh + 1 + spaces[li + 1]
    })
    return {layout: layout, fx: fx}
}

// when nothing by chance fits: the time and the date line in the default font, one under the other
function fallback_random_face(w, h, m)
{
    const layout = new ClockLayout("rnd", {})
    const item_of = (b)=>item_of_layout(layout, b)
    const fx = {fx:{v:FX_NONE}, fx_size:{v:1}, fx_speed:{v:1}, fx_pulse:{v:0}, fx_angle:{v:0}, fx_color:{r:0, g:0, b:0}, hue:0}
    const items = [item_of(layout.time), item_of(layout.date)]
    const c = hsv_rgb(rnd_int(0, 359), 0.5, 1.0)
    let y = 0
    const boxes = items.map((it)=>{
        it.block.font_index.v = -1
        it.block.color.r = c.r
        it.block.color.g = c.g
        it.block.color.b = c.b
        return m.measure(layout, it)
    })
    const total = boxes.reduce((s, b)=>s + (b ? b.h : 0), 0) + 2
    y = Math.max(0, Math.floor((h - total) / 2))
    items.forEach((it, i)=>{
        const bx = boxes[i]
        if (bx === null)
            return
        it.block.x.v = Math.floor((w - bx.w) / 2) - bx.x0
        it.block.y.v = y - bx.y0
        it.block.show.v = 1
        y += bx.h + 2
    })
    return {layout: layout, fx: fx}
}
