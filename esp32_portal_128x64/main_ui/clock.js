"use strict";

// the panel size in landscape, portrait is the other way around
const PX_LONG = 128
const PX_SHORT = 64

// the clock mode icon at the bottom left (icon_clock in ModeIcons.h), 8x8, a byte per row, the MSB on the left
const ICON_CLOCK = [0x3C, 0x42, 0x91, 0x91, 0x9D, 0x81, 0x42, 0x3C]
const ICON_SIZE = 8
const ICON_COLOR_DEF = 0x8410  // gray, as in the firmware

// the clock icon as an item of the layout: always shown, always at the bottom left, only its color can change
// (ClockFace::m_icon_land / m_icon_port in the firmware). it has no checkbox and can't be dragged
class IconBlock
{
    constructor(name, pref_json)
    {
        this.color = ColorProp.from_json(name + "_icon_col", pref_json, ICON_COLOR_DEF)
        this.show = {v:1}
        this.fixed = true
    }
    draw(gfx, drawers) {}  // drawn over the preview by draw_icon()
    add_ui(ctrl, display_cb, pref_update) {
        const line = add_div(ctrl, 'ctrl_line')
        const col_in = add_elem(line, 'input', 'clock_col_in')
        ColorEditBox.create_at(col_in, 300, (c)=>{
            this.color.set_and_update(c, pref_update)
            display_cb()
        }, {}, ColorPicker.make_hex(this.color, true))
        add_elem(line, 'span', 'cf_note').innerText = "  always shown at the bottom left, only its color can change"
    }
    // into the preview's canvas, on a black square like on the panel
    draw_icon(canvas, h) {
        const y0 = h - ICON_SIZE, c = this.color
        for (let y = 0; y < ICON_SIZE; ++y)
            for (let x = 0; x < ICON_SIZE; ++x) {
                const on = (ICON_CLOCK[y] >> (7 - x)) & 1
                canvas.setPixel(x, y0 + y, on ? c.r : 0, on ? c.g : 0, on ? c.b : 0)
            }
        return {x0: 0, y0: y0, x1: ICON_SIZE - 1, y1: h - 1}
    }
}

// the clock for one orientation: the time with and without seconds, the date line and the separate fields,
// and the clock icon. same as ClockLayout in the firmware (where the time without seconds and the icon's color
// are in ClockFace)
class ClockLayout extends Layout
{
    constructor(name, pref_json)
    {
        super()
        this.time = this.add("Time HH:MM:SS", new TimeTextBlock(name + "_t1", pref_json))
        this.time_hm = this.add("Time HH:MM", new TimeTextBlock(name + "_t3", pref_json))
        this.date = this.add("Date", new TextBlock(name + "_t2", pref_json, ""))
        this.hour = this.add("Hour", new FieldTextBlock(name + "_fh", pref_json, 'hour'))
        this.min = this.add("Minute", new FieldTextBlock(name + "_fm", pref_json, 'min'))
        this.sec = this.add("Second", new FieldTextBlock(name + "_fs", pref_json, 'sec'))
        this.weekday = this.add("Weekday", new FieldTextBlock(name + "_fw", pref_json, 'weekday'))
        this.day = this.add("Day", new FieldTextBlock(name + "_fd", pref_json, 'day'))
        this.month = this.add("Month", new FieldTextBlock(name + "_fo", pref_json, 'month'))
        this.year = this.add("Year", new FieldTextBlock(name + "_fy", pref_json, 'year'))
        this.icon = this.add("Clock icon", new IconBlock(name, pref_json))
        this.show_day = NumProp.from_json(name + "_show_day", pref_json)
    }

    set_time(d)
    {
        this.time.set_time(d.getHours(), d.getMinutes(), d.getSeconds(), true)
        this.time_hm.set_time(d.getHours(), d.getMinutes(), d.getSeconds(), false)
        let date = ""
        if (this.show_day.v)
            date += DAY_NAMES[d.getDay()] + " "
        date += d.getDate()
        date += "/" + (d.getMonth() + 1) + "/"
        date += d.getFullYear()
        this.date.text = date

        this.hour.set_value(d.getHours())
        this.min.set_value(d.getMinutes())
        this.sec.set_value(d.getSeconds())
        this.weekday.set_value(d.getDay())
        this.day.set_value(d.getDate())
        this.month.set_value(d.getMonth() + 1)
        this.year.set_value(d.getFullYear())
    }

    add_ui(ctrl, display_cb, pref_update, refresh_cb)
    {
        const opts = add_div(ctrl, 'lt_opts')
        add_checkbox_input(opts, "Date with day", this.show_day.v, (v)=>{
            this.show_day.set_and_update(v, pref_update)
            refresh_cb()
        }, 'outline_check')
        this.add_blocks_ui(ctrl, display_cb, pref_update)
    }
}

// ---- clock faces and the periods of the day ----
// a face is everything about how the clock looks: both layouts (all the texts: font, 2x, color, outline,
// position, shown...) and the background effect. the faces have names, they're in CLOCK_FACES slots
// "lf0".."lf7" (lfN_on says if the slot is used).
// the day is divided into periods, each shows one of the faces, several periods can show the same face.
// a period starts at its start minute and lasts until the next period starts, the last one goes on over
// midnight until the first one. so the periods always cover the whole day, one period is the whole day.
// the periods are in DAY_PERIODS slots "per0".."per11": perN_on, perN_start, perN_face (the face's slot).
// the order of the periods is by start, not by slot, so adding and deleting doesn't move the other slots.
// there are DAY_TYPES named types of days ("Weekday", "Off-day"...), each with its own periods: the first
// one's are "perN_*", the others' "d1perN_*", "d2perN_*"... (dtK_name, dtK_on). which type each day of the
// week is: wd0 (Sunday) .. wd6. before a day's first period starts, the last one of the day before goes on

// how a face shows the clock, same as FaceMode in the firmware
const FACE_NORMAL = 0  // the layouts and the effect
const FACE_OFF = 1     // nothing lit, the panel is dark (also no mode icon)
const FACE_RANDOM = 2  // every aspect by chance, another one every rnd_int seconds (random_face.js)
const RANDOM_BAR = 'linear-gradient(90deg, #e04040, #e0a020, #d0d020, #40c040, #30b0e0, #8060f0, #d040c0)'
const CLOCK_FACES = 8
const DAY_PERIODS = 12  // of each day type
const DAY_TYPES = 4
const DAY_TYPE_NAMES = ["Weekday", "Off-day", "Day 3", "Day 4"]
const WEEK_DAYS = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"]
const DAY_MINUTES = 24 * 60
const DRAG_SNAP = 5    // minutes, dragging a boundary on the timeline. the time inputs set any minute
const MIN_PERIOD = 1   // minutes
const FACE_NAME_MAX = 16

// the NumProp / ColorProp members of an object, in the order they were made
function props_of(obj)
{
    return Object.values(obj).filter((v)=>v instanceof NumProp || v instanceof ColorProp)
}

function minutes_str(m) {
    const h = Math.trunc(m / 60), mm = m % 60
    return (h < 10 ? "0" : "") + h + ":" + (mm < 10 ? "0" : "") + mm
}

// a text pref, sent with "UT name text"
class TextProp
{
    constructor(name, v) {
        this.name = name
        this.v = v
    }
    set_and_update(v, pref_update) {
        this.v = v
        pref_update(this.name, String(v))
    }
    static from_json(name, pref_json, def="") {
        const jv = pref_json[name]
        return new TextProp(name, (jv === undefined || jv === null) ? def : String(jv))
    }
}

const SOUND_FILES_MAX = 160  // the names of a face's sounds with the commas, like the firmware's
const SND_TIME_MAX = 24 * 3600  // seconds

// a number of seconds as a number and a unit (seconds / minutes / hours), into prop when it's changed
function add_duration_input(parent, prop, min_s, max_s, pref_update)
{
    const units = [{value:1, text:"seconds"}, {value:60, text:"minutes"}, {value:3600, text:"hours"}]
    const secs = prop.v
    const unit0 = [3600, 60, 1].find((u)=>secs % u == 0 && secs / u >= 1) || 1
    const num = add_elem(parent, 'input', 'cf_rnd_n')
    num.setAttribute('type', 'number')
    num.setAttribute('min', 1)
    num.value = secs / unit0
    const unit = add_select(parent, null, units, unit0, ()=>{ set() })
    const set = ()=>{
        let n = parseFloat(num.value)
        if (isNaN(n) || n <= 0)
            n = 1
        const v = Math.round(Math.min(max_s, Math.max(min_s, n * parseInt(unit.value))))
        num.value = Math.round(v / parseInt(unit.value) * 1000) / 1000
        if (v != prop.v)
            prop.set_and_update(v, pref_update)
    }
    num.addEventListener('change', set)
}

