"use strict";

class ClockPanel
{
    constructor(name, pref_json)
    {
        this.text1 = new TextBlock(name + "_t1", pref_json, "20:23:45")
        this.text2 = new TextBlock(name + "_t2", pref_json, "Fri 20/10/2023")
        this.show_seconds = NumProp.from_json(name + "_show_sec", pref_json)
        this.show_day = NumProp.from_json(name + "_show_day", pref_json)
        //this.back_color = ColorProp.from_json(name + "_back_col", pref_json)
        this.offset = NumProp.from_json("tz_offset", pref_json)
    }

    start_time(display_cb) {
        this.update_time(display_cb)
        window.setInterval(()=>{ this.update_time(display_cb) }, 1000)
    }

    update_time(display_cb)
    {
        const ld = new Date()
        const utc = ld.getTime()
        const d = new Date(utc + 3600000*this.offset.v + ld.getTimezoneOffset()*60000)

        let min = d.getMinutes()
        if (min < 10)
            min = "0" + min
        let time = d.getHours() + ":" + min
        if (this.show_seconds.v) {
            let sec = d.getSeconds()
            if (sec < 10)
                sec = "0" + sec
            time += ":" + sec
        }
        let date = ""
        if (this.show_day.v) 
            date += DAY_NAMES[d.getDay()] + " "
        date += d.getDate()
        date += "/" + d.getMonth() + "/"
        date += d.getFullYear()
        
        this.text1.text = time
        this.text2.text = date
        display_cb()
    }

    add_ui(ctrl, display_cb, pref_update)
    {
        const col1 = add_div(ctrl, ['t_col', 't_col1'])
        const t1cont = add_div(col1, ['t_cont', 't1_cont'])
        this.text1.add_ui(t1cont, display_cb, pref_update)

        const t2cont = add_div(col1, ['t_cont', 't2_cont'])
        this.text2.add_ui(t2cont, display_cb, pref_update)

        const col2 = add_div(ctrl, ['t_col', 't_col2'])
        add_checkbox_input(col2, "Seconds", this.show_seconds.v, (v)=>{
            this.show_seconds.set_and_update(v, pref_update)
            this.update_time(display_cb)
        })
        add_checkbox_input(col2, "Show Day", this.show_day.v, (v)=>{
            this.show_day.set_and_update(v, pref_update)
            this.update_time(display_cb)
        })

        /*const col_in = add_elem(col2, 'input', 'clock_col_in')
        col_in.setAttribute('readonly', true)
        ColorEditBox.create_at(col_in, 300, (c)=>{ 
            this.back_color.set_and_update(c, pref_update) 
            display_cb()
        }, {}, ColorPicker.make_hex(this.back_color, true))
        */
        add_num_input(col2, "Offset\n", this.offset.v, (value)=>{
            this.offset.set_and_update(value, pref_update)
            this.update_time(display_cb)
        })
    }

    draw(gfx)
    {
        //gfx.set_back_col(this.back_color.r, this.back_color.g, this.back_color.b)
        this.text1.draw(gfx)
        this.text2.draw(gfx)
    }
}

function clock_create(parent, pref_json, ws, width, height)
{
    const clock = add_div(parent, 'sect_top')
    const ctrl = add_div(clock, 'sect_ctrl')
    const disp = add_div(clock, 'sect_disp')
    const clock_canvas = new GfxCanvas(width, height, 5, disp, 0, "clock_canvas")
    Module.gfx_init_display(clock_canvas, width, height)

    const panel = new ClockPanel("p0", pref_json.clock)

    const display = ()=>{
        clock_canvas.gfx.clear()
        panel.draw(clock_canvas.gfx)
        clock_canvas.draw()
    }
    const send_update = (name, value)=>{
        const cmd = "U " + name + " " + value
        ws.send(cmd)
    }

    panel.add_ui(ctrl, display, send_update)
    panel.start_time(display)

    display()
}

// ---------------------------------------------------------------------------------------------------------


function format_time(d)
{
    const h = Math.trunc(d / (60*60*1000))
    d -= h * (60*60*1000)
    const m = Math.trunc(d / (60*1000))
    d -= m * (60*1000)
    const s = Math.trunc(d / 1000)
    d -= s * 1000
    const ms = Math.trunc(d / 100)

    let t = ""
    if (h > 0)
        t += h + ":"
    if (h > 0 && m < 10)
        t += "0"
    t += m + ":"
    if (s < 10)
        t += "0"
    t += s

    t += "." + ms
    return t
}

