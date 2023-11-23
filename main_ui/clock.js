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
    const clock = add_div(parent, 'clock_top')
    const ctrl = add_div(clock, 'clock_ctrl')
    const disp = add_div(clock, 'clock_disp')
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

class TimerPanel
{
    constructor(name, pref_json) {
        this.text1 = new TextBlock(name + "_t1", pref_json, "20:23:45")
        this.hours = NumProp.from_json(name + "_hours", pref_json)
        this.min = NumProp.from_json(name + "_min", pref_json)
        this.sec = NumProp.from_json(name + "_sec", pref_json)

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

        let d = this.cur_diff_msec
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
        this.text1.text = t

        display_cb()
    }

    add_ui(ctrl, display_cb, pref_update, send_toggle)
    {
        const col1 = add_div(ctrl, ['t_col', 't_col1'])
        const t1cont = add_div(col1, ['t_cont', 'tm1_cont'])
        this.text1.add_ui(t1cont, display_cb, pref_update)

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
        const start_btn = add_div(ctrl, 'tm_btn')
        start_btn.innerText = 'Toggle'
        start_btn.addEventListener('click', ()=>{
            this.toggle_run()
            send_toggle(this.running ? 1 : 0)
        })

    }

    draw(gfx)
    {
        this.text1.draw(gfx)
    }
}

function timer_create(parent, pref_json, ws, width, height)
{
    const timer = add_div(parent, 'timer_top')
    const ctrl = add_div(timer, 'timer_ctrl')
    const disp = add_div(timer, 'timer_disp')
    const timer_canvas = new GfxCanvas(width, height, 5, disp, 0, 'timer_canvas')
    Module.gfx_init_display(timer_canvas, width, height)
    const panel = new TimerPanel('tm', pref_json.timer)

    const display = ()=>{
        timer_canvas.gfx.clear()
        panel.draw(timer_canvas.gfx)
        timer_canvas.draw()
    }
    const send_update = (name, value)=>{
        const cmd = "U " + name + " " + value
        ws.send(cmd)
    }
    const send_toggle = (v)=>{
        const cmd = "TT " + v
        ws.send(cmd)
    }
    panel.add_ui(ctrl, display, send_update, send_toggle)
    panel.start_time(display)
    display()
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
        const ctrl_line1 = add_elem(ctrl, 'div', 'ctrl_line')
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
    const dimg = add_elem(parent, 'div', 'dimg_top')
    const ctrl = add_elem(dimg, 'div', 'dimg_ctrl')
    const disp = add_elem(dimg, 'div', 'dimg_disp')
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