// a named face
class ClockFace
{
    constructor(slot, pref_json)
    {
        const n = "lf" + slot
        this.slot = slot
        this.on = NumProp.from_json(n + "_on", pref_json, 0)
        this.name = TextProp.from_json(n + "_name", pref_json, "Face " + (slot + 1))
        this.layouts = [new ClockLayout(n + "l", pref_json), new ClockLayout(n + "p", pref_json)]
        this.fx = NumProp.from_json(n + "_fx", pref_json, FX_NONE)
        this.fx_color = ColorProp.from_json(n + "_fx_col", pref_json, 0xF800)
        this.fx_size = NumProp.from_json(n + "_fx_size", pref_json, 8)
        this.fx_speed = NumProp.from_json(n + "_fx_speed", pref_json, 3)
        this.fx_pulse = NumProp.from_json(n + "_fx_pulse", pref_json, 1)
        // not used any more (every period start shows the clock, not in a game), kept like the firmware's
        this.wake = NumProp.from_json(n + "_wake", pref_json, 0)
        this.mode = NumProp.from_json(n + "_mode", pref_json, FACE_NORMAL)
        this.rnd_int = NumProp.from_json(n + "_rnd_int", pref_json, RND_INTERVAL_DEF)  // seconds, Random
        this.fx_angle = NumProp.from_json(n + "_fx_angle", pref_json, 0)  // degrees, the direction Slide moves in
        // sounds while it's on: every snd_every seconds from the start of its period, for snd_total seconds (not
        // past its end), a random one of snd_files, the names of the panel's /sounds files separated by ','
        this.snd_on = NumProp.from_json(n + "_snd_on", pref_json, 0)
        this.snd_files = TextProp.from_json(n + "_snd_files", pref_json, "")
        this.snd_every = NumProp.from_json(n + "_snd_every", pref_json, 60)
        this.snd_total = NumProp.from_json(n + "_snd_total", pref_json, 600)
        this.snd_volume = NumProp.from_json(n + "_snd_vol", pref_json, 30)  // the panel's volume when its period starts
        if (this.fx.v == FX_SKY_REMOVED)  // shows nothing
            this.fx.v = FX_NONE
    }

    dark() {
        return this.mode.v == FACE_OFF
    }

    animated() {
        return !this.dark() && fx_animated(this.fx.v, this.fx_pulse.v)
    }

    // all the props of how the face looks (not its name), in the same order in every face
    face_props() {
        const out = [this.fx, this.fx_color, this.fx_size, this.fx_speed, this.fx_pulse, this.wake, this.mode, this.rnd_int, this.fx_angle,
                     this.snd_on, this.snd_files, this.snd_every, this.snd_total, this.snd_volume]
        for (const l of this.layouts) {
            out.push(l.show_day)
            for (const it of l.items)
                out.push(...props_of(it.block))
        }
        return out
    }

    // this face looks like src. returns its prefs {name: value}, to send them in one post_prefs()
    copy_from(src) {
        const from = src.face_props(), to = this.face_props()
        const prefs = {}
        to.forEach((p, k)=>{
            if (p instanceof ColorProp) {
                p.r = from[k].r
                p.g = from[k].g
                p.b = from[k].b
                prefs[p.name] = color565(p.r, p.g, p.b)
            }
            else {
                p.v = from[k].v
                prefs[p.name] = p.v
            }
        })
        return prefs
    }

    // for the timeline and the list: the color of the time text of the landscape layout, or the first shown text
    random() {
        return this.mode.v == FACE_RANDOM
    }

    // for the timeline and the list: a css background
    css_color() {
        if (this.dark())
            return '#000000'
        if (this.random())
            return RANDOM_BAR
        const l = this.layouts[0]
        const it = l.time.show.v ? l.items[0] : (l.items.find((i)=>i.block.show.v) || l.items[0])
        const c = it.block.color
        if (c.r + c.g + c.b < 60)
            return '#555555'  // black text, would be invisible on the dark bar
        return 'rgb(' + c.r + ',' + c.g + ',' + c.b + ')'
    }
}

// a part of the day and the face it shows. the first slot is on until it's changed
class DayPeriod
{
    constructor(type, slot, pref_json)
    {
        const n = (type == 0 ? "per" : "d" + type + "per") + slot
        this.type = type
        this.slot = slot
        this.on = NumProp.from_json(n + "_on", pref_json, slot == 0 ? 1 : 0)
        this.start = NumProp.from_json(n + "_start", pref_json, 0)
        this.face = NumProp.from_json(n + "_face", pref_json, 0)
    }
}

// a named type of day and its periods
class DayType
{
    constructor(slot, pref_json)
    {
        this.slot = slot
        this.name = TextProp.from_json("dt" + slot + "_name", pref_json, DAY_TYPE_NAMES[slot])
        this.on = NumProp.from_json("dt" + slot + "_on", pref_json, slot < 2 ? 1 : 0)
        this.all_periods = []
        for (let i = 0; i < DAY_PERIODS; ++i)
            this.all_periods.push(new DayPeriod(slot, i, pref_json))
        if (!this.all_periods.some((p)=>p.on.v))
            this.all_periods[0].on.v = 1
    }
    // in their order in the day
    periods() {
        return this.all_periods.filter((p)=>p.on.v).sort((a, b)=>a.start.v - b.start.v)
    }
}


class ClockPanel
{
    constructor(prefixes, pref_json)
    {
        this.offset = NumProp.from_json("tz_offset", pref_json)
        this.faces = []
        for (let i = 0; i < CLOCK_FACES; ++i)
            this.faces.push(new ClockFace(i, pref_json))
        if (!this.faces.some((f)=>f.on.v))
            this.faces[0].on.v = 1
        this.day_types = []
        for (let k = 0; k < DAY_TYPES; ++k)
            this.day_types.push(new DayType(k, pref_json))
        this.day_types[0].on.v = 1  // there's always the first one
        this.week = []  // the day type of each day of the week, Sunday first
        for (let d = 0; d < 7; ++d)
            this.week.push(NumProp.from_json("wd" + d, pref_json, 0))
        this.sel_type = this.day_types[0]  // the day type whose periods are edited
        this.now_minute = 0
        this.active = null         // the period of the time now
        this.anim_timer = null
        this.display_cb = null
        this.pref_update = null
        this.layouts_changed = null // set by time_generic_create, makes the layout editors again
        this.rnd = null             // the random face shown: {face, made (ms), faces: [landscape, portrait] {layout, fx}}
        this.rnd_count = 0
        this.mirror_to_panel = true // "Show on panel" for the preview
        // the parts of the ui that depend on the selection, made in add_ui
        this.timeline = null
        this.days_ui = null
        this.period_ui = null
        this.face_ui = null
        this.sel_face = null
        this.update_time(()=>{})
        this.sel_type = this.day_types[this.active.type]  // today's, or yesterday's while its last period goes on
        this.sel_period = this.active          // the period whose time and face are edited
        this.sel_face = this.face_of(this.active) // the face that's edited and previewed
    }

    // the periods of the edited day type
    get all_periods() {
        return this.sel_type.all_periods
    }
    used_day_types() {
        return this.day_types.filter((t)=>t.on.v)
    }
    // the day type of a day of the week (0 Sunday). one that was deleted is the first one
    day_type_of(wday) {
        const t = this.day_types[this.week[wday].v]
        return (t !== undefined && t.on.v) ? t : this.day_types[0]
    }
    // the period on at a minute of a day of the week, by the day types of the days: before the day's first
    // period, the last one of the day before
    period_on(wday, minute) {
        const ps = this.day_type_of(wday).periods()
        let cur = null
        for (const p of ps)
            if (p.start.v <= minute)
                cur = p
        if (cur !== null)
            return cur
        const yps = this.day_type_of((wday + 6) % 7).periods()
        return yps.length > 0 ? yps[yps.length - 1] : ps[0]
    }

    // the layouts of the edited face, the previews and the layout editors show them.
    // a random face: the ones that were made by chance
    get layouts() {
        const r = this.random_now()
        return r !== null ? r.faces.map((f)=>f.layout) : this.sel_face.layouts
    }

    // the random face shown for the edited face if it's random, made when there's none or it's time for another
    random_now() {
        const f = this.sel_face
        if (f === null || !f.random())
            return null
        const r = this.rnd
        const interval = Math.min(RND_INTERVAL_MAX, Math.max(RND_INTERVAL_MIN, f.rnd_int.v)) * 1000
        if (r === null || r.face !== f || Date.now() - r.made >= interval)
            this.shuffle()
        return this.rnd
    }

    // another random face now, for the landscape and the portrait previews
    shuffle() {
        this.rnd = {face: this.sel_face, made: Date.now(),
                    faces: [make_random_face(PX_LONG, PX_SHORT), make_random_face(PX_SHORT, PX_LONG)]}
        this.rnd_count += 1
    }

    // the layouts can be edited: not of a random face
    layouts_editable() {
        return !this.sel_face.random()
    }

    used_faces() {
        return this.faces.filter((f)=>f.on.v)
    }
    // the face a period shows. one that was deleted is the first face
    face_of(p) {
        const f = this.faces[p.face.v]
        return (f !== undefined && f.on.v) ? f : this.used_faces()[0]
    }
    // the periods that show a face, of all the day types
    periods_of(f) {
        const out = []
        for (const t of this.used_day_types())
            out.push(...t.periods().filter((p)=>this.face_of(p) === f))
        return out
    }

