"use strict";

// the panel size in landscape, portrait is the other way around
const PX_LONG = 128
const PX_SHORT = 64

// the clock for one orientation: the combined time and date lines and the separate fields
// same as ClockLayout in the firmware
class ClockLayout extends Layout
{
    constructor(name, pref_json)
    {
        super()
        this.time = this.add("Time", new TimeTextBlock(name + "_t1", pref_json))
        this.date = this.add("Date", new TextBlock(name + "_t2", pref_json, ""))
        this.hour = this.add("Hour", new FieldTextBlock(name + "_fh", pref_json, 'hour'))
        this.min = this.add("Minute", new FieldTextBlock(name + "_fm", pref_json, 'min'))
        this.sec = this.add("Second", new FieldTextBlock(name + "_fs", pref_json, 'sec'))
        this.weekday = this.add("Weekday", new FieldTextBlock(name + "_fw", pref_json, 'weekday'))
        this.day = this.add("Day", new FieldTextBlock(name + "_fd", pref_json, 'day'))
        this.month = this.add("Month", new FieldTextBlock(name + "_fo", pref_json, 'month'))
        this.year = this.add("Year", new FieldTextBlock(name + "_fy", pref_json, 'year'))
        this.show_seconds = NumProp.from_json(name + "_show_sec", pref_json)
        this.show_day = NumProp.from_json(name + "_show_day", pref_json)
    }

    set_time(d)
    {
        this.time.set_time(d.getHours(), d.getMinutes(), d.getSeconds(), this.show_seconds.v)
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
        add_checkbox_input(opts, "Time with seconds", this.show_seconds.v, (v)=>{
            this.show_seconds.set_and_update(v, pref_update)
            refresh_cb()
        }, 'outline_check')
        add_checkbox_input(opts, "Date with day", this.show_day.v, (v)=>{
            this.show_day.set_and_update(v, pref_update)
            refresh_cb()
        }, 'outline_check')
        this.add_blocks_ui(ctrl, display_cb, pref_update)
    }
}

// ---- special times: periods of the day when the clock has other colors and a background effect ----

const FX_NONE = 0, FX_BORDER = 1, FX_SLIDE = 2
const SPECIAL_PERIODS = 4
const DEMO_MS = 30000  // how long the panel shows a demo, same as ClockState::DEMO_MS