function format_time_sp(d)
{
    const h = Math.trunc(d / (60*60*1000))
    d -= h * (60*60*1000)
    const m = Math.trunc(d / (60*1000))
    d -= m * (60*1000)
    const s = Math.trunc(d / 1000)
    d -= s * 1000
    const ms = Math.trunc(d / 100)

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

class TimerPanel
{
    constructor(name, pref_json) {
        this.text1 = new ClockTextBlock(name + "_t1", pref_json)
        this.hours = NumProp.from_json(name + "_hours", pref_json)
        this.min = NumProp.from_json(name + "_min", pref_json)
        this.sec = NumProp.from_json(name + "_sec", pref_json)
        this.snd_vol = NumProp.from_json(name + "_snd_vol", pref_json)
        this.snd_file_num = NumProp.from_json(name + "_snd_file_num", pref_json) 

        this.running = false
        this.dest_time_msec = 0 // while running this is the current destimation
        this.cur_diff_msec = 0

        this.prev_diff_msec = 0
        this.force_draw = false
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
        this.force_draw = true
        this.update_time(display_cb)
    }

    update_time(display_cb)
    {
        this.prev_diff_msec = this.cur_diff_msec
        if (this.running) {
            const now = new Date().getTime()
            this.cur_diff_msec = this.dest_time_msec - now
            if (this.cur_diff_msec < 0) {
                this.cur_diff_msec = 0
                this.running = false
            }
        }
        //if (this.prev_diff_msec == this.cur_diff_msec && !this.force_draw)
        //    return
        this.force_draw = false
        //this.text1.text = format_time(this.cur_diff_msec)
        const [h, m, s, t] = format_time_sp(this.cur_diff_msec)
        this.text1.set_time(h, m, s, t)
        display_cb()
    }

    add_ui(ctrl, display_cb, pref_update, send_cmd)
    {
        const col1 = add_div(ctrl, ['t_col', 't_col1'])
        const t1cont = add_div(col1, ['t_cont', 'tm1_cont'])
        this.text1.add_ui(t1cont, display_cb, pref_update)

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

    draw(gfx)
    {
        this.text1.draw(gfx)
    }
}

function time_generic_create(parent, pref_json, ws, width, height, PanelCls, pref_prefix)
{
    const timer = add_div(parent, 'sect_top')
    const ctrl = add_div(timer, 'sect_ctrl')
    const disp = add_div(timer, 'sect_disp')
    const canvas = new GfxCanvas(width, height, 5, disp, 0, 'timer_canvas')
    Module.gfx_init_display(canvas, width, height)
    const panel = new PanelCls(pref_prefix, pref_json)

    const display = ()=>{
        canvas.gfx.clear()
        panel.draw(canvas.gfx)
        canvas.draw()
    }
    const send_update = (name, value)=>{
        const cmd = "U " + name + " " + value
        ws.send(cmd)
    }
    const send_cmd = (v)=>{
        ws.send(v)
    }
    panel.add_ui(ctrl, display, send_update, send_cmd)
    panel.start_time(display)
    display()
    return canvas
}

let timer_canvas = null

function timer_create(parent, pref_json, ws, width, height)
{
    timer_canvas = time_generic_create(parent, pref_json.timer, ws, width, height, TimerPanel, "tm")
}

// ---------------------------------------------------------------------------------------------------------

class StopWPanel
{
    constructor(name, pref_json) {
        this.text1 = new ClockTextBlock(name + "_t1", pref_json, "20:23:45", ALIGN_RIGHT)
        this.is_running = false
        this.start_time_msec = -1
        this.cur_diff_msec = 0
    }

    start_time(display_cb) {
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

        //this.text1.text = format_time(this.cur_diff_msec)
        const [h, m, s, t] = format_time_sp(this.cur_diff_msec)
        this.text1.set_time(h, m, s, t)
        display_cb()
    }

    add_ui(ctrl, display_cb, pref_update, send_cmd)
    {
        const col1 = add_div(ctrl, ['t_col', 't_col1'])
        const t1cont = add_div(col1, ['t_cont', 'tm1_cont'])
        this.text1.add_ui(t1cont, display_cb, pref_update)
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

    draw(gfx)
    {
        this.text1.draw(gfx)
    }
}

function stop_watch_create(parent, pref_json, ws, width, height)
{
    time_generic_create(parent, pref_json.stopw, ws, width, height, StopWPanel, "sw")
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
    constructor()
    {
        this.objs = [  new LinearGradient({r:255, g:0, b:0}, {x:0, y:0}, {r:0, g:0, b:0}, {x:63, y:0}, 10),
                       new LinearGradient({r:0, g:255, b:0}, {x:0, y:10}, {r:0, g:0, b:0}, {x:63, y:10}, 10), 
                       new LinearGradient({r:0, g:0, b:255}, {x:0, y:20}, {r:0, g:0, b:0}, {x:63, y:20}, 10)
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
    draw(gfx, w, h)
    {
        this.make_image(w, h)
        let i = 0;
        for(let x = 0; x < w; ++x)
            for(let y = 0; y < h; ++y) {
                const [r, g, b] = this.img.get(x, y)
                gfx.setPixel(x, y, r, g, b)
            }
    }
}

function draw_image_create(parent, width, height, ws)
{
    const dimg = add_div(parent, 'dimg_top')
    const ctrl = add_div(dimg, 'dimg_ctrl')
    const disp = add_div(dimg, 'dimg_disp')
    const dimg_canvas = new GfxCanvas(width, height, 5, disp, 0, 'dimg_canvas')
    Module.gfx_init_display(dimg_canvas, width, height)

    let panel = new DImgPanel()
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
        dimg_canvas.gfx.clear()
        panel.draw(dimg_canvas, width, height)
        dimg_canvas.draw()        
    }
    const send_update = ()=>{
        last_img = panel.img
    }

    panel.add_ui(ctrl, display, send_update)

    display()
}