    // the periods of the edited day type in their order in the day
    periods() {
        return this.sel_type.periods()
    }
    // the period after / before p, around the day
    next(p) {
        const ps = this.periods()
        return ps[(ps.indexOf(p) + 1) % ps.length]
    }
    prev(p) {
        const ps = this.periods()
        return ps[(ps.indexOf(p) + ps.length - 1) % ps.length]
    }
    // the minutes from p's start to its end, the next start. one period is the whole day
    length(p) {
        const n = this.next(p)
        return n === p ? DAY_MINUTES : (n.start.v - p.start.v + DAY_MINUTES) % DAY_MINUTES
    }
    // the period of a minute of the day: the last one that started, before the first is the last one of the day before
    period_at(minute) {
        const ps = this.periods()
        let cur = ps[ps.length - 1]
        for (const p of ps)
            if (p.start.v <= minute)
                cur = p
        return cur
    }

    start_time(display_cb) {
        this.display_cb = display_cb
        this.update_time(display_cb)
        window.setInterval(()=>{ this.update_time(display_cb) }, 1000)
        // the moving effects
        this.anim_timer = window.setInterval(()=>{
            const r = this.random_now()
            const fx = r !== null ? r.faces[0].fx : this.sel_face
            if (r !== null ? fx_animated(fx.fx.v, fx.fx_pulse.v) : this.sel_face.animated())
                this.display_cb()
        }, 40)
    }

    update_time(display_cb)
    {
        const ld = new Date()
        const utc = ld.getTime()
        const d = new Date(utc + 3600000*this.offset.v + ld.getTimezoneOffset()*60000)
        this.now_minute = d.getHours() * 60 + d.getMinutes()
        const active = this.period_on(d.getDay(), this.now_minute)
        const changed = active !== this.active
        this.active = active
        if (this.sel_face !== null) {
            const before = this.rnd_count
            for (const l of this.layouts)  // a random face may be made again here
                l.set_time(d)
            if (this.rnd_count != before && this.layouts_changed !== null)
                this.layouts_changed()
        }
        if (this.timeline !== null)
            this.timeline.update(changed)
        display_cb()
    }

    // the preview shows nothing: the edited face is off
    hides_layout() {
        return this.sel_face.dark()
    }

    // called for every preview before its layout is drawn: the effect of the edited face
    before_layout_draw(layout, view) {
        if (this.sel_face.dark())
            return
        const r = this.random_now()
        // the random face of this preview's size
        const rf = r !== null ? (r.faces.find((f)=>f.layout === layout) || r.faces[0]) : null  // made again meanwhile
        const fx = rf !== null ? rf.fx : this.sel_face
        if (fx.fx.v != FX_NONE)
            draw_effect(view.canvas, view.w, view.h, fx, Date.now() / 1000)
    }

    // edit the periods of another day type
    select_type(t) {
        this.sel_type = t
        const ps = t.periods()
        this.sel_period = ps.includes(this.active) ? this.active : ps[0]
        this.select_face(this.face_of(this.sel_period))
    }

    // the days: pick the one to edit, its name, new / delete
    make_days_ui(box)
    {
        const t = this.sel_type, pref_update = this.pref_update
        box.replaceChildren()

        const list = add_div(box, 'cf_face_list')
        for (const o of this.used_day_types()) {
            const chip = add_div(list, 'cf_face_chip')
            chip.setAttribute('selected', o === t)
            add_elem(chip, 'span', 'cf_face_name').innerText = o.name.v
            const n = this.week.filter((w, d)=>this.day_type_of(d) === o).length
            add_elem(chip, 'span', 'cf_face_uses').innerText = n == 0 ? " unused" : " " + n + "d"
            chip.setAttribute('title', n + " days of the week")
            chip.addEventListener('click', ()=>{ this.select_type(o) })
        }

        // the name, sent when the typing is done
        const l1 = add_div(box, 'cf_line')
        add_elem(l1, 'span', 'cf_label').innerText = "Name "
        const name_in = add_elem(l1, 'input', 'cf_name_in')
        name_in.setAttribute('type', 'text')
        name_in.setAttribute('maxlength', FACE_NAME_MAX)
        name_in.value = t.name.v
        name_in.addEventListener('input', ()=>{
            t.name.v = name_in.value
            const chip = list.children[this.used_day_types().indexOf(t)]
            if (chip)
                chip.querySelector('.cf_face_name').innerText = t.name.v
        })
        name_in.addEventListener('change', ()=>{
            const v = name_in.value.trim() || DAY_TYPE_NAMES[t.slot]
            t.name.set_and_update(v, pref_update)
            this.refresh_ui()
        })

        // a new one with the periods of this one, in one request
        const free = this.day_types.find((o)=>!o.on.v)
        const dup = add_btn(l1, 'Duplicate', ()=>{
            if (free === undefined)
                return
            const prefs = {}
            t.all_periods.forEach((p, i)=>{
                const q = free.all_periods[i]
                for (const k of ['on', 'start', 'face']) {
                    q[k].v = p[k].v
                    prefs[q[k].name] = q[k].v
                }
            })
            free.name.v = (t.name.v + " copy").slice(0, FACE_NAME_MAX)
            prefs[free.name.name] = free.name.v
            free.on.v = 1
            prefs[free.on.name] = 1
            post_prefs(prefs, (answer)=>{
                if (answer.unknown.length > 0 || answer.bad.length > 0)
                    console.error("the copy of the day, not saved:", answer.unknown, answer.bad)
            }, (err)=>{
                window.alert("The copy of the day wasn't saved: " + err + "\nReload the page to see what is saved.")
            })
            this.select_type(free)
        })
        dup.setAttribute('title', 'a new day with the periods of this one, to change')
        dup.setAttribute('disabled', free === undefined)
        // not one that a day of the week is, and not the first one (the days of deleted ones go to it)
        const used = this.week.some((w, d)=>this.day_type_of(d) === t)
        const del = add_btn(l1, 'Delete day', ()=>{
            if (used || t.slot == 0)
                return
            t.on.set_and_update(0, pref_update)
            this.select_type(this.day_types[0])
        })
        del.setAttribute('disabled', used || t.slot == 0)
        del.setAttribute('title', t.slot == 0 ? "the first day can't be deleted" :
                                  used ? "a day of the week uses it, give that day another one in the Week first" : "")
        if (free === undefined)
            add_elem(l1, 'span', 'cf_note').innerText = "at most " + DAY_TYPES + " days"

    }

    // which day each day of the week is
    make_week_ui(box)
    {
        const pref_update = this.pref_update
        box.replaceChildren()
        const l2 = add_div(box, 'cf_line')
        const opts = this.used_day_types().map((o)=>({value:o.slot, text:o.name.v}))
        WEEK_DAYS.forEach((name, d)=>{
            const cell = add_elem(l2, 'span', 'cf_week_day')
            add_select(cell, name + " ", opts, this.day_type_of(d).slot, (v)=>{
                this.week[d].set_and_update(parseInt(v), pref_update)
                this.update_time(this.display_cb)  // another period may be on now
                this.refresh_ui()
            })
        })
    }

    // edit another period, and its face
    select_period(p) {
        this.sel_period = p
        this.select_face(this.face_of(p))
    }

    // edit and preview another face
    select_face(f) {
        this.sel_face = f
        this.update_time(()=>{})
        if (this.layouts_changed !== null)
            this.layouts_changed()
        this.refresh_ui()
    }

    // the timeline and the controls of the period and the face, after something changed
    refresh_ui() {
        if (this.period_ui !== null) {
            this.days_ui()
            this.period_ui()
            this.face_ui()
            this.timeline.update(true)
        }
        if (this.display_cb !== null)
            this.display_cb()
    }

    // the controls that are not part of a layout
    add_ui(ctrl, display_cb, pref_update, send_cmd)
    {
        this.display_cb = display_cb
        this.pref_update = pref_update
        add_num_input(ctrl, "Time zone offset: ", this.offset.v, (value)=>{
            this.offset.set_and_update(value, pref_update)
            this.update_time(display_cb)
        })

        // the week: which day each day of the week is
        const wbox = add_div(ctrl, 'cf_box')
        add_div(wbox, 'cf_title').innerText = "Week"
        const week_box = add_div(wbox, 'cf_week')

        // a day: which one is edited, its name, new / delete, then its periods on the timeline
        const dbox = add_div(ctrl, 'cf_box')
        add_div(dbox, 'cf_title').innerText = "Day"
        const days_box = add_div(dbox, 'cf_days')
        this.timeline = new Timeline(add_div(dbox, 'cf_timeline'), this)
        const pbox = add_div(dbox, 'cf_period')

        const fbox = add_div(ctrl, 'cf_box')
        add_div(fbox, 'cf_title').innerText = "Clock faces"
        const face_box = add_div(fbox, 'cf_faces')

        this.days_ui = ()=>{
            this.make_week_ui(week_box)
            this.make_days_ui(days_box)
        }
        this.period_ui = ()=>{ this.make_period_ui(pbox) }
        this.face_ui = ()=>{ this.make_face_ui(face_box) }
        this.refresh_ui()
    }

