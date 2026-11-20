"use strict";

// the panel size in landscape, portrait is the other way around
const PX_LONG = 128
const PX_SHORT = 64

// the clock for one orientation: the time with and without seconds, the date line and the separate fields
// same as ClockLayout in the firmware (where the time without seconds is in ClockFace)
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
// the order of the periods is by start, not by slot, so adding and deleting doesn't move the other slots

const FX_NONE = 0, FX_BORDER = 1, FX_SLIDE = 2
// how a face shows the clock, same as FaceMode in the firmware
const FACE_NORMAL = 0  // the layouts and the effect
const FACE_OFF = 1     // nothing lit, the panel is dark (also no mode icon)
const CLOCK_FACES = 8
const DAY_PERIODS = 12
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
        // when a period with it starts, the panel shows the clock, also when it was off or in a game
        this.wake = NumProp.from_json(n + "_wake", pref_json, 0)
        this.mode = NumProp.from_json(n + "_mode", pref_json, FACE_NORMAL)
    }

    dark() {
        return this.mode.v == FACE_OFF
    }

    animated() {
        return !this.dark() && (this.fx.v == FX_SLIDE || (this.fx.v == FX_BORDER && this.fx_pulse.v))
    }

    // all the props of how the face looks (not its name), in the same order in every face
    face_props() {
        const out = [this.fx, this.fx_color, this.fx_size, this.fx_speed, this.fx_pulse, this.wake, this.mode]
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
    css_color() {
        if (this.dark())
            return '#000000'
        const l = this.layouts[0]
        const it = l.time.show.v ? l.items[0] : (l.items.find((i)=>i.block.show.v) || l.items[0])
        const c = it.block.color
        if (c.r + c.g + c.b < 60)
            return '#555555'  // black text, would be invisible on the dark bar
        return 'rgb(' + c.r + ',' + c.g + ',' + c.b + ')'
    }
}

// a part of the day and the face it shows
class DayPeriod
{
    constructor(slot, pref_json)
    {
        const n = "per" + slot
        this.slot = slot
        this.on = NumProp.from_json(n + "_on", pref_json, 0)
        this.start = NumProp.from_json(n + "_start", pref_json, 0)
        this.face = NumProp.from_json(n + "_face", pref_json, 0)
    }
}

