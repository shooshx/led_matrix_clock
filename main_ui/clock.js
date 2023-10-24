"use strict";

class ClockPanel
{
    constructor(name, pref_json, pref_update)
    {
        this.text1 = new TextBlock(name + "_t1", pref_json, pref_update, "20:23:45", 5, 1, 1)
        this.text2 = new TextBlock(name + "_t2", pref_json, pref_update, "Fri 20/10/2023", -1, 0, 20)
        this.show_seconds = NumProp.from_json(name + "_show_sec", pref_json)
        this.show_day = NumProp.from_json(name + "_show_day", pref_json)
        this.back_color = ColorProp.from_json(name + "_back_col", pref_json)
    }

    start_time(display_cb) {
        this.update_time(display_cb)
        window.setInterval(()=>{ this.update_time(display_cb) }, 1000)
    }

    update_time(display_cb)
    {
        const d = new Date()
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
        const col1 = add_elem(ctrl, 'div', ['t_col', 't_col1'])
        const t1cont = add_elem(col1, 'div', ['t_cont', 't1_cont'])
        this.text1.add_ui(t1cont, display_cb, pref_update)

        const t2cont = add_elem(col1, 'div', ['t_cont', 't2_cont'])
        this.text2.add_ui(t2cont, display_cb, pref_update)

        const col2 = add_elem(ctrl, 'div', ['t_col', 't_col2'])
        add_checkbox_input(col2, "Seconds", this.show_seconds.v, (v)=>{
            this.show_seconds.set_and_update(v, pref_update)
            this.update_time(display_cb)
        })
        add_checkbox_input(col2, "Show Day", this.show_day.v, (v)=>{
            this.show_day.set_and_update(v, pref_update)
            this.update_time(display_cb)
        })

        const col_in = add_elem(col2, 'input', 'clock_col_in')
        col_in.setAttribute('readonly', true)
        ColorEditBox.create_at(col_in, 300, (c)=>{ 
            this.back_color.set_and_update(c, pref_update) 
            display_cb()
        }, {}, ColorPicker.make_hex(this.back_color, true))
    }

    draw(gfx)
    {
        gfx.set_back_col(this.back_color.r, this.back_color.g, this.back_color.b)
        this.text1.draw(gfx)
        this.text2.draw(gfx)
    }
}

function clock_create(parent, pref_json, ws)
{
    const clock = add_elem(parent, 'div', 'clock_top')
    const ctrl = add_elem(clock, 'div', 'clock_ctrl')
    const disp = add_elem(clock, 'div', 'clock_disp')
    const clock_canvas = new GfxCanvas(PX_WIDTH, PX_HEIGHT, 5, disp, 0, "clock_canvas")
    Module.gfx_init_display(clock_canvas, PX_WIDTH, PX_HEIGHT)

    let panel = new ClockPanel("p0", pref_json.clock)

    const display = ()=>{
        clock_canvas.gfx.clear()
        panel.draw(clock_canvas.gfx)
        clock_canvas.draw()
    }
    const send_update = (name, value)=>{
        const cmd = "UC " + name + " " + value
        ws.send(cmd)
    }

    panel.add_ui(ctrl, display, send_update)
    panel.start_time(display)

    display()
}