    // when the selected period is, its face, split / delete
    make_period_ui(box)
    {
        const p = this.sel_period, pref_update = this.pref_update
        box.replaceChildren()
        const periods = this.periods()
        const single = periods.length == 1

        // the start and the end to the minute. the end is the start of the next period
        const l1 = add_div(box, 'cf_line')
        // to the period before / after, around the day
        if (!single) {
            const nav = (text, title, to)=>{
                const b = add_div(l1, ['tm_btn', 'cf_nav'])
                b.innerText = text
                b.setAttribute('title', title)
                b.addEventListener('click', ()=>{ this.select_period(to()) })
            }
            nav('◀', 'the period before', ()=>this.prev(p))
            add_elem(l1, 'span', 'cf_nav_pos').innerText = (periods.indexOf(p) + 1) + " of " + periods.length
            nav('▶', 'the period after', ()=>this.next(p))
        }
        const time_in = (label, get, set)=>{
            add_elem(l1, 'span', 'cf_label').innerText = label
            const t = add_elem(l1, 'input', 'cf_time')
            t.setAttribute('type', 'time')
            t.value = minutes_str(get())
            // applied when the typing is done, Enter or leaving the input. not on 'change': it comes with
            // every digit, applying it makes this ui again (the input loses the focus) and a half typed
            // time may be refused
            const apply = ()=>{
                const [hh, mm] = t.value.split(':').map((x)=>parseInt(x))
                if (isNaN(hh) || isNaN(mm) || hh * 60 + mm == get() || !set(hh * 60 + mm))
                    t.value = minutes_str(get())  // nothing new, or not possible: back to what it is
            }
            t.addEventListener('blur', apply)
            t.addEventListener('keydown', (e)=>{
                if (e.key == 'Enter')
                    apply()
            })
            return t
        }
        this.ui_from = time_in(single ? "All day, from " : "From ", ()=>p.start.v, (m)=>this.set_start(p, m))
        this.ui_to = time_in(" to ", ()=>this.next(p).start.v, (m)=>this.set_start(this.next(p), m))
        if (single)
            this.ui_to.disabled = true

        // its face
        add_elem(l1, 'span', 'cf_label').innerText = " shows "
        const opts = this.used_faces().map((f)=>({value:f.slot, text:f.name.v}))
        add_select(l1, null, opts, this.face_of(p).slot, (v)=>{
            p.face.set_and_update(parseInt(v), pref_update)
            this.select_face(this.face_of(p))
        })

        // split / delete
        const l2 = add_div(box, 'cf_line')
        const free = this.all_periods.find((o)=>!o.on.v)
        const split = add_btn(l2, 'Split', ()=>{
            if (free === undefined || this.length(p) < 2 * MIN_PERIOD)
                return
            const len = this.length(p)
            let mid = Math.round(len / 2 / DRAG_SNAP) * DRAG_SNAP
            if (mid < MIN_PERIOD || mid > len - MIN_PERIOD)
                mid = Math.floor(len / 2)
            free.start.set_and_update((p.start.v + mid) % DAY_MINUTES, pref_update)
            free.face.set_and_update(this.face_of(p).slot, pref_update)
            free.on.set_and_update(1, pref_update)
            this.select_period(free)
        })
        split.setAttribute('title', 'two periods with the same face, the second one is selected')
        split.setAttribute('disabled', free === undefined || this.length(p) < 2 * MIN_PERIOD)
        const del = add_btn(l2, 'Delete period', ()=>{
            if (single)
                return
            const before = this.prev(p)
            p.on.set_and_update(0, pref_update)
            this.select_period(before)  // it goes on until the next one now
        })
        del.setAttribute('title', 'the period before takes its time')
        del.setAttribute('disabled', single)
        if (free === undefined)
            add_elem(l2, 'span', 'cf_note').innerText = "at most " + DAY_PERIODS + " periods"
    }