// the background effect into the canvas, t in seconds. same as SpecialPeriod::draw_effect() in the firmware
function draw_effect(canvas, w, h, sp, t)
{
    const {r, g, b} = sp.fx_color
    const size = Math.min(64, Math.max(1, sp.fx_size.v))
    const speed = Math.min(10, Math.max(1, sp.fx_speed.v))
    if (sp.fx.v == FX_BORDER) {
        const k = sp.fx_pulse.v ? 0.6 + 0.4 * Math.sin(2 * Math.PI * t * speed / 6.0) : 1.0
        for (let y = 0; y < h; ++y) {
            for (let x = 0; x < w; ++x) {
                const d = Math.min(x, y, w - 1 - x, h - 1 - y)
                if (d >= size)
                    continue
                let a = 1.0 - d / size
                a = a * a * k
                canvas.setPixel(x, y, Math.trunc(r * a), Math.trunc(g * a), Math.trunc(b * a))
            }
        }
    }
    else if (sp.fx.v == FX_SLIDE) {
        const period = 12.0 / speed
        const ph = (t % period) / period
        const tri = ph < 0.5 ? ph * 2 : 2 - ph * 2
        const half = size / 2.0
        const cx = -half + tri * (w + size)
        for (let x = Math.max(0, Math.trunc(cx - half)); x <= Math.min(w - 1, Math.trunc(cx + half) + 1); ++x) {
            let a = 1.0 - Math.abs(x + 0.5 - cx) / half
            if (a <= 0)
                continue
            a = a * a
            for (let y = 0; y < h; ++y)
                canvas.setPixel(x, y, Math.trunc(r * a), Math.trunc(g * a), Math.trunc(b * a))
        }
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
        this.all_periods = []
        for (let i = 0; i < DAY_PERIODS; ++i)
            this.all_periods.push(new DayPeriod(i, pref_json))
        if (!this.all_periods.some((p)=>p.on.v))
            this.all_periods[0].on.v = 1
        this.now_minute = 0
        this.active = null         // the period of the time now
        this.anim_timer = null
        this.display_cb = null
        this.pref_update = null
        this.layouts_changed = null // set by time_generic_create, makes the layout editors again
        this.mirror_to_panel = true // "Show on panel" for the preview
        // the parts of the ui that depend on the selection, made in add_ui
        this.timeline = null
        this.period_ui = null
        this.face_ui = null
        this.sel_face = null
        this.update_time(()=>{})
        this.sel_period = this.active          // the period whose time and face are edited
        this.sel_face = this.face_of(this.active) // the face that's edited and previewed
    }

    // the layouts of the edited face, the previews and the layout editors show them
    get layouts() {
        return this.sel_face.layouts
    }

    used_faces() {
        return this.faces.filter((f)=>f.on.v)
    }
    // the face a period shows. one that was deleted is the first face
    face_of(p) {
        const f = this.faces[p.face.v]
        return (f !== undefined && f.on.v) ? f : this.used_faces()[0]
    }
    // the periods that show a face
    periods_of(f) {
        return this.periods().filter((p)=>this.face_of(p) === f)
    }

    // the periods in their order in the day
    periods() {
        return this.all_periods.filter((p)=>p.on.v).sort((a, b)=>a.start.v - b.start.v)
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
            if (this.sel_face.animated())
                this.display_cb()
        }, 40)
    }

    update_time(display_cb)
    {
        const ld = new Date()
        const utc = ld.getTime()
        const d = new Date(utc + 3600000*this.offset.v + ld.getTimezoneOffset()*60000)
        this.now_minute = d.getHours() * 60 + d.getMinutes()
        const active = this.period_at(this.now_minute)
        const changed = active !== this.active
        this.active = active
        if (this.sel_face !== null)
            for (const l of this.sel_face.layouts)
                l.set_time(d)
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
        if (this.sel_face.fx.v != FX_NONE && !this.sel_face.dark())
            draw_effect(view.canvas, view.w, view.h, this.sel_face, Date.now() / 1000)
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

        const dbox = add_div(ctrl, 'cf_box')
        add_div(dbox, 'cf_title').innerText = "The day"
        this.timeline = new Timeline(add_div(dbox, 'cf_timeline'), this)
        const pbox = add_div(dbox, 'cf_period')

        const fbox = add_div(ctrl, 'cf_box')
        add_div(fbox, 'cf_title').innerText = "Clock faces"
        const face_box = add_div(fbox, 'cf_faces')

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
            dot.style.backgroundColor = o.css_color()
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
        add_select(l_mode, "Mode ", [{value:FACE_NORMAL, text:"Normal"}, {value:FACE_OFF, text:"Off"}], f.mode.v, (v)=>{
            f.mode.set_and_update(parseInt(v), pref_update)
            show_fx_opts()
            changed()
        })
        const mode_note = add_elem(l_mode, 'span', 'cf_note')

        // the background effect of this face
        const l3 = add_div(box, 'cf_line')
        add_select(l3, "Background effect ", [{value:FX_NONE, text:"None"}, {value:FX_BORDER, text:"Border"}, {value:FX_SLIDE, text:"Slide"}], f.fx.v, (v)=>{
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
        add_num_input(fx_opts, " Thickness ", f.fx_size.v, (v)=>{
            f.fx_size.set_and_update(parseInt(v), pref_update)
            changed()
        }, {min:1, max:32})
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
            const off = f.dark()
            l3.style.display = off ? 'none' : ''
            fx_opts.style.display = (off || f.fx.v == FX_NONE) ? 'none' : ''
            l4.style.display = (off || f.fx.v == FX_NONE) ? 'none' : ''
            pulse.style.display = (f.fx.v == FX_BORDER) ? '' : 'none'
            mode_note.innerText = off ? "  nothing is lit on the panel in its periods, not even the mode icon" : ""
        }
        show_fx_opts()

        const l5 = add_div(box, 'cf_line')
        add_checkbox_input(l5, "When its period starts, show the clock (also when the panel is off or in a game)", f.wake.v, (v)=>{
            f.wake.set_and_update(v, pref_update)
        }, 'outline_check')

        const n = this.periods_of(f).length
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
                    seg.style.backgroundColor = face.css_color()
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
        add_drag(views[i], ()=>panel.layouts[i])

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

// "Show on panel": the panel shows its clock as if it's the middle of the selected period ("SM <minute>"), with
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
    let sent_minute = -1, sent_at = 0

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
        sent_at = Date.now()
        ws.send("SM " + sent_minute)
        note.innerText = " as if it's " + minutes_str(sent_minute)
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
        if (minute() != sent_minute || Date.now() - sent_at >= SIM_RESEND_MS)
            send()
    }, 300)
}

// click on the preview selects the text there, dragging moves it. get_layout() is the layout shown now
function add_drag(view, get_layout)
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
        layout = get_layout()
        const [px, py] = to_px(e)
        const it = layout.hit(px, py)
        if (it === null)
            return false
        layout.select(it)
        drag = {sx:px, sy:py, ox:it.block.x.v, oy:it.block.y.v, moved:false}
        return true
    }
    const move = (e)=>{
        const [px, py] = to_px(e)
        if (drag === null) {
            el.style.cursor = get_layout().hit(px, py) !== null ? 'move' : 'default'
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