// a period of the day and how the clock looks in it, same as SpecialPeriod in the firmware.
// colors[i] / outlines[i] are for the texts of ClockLayout in their order (Time, Date, Hour...)
class SpecialPeriod
{
    constructor(name, pref_json, text_count)
    {
        this.name = name
        // the defaults are the firmware's, for prefs from before the special times
        this.on = NumProp.from_json(name + "_on", pref_json, 0)
        this.from = NumProp.from_json(name + "_from", pref_json, 19 * 60)
        this.to = NumProp.from_json(name + "_to", pref_json, 20 * 60)
        this.fx = NumProp.from_json(name + "_fx", pref_json, FX_BORDER)
        this.fx_color = ColorProp.from_json(name + "_fx_col", pref_json, 0xF800)
        this.fx_size = NumProp.from_json(name + "_fx_size", pref_json, 8)
        this.fx_speed = NumProp.from_json(name + "_fx_speed", pref_json, 3)
        this.fx_pulse = NumProp.from_json(name + "_fx_pulse", pref_json, 1)
        this.colors = []
        this.outlines = []
        for (let i = 0; i < text_count; ++i) {
            this.colors.push(ColorProp.from_json(name + "_c" + i, pref_json, 0xFFFF))
            this.outlines.push(ColorProp.from_json(name + "_o" + i, pref_json, 0))  // black: no outline
        }
    }
    animated() {
        return this.fx.v == FX_SLIDE || (this.fx.v == FX_BORDER && this.fx_pulse.v)
    }
    // same as SpecialPeriod::active_at in the firmware
    active_at(minute) {
        if (!this.on.v)
            return false
        const from = this.from.v, to = this.to.v
        if (from <= to)
            return minute >= from && minute < to
        return minute >= from || minute < to  // over midnight
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

function minutes_str(m) {
    const h = Math.trunc(m / 60), mm = m % 60
    return (h < 10 ? "0" : "") + h + ":" + (mm < 10 ? "0" : "") + mm
}

class ClockPanel
{
    constructor(prefixes, pref_json)
    {
        this.layouts = prefixes.map((p)=>new ClockLayout(p, pref_json))
        this.offset = NumProp.from_json("tz_offset", pref_json)
        const text_count = this.layouts[0].items.length
        this.text_labels = this.layouts[0].items.map((it)=>it.label)
        this.special = []
        for (let i = 0; i < SPECIAL_PERIODS; ++i)
            this.special.push(new SpecialPeriod("sp" + i, pref_json, text_count))
        this.preview = null       // the special period shown in the previews with "Preview here"
        this.active = null        // the special period of the time now, shown when there's no preview
        this.anim_timer = null
        this.display_cb = null
    }

    start_time(display_cb) {
        this.display_cb = display_cb
        this.update_time(display_cb)
        window.setInterval(()=>{ this.update_time(display_cb) }, 1000)
        // the moving effects
        this.anim_timer = window.setInterval(()=>{
            const sp = this.shown()
            if (sp !== null && sp.animated())
                this.display_cb()
        }, 40)
    }

    // the special period the previews show now, null if none
    shown() {
        return (this.preview !== null) ? this.preview : this.active
    }

    update_time(display_cb)
    {
        const ld = new Date()
        const utc = ld.getTime()
        const d = new Date(utc + 3600000*this.offset.v + ld.getTimezoneOffset()*60000)
        for (const l of this.layouts)
            l.set_time(d)
        // like the panel: the first period the time is in
        const minute = d.getHours() * 60 + d.getMinutes()
        this.active = this.special.find((sp)=>sp.active_at(minute)) || null
        display_cb()
    }

    // called for every preview before its layout is drawn: the effect and the colors of the previewed period
    before_layout_draw(layout, view) {
        const sp = this.shown()
        if (sp !== null)
            draw_effect(view.canvas, view.w, view.h, sp, Date.now() / 1000)
        layout.items.forEach((it, k)=>{
            it.block.color_override = (sp !== null) ? sp.colors[k] : null
            it.block.outline_override = (sp !== null) ? sp.outlines[k] : null
        })
    }

    // shows the special period in the previews, null stops. moving effects are animated
    set_preview(sp) {
        this.preview = sp
        if (this.display_cb !== null)
            this.display_cb()
    }

    // the controls that are not part of a layout
    add_ui(ctrl, display_cb, pref_update, send_cmd)
    {
        add_num_input(ctrl, "Time zone offset: ", this.offset.v, (value)=>{
            this.offset.set_and_update(value, pref_update)
            this.update_time(display_cb)
        })
        this.add_special_ui(ctrl, display_cb, pref_update, send_cmd)
    }

    // a tab per period and the editor of the selected one
    add_special_ui(ctrl, display_cb, pref_update, send_cmd)
    {
        const box = add_div(ctrl, 'sp_box')
        const title = add_div(box, 'sp_title')
        const content = add_div(box, 'sp_content')
        const tabs = add_div(content, 'sp_tabs')
        const body = add_div(content, 'sp_body')
        // closed by default, it takes a lot of room. this browser remembers if it was opened
        let open = false
        try { open = localStorage['special_open'] == '1' } catch(e) {}
        const set_open = (v)=>{
            open = v
            content.style.display = v ? '' : 'none'
            title.innerText = (v ? "▾" : "▸") + " Special times"
            try { localStorage['special_open'] = v ? '1' : '0' } catch(e) {}
        }
        title.addEventListener('click', ()=>{ set_open(!open) })
        set_open(open)
        const chips = []
        let selected = 0
        let demo = null // {period, timer} while the panel shows a demo

        const chip_text = (i)=>{
            const sp = this.special[i]
            chips[i].innerText = (i + 1) + "  " + minutes_str(sp.from.v) + "-" + minutes_str(sp.to.v)
            chips[i].setAttribute('on', sp.on.v != 0)
        }
        const stop_demo = ()=>{
            if (demo === null)
                return
            window.clearTimeout(demo.timer)
            demo = null
            send_cmd("SD -1")
        }
        const select = (i)=>{
            selected = i
            for (let k = 0; k < chips.length; ++k)
                chips[k].setAttribute('selected', k == i)
            if (this.preview !== null)
                this.set_preview(this.special[i])  // the preview follows the selected period
            build(i)
        }

        const build = (i)=>{
            const sp = this.special[i]
            body.replaceChildren()
            const changed = ()=>{
                chip_text(i)
                this.update_time(display_cb)  // also which period is active now
            }

            // when
            const l1 = add_div(body, 'sp_line')
            add_checkbox_input(l1, "On", sp.on.v, (v)=>{
                sp.on.set_and_update(v, pref_update)
                changed()
            }, 'outline_check')
            for (const [label, prop] of [["From ", sp.from], [" to ", sp.to]]) {
                add_elem(l1, 'span', 'sp_label').innerText = label
                const t = add_elem(l1, 'input', 'sp_time')
                t.setAttribute('type', 'time')
                t.value = minutes_str(prop.v)
                t.addEventListener('change', ()=>{
                    const [hh, mm] = t.value.split(':').map((x)=>parseInt(x))
                    if (isNaN(hh) || isNaN(mm))
                        return
                    prop.set_and_update(hh * 60 + mm, pref_update)
                    changed()
                })
            }

            // background effect
            const l2 = add_div(body, 'sp_line')
            add_select(l2, "Effect ", [{value:FX_NONE, text:"None"}, {value:FX_BORDER, text:"Border"}, {value:FX_SLIDE, text:"Slide"}], sp.fx.v, (v)=>{
                sp.fx.set_and_update(parseInt(v), pref_update)
                show_fx_opts()
                changed()
            })
            const fx_opts = add_div(body, 'sp_line')
            const fx_col = add_elem(fx_opts, 'input', 'clock_col_in')
            ColorEditBox.create_at(fx_col, 300, (c)=>{
                sp.fx_color.set_and_update(c, pref_update)
                changed()
            }, {}, ColorPicker.make_hex(sp.fx_color, true))
            add_num_input(fx_opts, " Thickness ", sp.fx_size.v, (v)=>{
                sp.fx_size.set_and_update(parseInt(v), pref_update)
                changed()
            }, {min:1, max:32})
            const l3 = add_div(body, 'sp_line')
            add_elem(l3, 'span', 'sp_label').innerText = "Speed "
            add_slider(l3, 'sp_speed', 1, 10, sp.fx_speed.v, (v)=>{
                sp.fx_speed.set_and_update(v, pref_update)
                changed()
            })
            const pulse = add_div(l3, 'sp_pulse')
            add_checkbox_input(pulse, "Pulse", sp.fx_pulse.v, (v)=>{
                sp.fx_pulse.set_and_update(v, pref_update)
                changed()
            }, 'outline_check')
            const show_fx_opts = ()=>{
                fx_opts.style.display = (sp.fx.v == FX_NONE) ? 'none' : ''
                l3.style.display = (sp.fx.v == FX_NONE) ? 'none' : ''
                pulse.style.display = (sp.fx.v == FX_BORDER) ? '' : 'none'
            }
            show_fx_opts()

            // the colors of the texts
            const tbl = add_div(body, 'sp_colors')
            add_div(tbl, "sp_colors_hdr").innerText = "Text colors:   color / outline (black: none)"
            this.text_labels.forEach((label, k)=>{
                const row = add_div(tbl, 'sp_color_row')
                add_elem(row, 'span', 'sp_color_label').innerText = label
                for (const prop of [sp.colors[k], sp.outlines[k]]) {
                    const inp = add_elem(row, 'input', 'clock_col_in')
                    ColorEditBox.create_at(inp, 300, (c)=>{
                        prop.set_and_update(c, pref_update)
                        changed()
                    }, {}, ColorPicker.make_hex(prop, true))
                }
            })

            // seeing it without waiting for the time
            const l4 = add_div(body, 'sp_line')
            const prev_btn = add_div(l4, 'tm_btn')
            const prev_text = ()=>{ prev_btn.innerText = (this.preview === sp) ? "Stop preview" : "Preview here" }
            prev_text()
            prev_btn.addEventListener('click', ()=>{
                this.set_preview(this.preview === sp ? null : sp)
                prev_text()
            })
            const demo_btn = add_div(l4, 'tm_btn')
            const demo_text = ()=>{ demo_btn.innerText = (demo !== null && demo.period == i) ? "Stop demo" : "Demo on panel" }
            demo_text()
            demo_btn.addEventListener('click', ()=>{
                if (demo !== null && demo.period == i) {
                    stop_demo()
                }
                else {
                    stop_demo()
                    send_cmd("SD " + i)
                    demo = {period:i, timer:window.setTimeout(()=>{ demo = null; demo_text() }, DEMO_MS)}
                }
                demo_text()
            })
            add_elem(l4, 'span', 'sp_note').innerText = "the panel shows it for 30 seconds"
        }

        for (let i = 0; i < SPECIAL_PERIODS; ++i) {
            const chip = add_div(tabs, 'sp_chip')
            chip.addEventListener('click', ()=>{ select(i) })
            chips.push(chip)
            chip_text(i)
        }
        select(0)
    }
}

// a selector between the landscape and the portrait layout of a panel, each with its own preview
// where the texts can be selected by clicking and moved by dragging
function time_generic_create(parent, pref_json, ws, PanelCls, prefixes, portrait)
{
    const send_update = (name, value)=>{
        const cmd = "U " + name + " " + value
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

    const display = ()=>{
        for (let i = 0; i < views.length; ++i) {
            const v = views[i]
            const layout = panel.layouts[i]
            v.gfx.clear()
            if (panel.before_layout_draw)
                panel.before_layout_draw(layout, v)  // the clock's special times
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

    panel.add_ui(shared, display, send_update, send_cmd)
    for (let i = 0; i < views.length; ++i) {
        panel.layouts[i].add_ui(views[i].ctrl, display, send_update, refresh)
        add_drag(views[i], panel.layouts[i])
    }

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
}

// click on the preview selects the text there, dragging moves it
function add_drag(view, layout)
{
    const el = view.canvas.canvas
    // panel pixel of a mouse / touch position
    const to_px = (e)=>{
        const r = el.getBoundingClientRect()
        return [Math.floor((e.clientX - r.left) * view.w / r.width), Math.floor((e.clientY - r.top) * view.h / r.height)]
    }
    let drag = null
    const start = (e)=>{
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
            el.style.cursor = layout.hit(px, py) !== null ? 'move' : 'default'
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