    // the list of faces, the name and effect of the selected one, new / delete
    make_face_ui(box)
    {
        const f = this.sel_face, pref_update = this.pref_update
        box.replaceChildren()
        const changed = ()=>{
            this.update_time(this.display_cb)
            this.timeline.update(true)
        }

        const list = add_div(box, 'cf_face_list')
        for (const o of this.used_faces()) {
            const chip = add_div(list, 'cf_face_chip')
            chip.setAttribute('selected', o === f)
            const dot = add_elem(chip, 'span', 'cf_face_dot')
            dot.style.background = o.css_color()
            add_elem(chip, 'span', 'cf_face_name').innerText = o.name.v
            const n = this.periods_of(o).length
            add_elem(chip, 'span', 'cf_face_uses').innerText = n == 0 ? " unused" : (n == 1 ? "" : " ×" + n)
            chip.addEventListener('click', ()=>{ this.select_face(o) })
        }

        // the name, sent when the typing is done. the list, the timeline and the period's choice show it as it's typed
        const l1 = add_div(box, 'cf_line')
        add_elem(l1, 'span', 'cf_label').innerText = "Name "
        const name_in = add_elem(l1, 'input', 'cf_name_in')
        name_in.setAttribute('type', 'text')
        name_in.setAttribute('maxlength', FACE_NAME_MAX)
        name_in.value = f.name.v
        name_in.addEventListener('input', ()=>{
            f.name.v = name_in.value
            this.timeline.update(true)
            const chip = list.children[this.used_faces().indexOf(f)]
            if (chip)
                chip.querySelector('.cf_face_name').innerText = f.name.v
        })
        name_in.addEventListener('change', ()=>{
            const v = name_in.value.trim() || ("Face " + (f.slot + 1))
            f.name.set_and_update(v, pref_update)
            this.refresh_ui()
        })

        const free = this.faces.find((o)=>!o.on.v)
        const dup = add_btn(l1, 'Duplicate', ()=>{
            if (free === undefined)
                return
            // the ~150 prefs of the face in one request
            const prefs = free.copy_from(f)
            free.name.v = (f.name.v + " copy").slice(0, FACE_NAME_MAX)
            prefs[free.name.name] = free.name.v
            free.on.v = 1
            prefs[free.on.name] = 1
            post_prefs(prefs, (answer)=>{
                if (answer.unknown.length > 0 || answer.bad.length > 0)
                    console.error("the copy of the face, not saved:", answer.unknown, answer.bad)
            }, (err)=>{
                window.alert("The copy of the face wasn't saved: " + err + "\nReload the page to see what is saved.")
            })
            this.select_face(free)
        })
        dup.setAttribute('title', 'a new face like this one, to change')
        dup.setAttribute('disabled', free === undefined)
        const used = this.periods_of(f).length > 0
        const del = add_btn(l1, 'Delete face', ()=>{
            if (used || this.used_faces().length == 1)
                return
            f.on.set_and_update(0, pref_update)
            this.select_face(this.face_of(this.sel_period))
        })
        del.setAttribute('disabled', used || this.used_faces().length == 1)
        del.setAttribute('title', used ? 'a period shows it, give the period another face first' : '')
        if (free === undefined)
            add_elem(l1, 'span', 'cf_note').innerText = "at most " + CLOCK_FACES + " faces"

        // how it shows the clock
        const l_mode = add_div(box, 'cf_line')
        add_select(l_mode, "Mode ", [{value:FACE_NORMAL, text:"Normal"}, {value:FACE_OFF, text:"Off"}, {value:FACE_RANDOM, text:"Random"}], f.mode.v, (v)=>{
            f.mode.set_and_update(parseInt(v), pref_update)
            show_fx_opts()
            if (this.layouts_changed !== null)
                this.layouts_changed()  // the layout editors, or that there's nothing to edit
            changed()
        })
        const mode_note = add_elem(l_mode, 'span', 'cf_note')

        // Random: how often another face, in seconds / minutes / hours, 1 second to 24 hours
        const l_rnd = add_div(box, 'cf_line')
        add_elem(l_rnd, 'span', 'cf_label').innerText = "A new face every "
        const units = [{value:1, text:"seconds"}, {value:60, text:"minutes"}, {value:3600, text:"hours"}]
        const secs = f.rnd_int.v
        const unit0 = [3600, 60, 1].find((u)=>secs % u == 0 && secs / u >= 1) || 1
        const rnd_n = add_elem(l_rnd, 'input', 'cf_rnd_n')
        rnd_n.setAttribute('type', 'number')
        rnd_n.setAttribute('min', 1)
        rnd_n.value = secs / unit0
        const rnd_unit = add_select(l_rnd, null, units, unit0, ()=>{ set_interval() })
        const set_interval = ()=>{
            let n = parseFloat(rnd_n.value)
            if (isNaN(n) || n <= 0)
                n = 1
            const v = Math.round(Math.min(RND_INTERVAL_MAX, Math.max(RND_INTERVAL_MIN, n * parseInt(rnd_unit.value))))
            rnd_n.value = Math.round(v / parseInt(rnd_unit.value) * 1000) / 1000
            if (v != f.rnd_int.v)
                f.rnd_int.set_and_update(v, pref_update)
        }
        rnd_n.addEventListener('change', set_interval)
        const shuffle = add_btn(l_rnd, 'Shuffle', ()=>{
            this.shuffle()
            if (this.layouts_changed !== null)
                this.layouts_changed()
            changed()
        })
        shuffle.setAttribute('title', 'another random face now, in the preview')

        // the background effect of this face
        const l3 = add_div(box, 'cf_line')
        add_select(l3, "Background effect ", FX_LIST.map((e)=>({value:e.value, text:e.text})), f.fx.v, (v)=>{
            f.fx.set_and_update(parseInt(v), pref_update)
            show_fx_opts()
            changed()
        })
        const fx_opts = add_div(box, 'cf_line')
        const fx_col = add_elem(fx_opts, 'input', 'clock_col_in')
        ColorEditBox.create_at(fx_col, 300, (c)=>{
            f.fx_color.set_and_update(c, pref_update)
            changed()
        }, {}, ColorPicker.make_hex(f.fx_color, true))
        const size_label = add_elem(fx_opts, 'label', 'num_in_label')  // what the size is, by the effect
        add_num_input(fx_opts, null, f.fx_size.v, (v)=>{
            f.fx_size.set_and_update(parseInt(v), pref_update)
            changed()
        }, {min:1, max:FX_SIZE_MAX})
        const angle = add_elem(fx_opts, 'span')  // Slide only
        add_elem(angle, 'span', 'cf_label').innerText = " Angle "
        add_slider(angle, 'cf_angle', 0, 179, f.fx_angle.v, (v)=>{
            f.fx_angle.set_and_update(v, pref_update)
            changed()
        })
        const l4 = add_div(box, 'cf_line')
        add_elem(l4, 'span', 'cf_label').innerText = "Speed "
        add_slider(l4, 'cf_speed', 1, 10, f.fx_speed.v, (v)=>{
            f.fx_speed.set_and_update(v, pref_update)
            changed()
        })
        const pulse = add_div(l4, 'cf_pulse')
        add_checkbox_input(pulse, "Pulse", f.fx_pulse.v, (v)=>{
            f.fx_pulse.set_and_update(v, pref_update)
            changed()
        }, 'outline_check')
        const show_fx_opts = ()=>{
            // off: nothing. random: the effect is by chance too
            const no_fx = f.dark() || f.random()
            l3.style.display = no_fx ? 'none' : ''
            fx_opts.style.display = (no_fx || f.fx.v == FX_NONE) ? 'none' : ''
            l4.style.display = (no_fx || f.fx.v == FX_NONE) ? 'none' : ''
            const e = FX_LIST.find((e)=>e.value == f.fx.v)
            size_label.innerText = " " + (e ? e.size : "Size") + " "
            pulse.style.display = fx_can_pulse(f.fx.v) ? '' : 'none'
            angle.style.display = f.fx.v == FX_SLIDE ? '' : 'none'
            l_rnd.style.display = f.random() ? '' : 'none'
            mode_note.innerText = f.dark() ? "  nothing is lit on the panel in its periods, not even the mode icon" :
                                  f.random() ? "  everything about the face is by chance, the time and the date stay legible" : ""
        }
        show_fx_opts()

        // sounds while it's on: a sub-frame, only its checkbox while it's off
        const snd = add_div(box, ['cf_line', 'cf_sub'])
        add_checkbox_input(snd, "Play sounds", f.snd_on.v, (v)=>{
            f.snd_on.set_and_update(v, pref_update)
            show_snd()
        }, 'outline_check')
        const snd_body = add_div(snd, 'cf_sub_body')
        const show_snd = ()=>{
            snd_body.style.display = f.snd_on.v ? '' : 'none'
            snd.classList.toggle('cf_sub_closed', !f.snd_on.v)
        }
        show_snd()

        // which sounds, a random one of them each time: picked in the list, Play to hear it, Add to the face's
        const l_pick = add_div(snd_body, 'audio_row')
        add_elem(l_pick, 'span', 'cf_label').innerText = "Sound "
        const pick = add_select(l_pick, null, [{value:'', text:'loading...'}], '', ()=>{})
        pick.classList.add('audio_sound_sel')
        add_btn(l_pick, 'Play', ()=>{  // at the panel's volume
            if (pick.value)
                audio_play_now(pick.value)
        })
        add_btn(l_pick, 'Add', ()=>{
            const name = pick.value
            const cur = selected()
            if (!name || cur.includes(name))
                return
            cur.push(name)
            const text = cur.join(',')
            if (text.length > SOUND_FILES_MAX) {
                snd_note.innerText = "no room for more sounds, remove some first"
                return
            }
            snd_note.innerText = ""
            f.snd_files.set_and_update(text, pref_update)
            show_selected()
        })
        const snd_note = add_elem(l_pick, 'span', 'cf_note')

        // the face's sounds, each with a button that removes it
        const snd_list = add_div(snd_body, 'cf_snd_list')
        const selected = ()=>f.snd_files.v.split(',').map((s)=>s.trim()).filter((s)=>s.length > 0)
        let on_panel = null  // the names, once they came
        const show_selected = ()=>{
            snd_list.replaceChildren()
            const sel = selected()
            if (sel.length == 0)
                add_elem(snd_list, 'span', 'cf_note').innerText = "no sounds added yet, each time it plays a random one of them"
            for (const name of sel) {
                const chip = add_div(snd_list, 'cf_snd_chip')
                const missing = on_panel !== null && !on_panel.includes(name)
                add_elem(chip, 'span').innerText = name.replace(/\.mp3$/i, '') + (missing ? " (not on the panel)" : "")
                const x = add_elem(chip, 'span', 'cf_snd_remove')
                x.innerText = '×'
                x.setAttribute('title', 'remove')
                x.addEventListener('click', ()=>{
                    f.snd_files.set_and_update(selected().filter((s)=>s != name).join(','), pref_update)
                    snd_note.innerText = ""
                    show_selected()
                })
            }
        }
        show_selected()
        get_sounds().then((names)=>{
            on_panel = names
            pick.replaceChildren()
            for (const name of names) {
                const opt = add_elem(pick, 'option', 'combo_opt')
                opt.value = name
                opt.innerText = name.replace(/\.mp3$/i, '')
            }
            if (names.length == 0)
                add_elem(pick, 'option', 'combo_opt').innerText = 'there are no sounds on the panel'
            show_selected()
        }).catch((e)=>{
            pick.replaceChildren()
            add_elem(pick, 'option', 'combo_opt').innerText = "couldn't get the sounds"
            console.log("sounds:", e)
        })

        const l_every = add_div(snd_body, 'cf_line')
        add_elem(l_every, 'span', 'cf_label').innerText = "Every "
        add_duration_input(l_every, f.snd_every, 1, SND_TIME_MAX, pref_update)
        const l_total = add_div(snd_body, 'cf_line')
        add_elem(l_total, 'span', 'cf_label').innerText = "For "
        add_duration_input(l_total, f.snd_total, 1, SND_TIME_MAX, pref_update)
        add_elem(l_total, 'span', 'cf_note').innerText = "  from the start of its period, never past its end"
        const l_vol = add_div(snd_body, 'cf_line')
        add_elem(l_vol, 'span', 'cf_label').innerText = "Volume "
        add_slider(l_vol, 'cf_speed', 0, 100, f.snd_volume.v, (v)=>{
            f.snd_volume.set_and_update(v, pref_update)
            vol_val.innerText = v + "%"
        })
        const vol_val = add_elem(l_vol, 'span', 'cf_note')
        vol_val.innerText = f.snd_volume.v + "%"
        add_elem(l_vol, 'span', 'cf_note').innerText = "  the panel's volume is set to it when its period starts"

        const n = this.periods_of(f).length
        if (!f.random())
            add_div(box, 'cf_note').innerText = "The texts of \"" + f.name.v + "\" are edited below, the preview shows it." +
                (n > 1 ? " Changes show in all the " + n + " periods with this face." : "")
    }

    // p starts at minute m. false if another period starts then
    set_start(p, m, send=true) {
        m = ((m % DAY_MINUTES) + DAY_MINUTES) % DAY_MINUTES
        if (this.periods().some((o)=>o !== p && o.start.v == m))
            return false
        if (send)
            p.start.set_and_update(m, this.pref_update)
        else
            p.start.v = m
        this.update_time(this.display_cb)
        this.timeline.update(true)
        if (send)
            this.period_ui()  // the order may have changed
        return true
    }

    // the time inputs follow a boundary being dragged
    show_times() {
        if (this.ui_from) {
            this.ui_from.value = minutes_str(this.sel_period.start.v)
            this.ui_to.value = minutes_str(this.next(this.sel_period).start.v)
        }
    }
}

// the day as a bar, a part for every period in the color of its face, with its name. clicking a period
// selects it, the boundaries between periods can be dragged (by DRAG_SNAP minutes). a mark shows the time now
class Timeline
{
    constructor(parent, panel)
    {
        this.panel = panel
        this.bar = add_div(parent, 'cf_bar')
        const ticks = add_div(parent, 'cf_ticks')
        for (let h = 0; h <= 24; h += 3) {
            const t = add_div(ticks, 'cf_tick')
            t.style.left = (h / 24 * 100) + '%'
            t.innerText = h
        }
        this.drag = null  // {p, prev, next, orig} while a boundary is dragged
        // on the window, the bar and its handles are made again while dragging
        window.addEventListener('pointermove', (e)=>{ this.drag_move(e) })
        window.addEventListener('pointerup', ()=>{ this.drag_end() })
        window.addEventListener('pointercancel', ()=>{ this.drag_end() })
        this.update(true)
    }

    pct(m) {
        return (m / DAY_MINUTES * 100) + '%'
    }

    // made again when the periods change, only the mark of the time moves every second otherwise
    update(all) {
        const panel = this.panel
        if (all) {
            this.bar.replaceChildren()
            const ps = panel.periods()
            for (const p of ps) {
                const from = p.start.v, len = panel.length(p), face = panel.face_of(p)
                // a period over midnight is two parts, a single period is the whole bar
                const parts = (ps.length == 1) ? [[0, DAY_MINUTES]] :
                              (from + len <= DAY_MINUTES) ? [[from, len]] : [[from, DAY_MINUTES - from], [0, from + len - DAY_MINUTES]]
                for (const [s, l] of parts) {
                    const seg = add_div(this.bar, 'cf_seg')
                    seg.style.left = this.pct(s)
                    seg.style.width = this.pct(l)
                    seg.style.background = face.css_color()
                    seg.setAttribute('selected', p === panel.sel_period)
                    seg.setAttribute('face', face === panel.sel_face)  // the periods of the edited face
                    seg.setAttribute('title', face.name.v + "  " + minutes_str(from) + " - " + minutes_str(panel.next(p).start.v) +
                                     (p === panel.active ? "  (now)" : ""))
                    if (l >= 60)  // room for the name
                        add_elem(seg, 'span', 'cf_seg_label').innerText = face.name.v
                    seg.addEventListener('click', ()=>{ if (this.drag === null) panel.select_period(p) })
                }
                if (ps.length > 1)
                    this.add_handle(p)
            }
            this.now = add_div(this.bar, 'cf_now')
            this.now.setAttribute('title', 'now')
        }
        this.now.style.left = this.pct(panel.now_minute)
    }

    // the boundary at the start of p
    add_handle(p) {
        const panel = this.panel
        const h = add_div(this.bar, 'cf_handle')
        h.style.left = this.pct(p.start.v)
        h.setAttribute('title', minutes_str(p.start.v) + ", drag to move")
        h.addEventListener('pointerdown', (e)=>{
            e.preventDefault()
            e.stopPropagation()
            this.drag = {p:p, prev:panel.prev(p), next:panel.next(p), orig:p.start.v}
        })
    }

    drag_move(e) {
        const d = this.drag, panel = this.panel
        if (d === null)
            return
        const r = this.bar.getBoundingClientRect()
        let m = Math.round((e.clientX - r.left) / r.width * DAY_MINUTES / DRAG_SNAP) * DRAG_SNAP
        m = Math.min(DAY_MINUTES - 1, Math.max(0, m))
        // between the period before and the one after, around the day
        const span = (d.next === d.prev) ? DAY_MINUTES : (d.next.start.v - d.prev.start.v + DAY_MINUTES) % DAY_MINUTES
        let off = (m - d.prev.start.v + DAY_MINUTES) % DAY_MINUTES
        off = Math.min(span - MIN_PERIOD, Math.max(MIN_PERIOD, off))
        m = (d.prev.start.v + off) % DAY_MINUTES
        if (m != d.p.start.v) {
            panel.set_start(d.p, m, false)  // sent when it's let go
            panel.show_times()
        }
    }

    drag_end() {
        const d = this.drag
        if (d === null)
            return
        // the click that ends the drag doesn't select a period
        window.setTimeout(()=>{ this.drag = null }, 0)
        if (d.p.start.v != d.orig)
            this.panel.set_start(d.p, d.p.start.v, true)
    }
}

// a selector between the landscape and the portrait layout of a panel, each with its own preview
// where the texts can be selected by clicking and moved by dragging
function time_generic_create(parent, pref_json, ws, PanelCls, prefixes, portrait)
{
    // a text pref is "UT name text", the text to the end of the line
    const send_update = (name, value)=>{
        const cmd = (typeof value == 'string' ? "UT " : "U ") + name + " " + value
        ws.send(cmd)
    }
    const send_cmd = (v)=>{
        ws.send(v)
    }
    const panel = new PanelCls(prefixes, pref_json)

    const shared = add_div(parent, 'sect_shared')
    const sel = add_div(parent, 'orient_sel')
    const views = []
    const sizes = [[PX_LONG, PX_SHORT], [PX_SHORT, PX_LONG]]
    for (let i = 0; i < 2; ++i) {
        const [w, h] = sizes[i]
        const top = add_div(parent, 'sect_top')
        const disp = add_div(top, 'sect_disp')
        const ctrl = add_div(top, 'sect_ctrl')
        const canvas = new GfxCanvas(w, h, 5, disp, 0, 'timer_canvas')
        // the pixels drawn while a text block is drawn make its bounding box, for selecting it on the preview
        const tracker = {cur:null}
        const set_pixel = canvas.setPixel.bind(canvas)
        canvas.setPixel = (x, y, r, g, b)=>{
            const it = tracker.cur
            if (it !== null) {
                if (it.bbox === null)
                    it.bbox = {x0:x, y0:y, x1:x, y1:y}
                else {
                    it.bbox.x0 = Math.min(it.bbox.x0, x)
                    it.bbox.y0 = Math.min(it.bbox.y0, y)
                    it.bbox.x1 = Math.max(it.bbox.x1, x)
                    it.bbox.y1 = Math.max(it.bbox.y1, y)
                }
            }
            set_pixel(x, y, r, g, b)
        }
        const drawer = new Module.JsDraw(canvas)
        const gfx = Module.gfx_init_display(w, h)
        gfx.set_drawer(drawer)
        views.push({top:top, ctrl:ctrl, canvas:canvas, gfx:gfx, w:w, h:h, tracker:tracker,
                    drawers:{drawer:drawer, halo:new Module.HaloDraw(drawer, w, h)}})
    }

    // not drawn while the tab is hidden: the clock every second, the timer and the stop-watch every 100ms,
    // two previews each, made the other tabs stutter (dashed lines in the draw tab). drawn when it's opened
    const display = ()=>{
        if (parent.offsetParent === null)
            return
        for (let i = 0; i < views.length; ++i) {
            const v = views[i]
            const layout = panel.layouts[i]
            v.gfx.clear()
            if (panel.before_layout_draw)
                panel.before_layout_draw(layout, v)  // the clock face's effect
            if (panel.hides_layout && panel.hides_layout()) {  // a clock face that's off
                for (const it of layout.items)
                    it.bbox = null  // nothing to select on it
            }
            else
                layout.draw(v.gfx, v.drawers, v.tracker)
            v.gfx.finish()
            // the clock icon, over everything like on the panel. a random face's is the edited face's
            if (layout.icon && !(panel.hides_layout && panel.hides_layout())) {
                const own = panel.sel_face ? panel.sel_face.layouts[i].icon : layout.icon
                const it = layout.items.find((o)=>o.block === layout.icon)
                it.bbox = own.draw_icon(v.canvas, v.h)
            }
            v.canvas.draw()
            // mark the selected text
            const it = layout.selected
            if (it !== null && it.bbox !== null) {
                const ctx = v.canvas.ctx, sc = v.canvas.scale
                ctx.save()
                ctx.strokeStyle = '#ff40ff'
                ctx.lineWidth = 2
                ctx.setLineDash([6, 4])
                ctx.strokeRect((it.bbox.x0 - 1) * sc, (it.bbox.y0 - 1) * sc,
                               (it.bbox.x1 - it.bbox.x0 + 3) * sc, (it.bbox.y1 - it.bbox.y0 + 3) * sc)
                ctx.restore()
            }
        }
    }
    const refresh = ()=>{ panel.update_time(display) }

    const sel_btns = []
    let cur_view = 0
    const select = (idx)=>{
        cur_view = idx
        for (let i = 0; i < views.length; ++i) {
            views[i].top.style.display = (i == idx) ? '' : 'none'
            sel_btns[i].setAttribute('selected', i == idx)
        }
    }
    for (const [i, label] of ["Landscape", "Portrait"].entries()) {
        const btn = add_div(sel, 'orient_btn')
        btn.innerText = label + " layout"
        btn.addEventListener('click', ()=>{ select(i) })
        sel_btns.push(btn)
    }
    add_elem(sel, 'span', 'orient_note').innerText = "the panel is now in " + (portrait ? "portrait" : "landscape")
    if (panel.mirror_to_panel)  // the clock: its preview can be shown on the panel
        add_panel_mirror(sel, panel, ws)

    panel.add_ui(shared, display, send_update, send_cmd)
    // the layout editors. the clock makes them again for the face of another period, with the same text selected
    const make_layout_ui = ()=>{
        for (let i = 0; i < views.length; ++i) {
            const prev = views[i].shown_layout
            const label = (prev && prev.selected) ? prev.selected.label : null
            const layout = panel.layouts[i]
            views[i].ctrl.replaceChildren()
            if (panel.layouts_editable && !panel.layouts_editable()) {
                add_div(views[i].ctrl, 'cf_note').innerText = "A random face, made by chance: nothing to edit here. Shuffle shows another one."
                continue  // shown_layout stays the last edited one, its selected text is selected again later
            }
            layout.add_ui(views[i].ctrl, display, send_update, refresh)
            const same = layout.items.find((it)=>it.label == label)
            if (same !== undefined)
                layout.select(same)
            views[i].shown_layout = layout
        }
    }
    make_layout_ui()
    panel.layouts_changed = make_layout_ui
    for (let i = 0; i < views.length; ++i)
        add_drag(views[i], ()=>panel.layouts[i], ()=>!panel.layouts_editable || panel.layouts_editable())

    // arrow keys move the selected text of the view that's visible, shift moves by 5
    document.addEventListener('keydown', (e)=>{
        const v = views[cur_view]
        if (v.canvas.canvas.offsetParent === null) // this tab isn't shown
            return
        const tag = (e.target && e.target.tagName) ? e.target.tagName.toLowerCase() : ''
        if (tag == 'input' || tag == 'select' || tag == 'textarea')
            return
        const d = {ArrowLeft:[-1, 0], ArrowRight:[1, 0], ArrowUp:[0, -1], ArrowDown:[0, 1]}[e.key]
        const layout = panel.layouts[cur_view]
        if (d === undefined || layout.selected === null)
            return
        e.preventDefault()
        const step = e.shiftKey ? 5 : 1
        const b = layout.selected.block
        layout.move_selected(b.x.v + d[0] * step, b.y.v + d[1] * step, true)
    })

    select(portrait ? 1 : 0)
    panel.start_time(display)
    display()
    // the tab's section was opened (create_section in main.html), it wasn't drawn while hidden
    const section = parent.closest('.section_elem')
    if (section !== null)
        section.addEventListener('section_shown', display)
}

// "Show on panel": the panel shows its clock as if it's the middle of the selected period of the edited day type
// ("SM <minute> <day type>"), with
// that period's face, drawn by the panel itself. sent again when another period is selected or it moves, and
// every SIM_RESEND_MS: the panel goes back to the real time if it doesn't hear it for a while (the page closed).
// only when the page is from the panel, there's no panel on localhost (the py_server)
const SIM_RESEND_MS = 10000  // the panel keeps it for 30 seconds, ClockPanel::SIMULATE_MS
const SECTION_CLOCK_N = 1    // as in the firmware

function add_panel_mirror(parent, panel, ws)
{
    if (['localhost', '127.0.0.1', ''].includes(window.location.hostname))
        return
    const cont = add_div(parent, 'mirror_cont')
    let on = false
    let sent_minute = -1, sent_type = -1, sent_at = 0

    const check = add_elem(cont, 'input', 'in_checkbox')
    check.setAttribute('type', 'checkbox')
    const lbl = add_elem(cont, 'label', 'in_check_label')
    lbl.innerText = "Show on panel"
    const id = 'check_' + running_id++
    check.setAttribute('id', id)
    lbl.setAttribute('for', id)
    const note = add_elem(cont, 'span', 'mirror_note')

    // the middle of the selected period
    const minute = ()=>{
        const p = panel.sel_period
        return (p.start.v + Math.floor(panel.length(p) / 2)) % DAY_MINUTES
    }
    const send = ()=>{
        sent_minute = minute()
        sent_type = panel.sel_type.slot
        sent_at = Date.now()
        ws.send("SM " + sent_minute + " " + panel.sel_type.slot)
        note.innerText = " as if it's " + minutes_str(sent_minute) + " of a " + panel.sel_type.name.v
    }
    const set_on = (v)=>{
        on = v
        check.checked = v
        if (v) {
            ws.send("U active_section " + SECTION_CLOCK_N)  // the clock, also if the panel was off
            send()
        }
        else {
            ws.send("SM -1")
            note.innerText = ""
        }
    }
    check.addEventListener('change', ()=>{ set_on(check.checked) })

    window.setInterval(()=>{
        if (!on)
            return
        if (cont.offsetParent === null) {  // the clock tab was left, the panel shows the real time again
            set_on(false)
            return
        }
        if (minute() != sent_minute || panel.sel_type.slot != sent_type || Date.now() - sent_at >= SIM_RESEND_MS)
            send()
    }, 300)
}

// click on the preview selects the text there, dragging moves it. get_layout() is the layout shown now,
// editable() says if it can be changed now (a random face can't)
function add_drag(view, get_layout, editable=()=>true)
{
    const el = view.canvas.canvas
    // panel pixel of a mouse / touch position
    const to_px = (e)=>{
        const r = el.getBoundingClientRect()
        return [Math.floor((e.clientX - r.left) * view.w / r.width), Math.floor((e.clientY - r.top) * view.h / r.height)]
    }
    let drag = null
    let layout = get_layout()
    const start = (e)=>{
        if (!editable())
            return false
        layout = get_layout()
        const [px, py] = to_px(e)
        const it = layout.hit(px, py)
        if (it === null)
            return false
        layout.select(it)
        if (it.block.fixed)  // the clock icon: selected, not moved
            return false
        drag = {sx:px, sy:py, ox:it.block.x.v, oy:it.block.y.v, moved:false}
        return true
    }
    const move = (e)=>{
        const [px, py] = to_px(e)
        if (drag === null) {
            const hov = editable() ? get_layout().hit(px, py) : null
            el.style.cursor = hov === null ? 'default' : hov.block.fixed ? 'pointer' : 'move'
            return
        }
        const nx = drag.ox + px - drag.sx, ny = drag.oy + py - drag.sy
        const b = layout.selected.block
        if (nx == b.x.v && ny == b.y.v)
            return
        drag.moved = true
        layout.move_selected(nx, ny, false)  // sent to the panel when the drag ends
    }
    const end = ()=>{
        if (drag !== null && drag.moved) {
            const b = layout.selected.block
            layout.move_selected(b.x.v, b.y.v, true)
        }
        drag = null
    }
    el.addEventListener('mousedown', (e)=>{
        if (start(e))
            e.preventDefault()
    })
    el.addEventListener('mousemove', (e)=>{ if (drag === null) move(e) })  // hover cursor
    window.addEventListener('mousemove', (e)=>{ if (drag !== null) move(e) })  // also outside the canvas
    window.addEventListener('mouseup', end)
    el.addEventListener('touchstart', (e)=>{
        if (start(e.touches[0]))
            e.preventDefault()  // don't scroll the page while dragging a text
    }, {passive:false})
    el.addEventListener('touchmove', (e)=>{
        if (drag !== null) {
            e.preventDefault()
            move(e.touches[0])
        }
    }, {passive:false})
    el.addEventListener('touchend', end)
    el.addEventListener('touchcancel', end)
}

function clock_create(parent, pref_json, ws, portrait)
{
    time_generic_create(parent, pref_json.clock, ws, ClockPanel, ["p0", "cp"], portrait)
}

// ---------------------------------------------------------------------------------------------------------

// [hours, minutes, seconds, tenths] of a time in milliseconds
function separate_time(d)
{
    const h = Math.trunc(d / (60*60*1000))
    d -= h * (60*60*1000)
    const m = Math.trunc(d / (60*1000))
    d -= m * (60*1000)
    const s = Math.trunc(d / 1000)
    d -= s * 1000
    const ms = Math.trunc(d / 100)
    return [h, m, s, ms]
}

function format_time_sp(d)
{
    const [h, m, s, ms] = separate_time(d)

    let shour = null, smin = null, ssec = null, stsec = null
    if (h > 0) {
        shour = "" + h
    }
    if (h > 0 && m < 10)
        smin = "0"
    else
        smin = ""
    smin += m
    if (s < 10)
        ssec = "0"
    else
        ssec = ""
    ssec += s
    stsec = "" + ms
    return [shour, smin, ssec, stsec]
}

// the texts of a timer / stop-watch for one orientation: the combined h:mm:ss.t line and the separate fields
// same as TimeLayout in the firmware
class TimeLayout extends Layout
{
    constructor(name, pref_json)
    {
        super()
        this.text = this.add("Time", new ClockTextBlock(name + "_t1", pref_json))
        this.hour = this.add("Hours", new FieldTextBlock(name + "_fh", pref_json, 'hour', true))
        this.min = this.add("Minutes", new FieldTextBlock(name + "_fm", pref_json, 'min'))
        this.sec = this.add("Seconds", new FieldTextBlock(name + "_fs", pref_json, 'sec'))
        this.tenth = this.add("Tenths", new FieldTextBlock(name + "_ft", pref_json, 'tenth'))
    }

    set_time(msec)
    {
        const [sh, sm, ss, st] = format_time_sp(msec)
        this.text.set_time(sh, sm, ss, st)
        const [h, m, s, t] = separate_time(msec)
        this.hour.set_value(h)
        this.min.set_value(m)
        this.sec.set_value(s)
        this.tenth.set_value(t)
    }

    add_ui(ctrl, display_cb, pref_update, refresh_cb)
    {
        this.add_blocks_ui(ctrl, display_cb, pref_update)
    }
}

class TimerPanel
{
    constructor(prefixes, pref_json) {
        const name = prefixes[0]
        this.layouts = prefixes.map((p)=>new TimeLayout(p, pref_json))
        this.hours = NumProp.from_json(name + "_hours", pref_json)
        this.min = NumProp.from_json(name + "_min", pref_json)
        this.sec = NumProp.from_json(name + "_sec", pref_json)
        this.snd_vol = NumProp.from_json(name + "_snd_vol", pref_json)
        this.snd_file_num = NumProp.from_json(name + "_snd_file_num", pref_json)

        this.running = false
        this.dest_time_msec = 0 // while running this is the current destimation
        this.cur_diff_msec = 0
    }

    read_input() {
        return ((this.hours.v * 60 + this.min.v) * 60 + this.sec.v) * 1000;
    }

    toggle_run() {
        if (!this.running) {
            if (this.cur_diff_msec == 0)
                this.cur_diff_msec = this.read_input()
            this.dest_time_msec = new Date().getTime() + this.cur_diff_msec
            this.running = true
        }
        else {
            this.running = false
        }
    }

    start_time(display_cb) {
        this.set_time(display_cb)
        window.setInterval(()=>{ this.update_time(display_cb) }, 100)
    }

    // called when the input is changed
    set_time(display_cb) {
        this.cur_diff_msec = this.read_input()
        this.update_time(display_cb)
    }

    update_time(display_cb)
    {
        if (this.running) {
            const now = new Date().getTime()
            this.cur_diff_msec = this.dest_time_msec - now
            if (this.cur_diff_msec < 0) {
                this.cur_diff_msec = 0
                this.running = false
            }
        }
        for (const l of this.layouts)
            l.set_time(this.cur_diff_msec)
        display_cb()
    }

    // the controls that are not part of a layout
    add_ui(ctrl, display_cb, pref_update, send_cmd)
    {
        const sndcont = add_div(ctrl, 't_col_snd')
        const sline1 = add_div(sndcont, 'ctrl_line')
        add_num_input(sline1, "Volume:", this.snd_vol.v, (value)=>{
            this.snd_vol.set_and_update(value, pref_update)
        })
        const sline2 = add_div(sndcont, 'ctrl_line_last')
        add_num_input(sline2, "File-Num:", this.snd_file_num.v, (value)=>{
            this.snd_file_num.set_and_update(value, pref_update)
        })
        add_btn(sline2, 'Play', ()=>{
            send_cmd("TP")
        })

        const tset = add_div(ctrl, 't_set_time')
        add_div(tset, 'tset_label').innerText = 'Set Time:'
        add_num_input(tset, null, this.hours.v, (value)=>{
            this.hours.set_and_update(value, pref_update)
            this.set_time(display_cb)
        }, {min:0})
        add_num_input(tset, null, this.min.v, (value)=>{
            this.min.set_and_update(value, pref_update)
            this.set_time(display_cb)
        }, {min:0, max:59})
        add_num_input(tset, null, this.sec.v, (value)=>{
            this.sec.set_and_update(value, pref_update)
            this.set_time(display_cb)
        }, {min:0, max:59})

        add_btn(ctrl, 'Toggle', ()=>{
            this.toggle_run()
            const cmd = "TT " + (this.running ? 1 : 0)
            send_cmd(cmd)
        })
    }
}

function timer_create(parent, pref_json, ws, portrait)
{
    time_generic_create(parent, pref_json.timer, ws, TimerPanel, ["tm", "tp"], portrait)
}

// ---------------------------------------------------------------------------------------------------------

class StopWPanel
{
    constructor(prefixes, pref_json) {
        this.layouts = prefixes.map((p)=>new TimeLayout(p, pref_json))
        this.running = false
        this.start_time_msec = -1
        this.cur_diff_msec = 0
    }

    start_time(display_cb) {
        this.update_time(display_cb)
        window.setInterval(()=>{ this.update_time(display_cb) }, 100)
    }

    toggle_run() {
        if (!this.running) {
            if (this.start_time_msec == -1)
                this.start_time_msec = new Date().getTime()
            else
                this.start_time_msec = new Date().getTime() - this.cur_diff_msec // as if it was started diff msec ago
        }
        this.running = !this.running
    }
    reset() {
        this.cur_diff_msec = 0
        if (!this.running) {
            this.start_time_msec = -1
        }
        else {
            this.start_time_msec = new Date().getTime()
        }
    }

    update_time(display_cb)
    {
        if (this.running) {
            const now = new Date().getTime()
            this.cur_diff_msec = now - this.start_time_msec
        }
        for (const l of this.layouts)
            l.set_time(this.cur_diff_msec)
        display_cb()
    }

    // the controls that are not part of a layout
    add_ui(ctrl, display_cb, pref_update, send_cmd)
    {
        const colbtn = add_div(ctrl, 'sw_btns')
        add_btn(colbtn, 'Toggle', ()=>{
            this.toggle_run()
            const cmd = "TS " + (this.running ? 1 : 0)
            send_cmd(cmd)
        })
        add_btn(colbtn, 'Reset', ()=>{
            this.reset()
            send_cmd("TR")
        })
    }
}

function stop_watch_create(parent, pref_json, ws, portrait)
{
    time_generic_create(parent, pref_json.stopw, ws, StopWPanel, ["sw", "sp"], portrait)
}

// ---------------------------------------------------------------------------------------------------------

class MemImage
{
    constructor(w, h) {
        this.d_rgb = new Uint8Array(w * h * 3)
        this.w = w
        this.h = h
    }
    set(x, y, r, g, b) {
        if (x < 0 || y < 0 || x >= this.w || y >= this.h)
            return
        const i = (x + y * this.w) * 3
        this.d_rgb[i] = r
        this.d_rgb[i+1] = g
        this.d_rgb[i+2] = b
    }
    get(x, y) {
        const i = (x + y * this.w) * 3
        return [this.d_rgb[i], this.d_rgb[i + 1], this.d_rgb[i + 2]]
    }
}

class LinearGradient
{
    constructor(col1, coord1, col2, coord2, w)
    {
        this.color1 = col1
        this.color2 = col2
        this.coord1 = coord1
        this.coord2 = coord2
        this.width = w
    }
    
    add_ui(ctrl, display_cb, send_update_cb)
    {   
        const ctrl_line1 = add_div(ctrl, 'ctrl_line')
        const col1_in = add_elem(ctrl_line1, 'input', 'clock_col_in')
        ColorEditBox.create_at(col1_in, 300, (c)=>{ 
            this.color1 = {r:c.r, g:c.g, b:c.b}
            display_cb()
            send_update_cb()
        }, {}, ColorPicker.make_hex(this.color1, true))

        const col2_in = add_elem(ctrl_line1, 'input', 'clock_col_in')
        ColorEditBox.create_at(col2_in, 300, (c)=>{ 
            this.color2 = {r:c.r, g:c.g, b:c.b}
            display_cb()
            send_update_cb()
        }, {}, ColorPicker.make_hex(this.color2, true))
    }

    add_to_image(img)
    {
        const levels = (this.coord2.x - this.coord1.x)
        const d_r = (this.color2.r - this.color1.r) / levels
        const d_g = (this.color2.g - this.color1.g) / levels
        const d_b = (this.color2.b - this.color1.b) / levels
        for(let i = 0; i <= levels; ++i) {
            const r = this.color1.r + i*d_r
            const g = this.color1.g + i*d_g
            const b = this.color1.b + i*d_b
            const x = this.coord1.x + i
            for(let j = 0; j < this.width; ++j) {
                const y = this.coord1.y + j
                img.set(x, y, r, g, b)
            }
        }
    }

}
class DImgPanel
{
    constructor(drawer, gfx, w, h)
    {
        this.drawer = drawer
        this.gfx = gfx
        gfx.set_drawer(drawer)
        this.w = w
        this.h = h

        const band = Math.floor(h / 3), right = w - 1
        this.objs = [  new LinearGradient({r:255, g:0, b:0}, {x:0, y:0}, {r:0, g:0, b:0}, {x:right, y:0}, band),
                       new LinearGradient({r:0, g:255, b:0}, {x:0, y:band}, {r:0, g:0, b:0}, {x:right, y:band}, band),
                       new LinearGradient({r:0, g:0, b:255}, {x:0, y:band*2}, {r:0, g:0, b:0}, {x:right, y:band*2}, band)
        ]
        this.img = null
    }
    make_image(w, h) {
        this.img = new MemImage(w, h) 
        for(let obj of this.objs)
            obj.add_to_image(this.img)
        
    }
    add_ui(ctrl, display_cb, send_update) {
        for(let obj of this.objs)
            obj.add_ui(ctrl, display_cb, send_update)
    }
    draw()
    {
        const w = this.w, h = this.h
        this.make_image(w, h)
        for(let x = 0; x < w; ++x)
            for(let y = 0; y < h; ++y) {
                const [r, g, b] = this.img.get(x, y)
                this.drawer.setPixel(x, y, r, g, b)
            }
    }
}

function draw_image_create(parent, width, height, ws)
{
    const dimg = add_div(parent, 'dimg_top')
    const ctrl = add_div(dimg, 'dimg_ctrl')
    const disp = add_div(dimg, 'dimg_disp')
    const canvas = new GfxCanvas(width, height, 5, disp, 0, 'dimg_canvas')
    const drawer = new Module.JsDraw(canvas)
    const gfx = Module.gfx_init_display(width, height)

    let panel = new DImgPanel(drawer, gfx, width, height)
    let last_img = null

    window.setInterval(()=>{
        if (last_img === null)
            return
        const sz = last_img.w * last_img.h
        let [cmd, offset] = make_buf_cmd('DI', sz, 3)
        for(let i = 0; i < sz * 3; ++i)
            cmd[offset++] = last_img.d_rgb[i]
        last_img = null;
        ws.send(cmd)
        console.log("sent DI " + offset + " bytes")
    }, 500)

    const display = ()=>{
        gfx.clear()
        panel.draw()
        canvas.draw()        
    }
    const send_update = ()=>{
        last_img = panel.img
    }

    panel.add_ui(ctrl, display, send_update)

    display()
    return send_update
}