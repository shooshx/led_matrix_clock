"use strict";


function create_elem(elem_type, cls) {
    let e = document.createElement(elem_type);
    if (cls !== undefined && cls !== null) {
        if (!Array.isArray(cls))
            cls = [cls]
        e.classList = cls.join(" ")
    }
    return e
}
function add_elem(parent, elem_type, cls) {
    let e = create_elem(elem_type, cls)
    parent.appendChild(e)
    return e
}
function add_div(parent, cls) {
    return add_elem(parent, 'div', cls)
}


class GfxCanvas
{
    constructor(w, h, scale, parent_elem, extra_sz, canvas_name)
    {
        this.width = w
        this.height = h
        this.scale = scale
        this.canvas_width = w*scale + extra_sz
        this.canvas_height = h*scale + extra_sz
        this.log = false

        this.canvas = add_elem(parent_elem, 'canvas', ['gfx_canvas', canvas_name])
        this.canvas.setAttribute('id', canvas_name)
        this.canvas.width = this.canvas_width
        this.canvas.height = this.canvas_height
        this.ctx = this.canvas.getContext('2d')
        this.ctx.imageSmoothingEnabled = false

        this.shadow_canvas = add_elem(parent_elem, 'canvas', 'gfx_shadow_canvas')
        this.shadow_canvas.style.visibility = "hidden"
        this.shadow_canvas.width = w
        this.shadow_canvas.height = h
        this.shadow_ctx = this.shadow_canvas.getContext('2d')

        this.image_data = this.shadow_ctx.getImageData(0, 0, this.width, this.height)
        this.pixels = this.image_data.data

        this.cmd_cb = null
    }

    setPixel(x, y, r, g, b)
    {
        if (x < 0 || y < 0 || x >= this.width || y >= this.height)
            return
        //if (this.log && (r != 0 || g != 0 || b != 0))
        //    console.log("jgfx " + x + "," + y + " - " + r + "," + g + "," + b)
        let i = (y * this.width + x)*4
        this.pixels[i] = r
        this.pixels[i+1] = g
        this.pixels[i+2] = b
        this.pixels[i+3] = 255;
        if (this.cmd_cb != null)
            this.cmd_cb.add(x, y, r, g, b)
    }

    draw()
    {
        //if (this.log)
        //    console.log("jgfx draw")
        this.ctx.fillStyle = '#000000'
        this.ctx.fillRect(0, 0, this.canvas_width, this.canvas_height)
        this.shadow_ctx.putImageData(this.image_data, 0, 0);
        this.ctx.drawImage(this.shadow_canvas, 0, 0, this.canvas_width, this.canvas_height)
    }

}


function add_slider(parent, cls, min, max, value, cb) {
    const size_in = add_elem(parent, 'input', [cls, 'slider'])
    size_in.setAttribute('type', 'range')
    size_in.setAttribute('min', min)
    size_in.setAttribute('max', max)
    size_in.setAttribute('value', value)
    size_in.addEventListener('input', (e)=>{
        cb(parseInt(size_in.value))
    })
}

let running_id = 1
function add_select(parent, label, values, init_value, cb)
{
    let lbl = null
    if (label !== null) {
        lbl = add_elem(parent, 'label', 'combo_label')
        lbl.innerText = label
    }
    const sel = add_elem(parent, 'select', 'combo_sel')
    const id = 'sel_' + running_id++;
    sel.setAttribute('id', id)
    if (lbl)
        lbl.setAttribute('for', id)
    for(let v of values) {
        const opt = add_elem(sel, 'option', 'combo_opt')
        opt.setAttribute('value', v.value)
        opt.innerText = v.text
    }
    sel.value = init_value
    sel.addEventListener('change', ()=>{ cb(sel.value) } )
    return sel
}

function add_num_input(parent, label, init_val, cb, opt={})
{
    if (label != null) {
        const lbl = add_elem(parent, 'label', 'num_in_label')
        lbl.innerText = label
    }
    const cont = add_div(parent, 'num_in_cont')
    const minus_btn = add_div(cont, ['num_in_minus', 'num_in_btn'])
    minus_btn.innerHTML = '&ndash;'

    const inp = add_elem(cont, 'input', 'num_input')
    inp.setAttribute('type', 'number')
    inp.setAttribute('readonly', true)
    inp.value = init_val
    inp.addEventListener('change', ()=>{ cb(inp.value) } )

    const plus_btn = add_div(cont, ['num_in_plus', 'num_in_btn'])
    plus_btn.innerText = '+'

    minus_btn.addEventListener('click', ()=>{ 
        const prevv = parseInt(inp.value)
        let newv = prevv - 1; 
        if (opt.min !== undefined && newv < opt.min)
            newv = opt.min
        if (newv == prevv)
            return
        inp.value = newv
        cb(newv) 
    })
    plus_btn.addEventListener('click', ()=>{ 
        const prevv = parseInt(inp.value)
        let newv = prevv + 1
        if (opt.max !== undefined && newv > opt.max)
            newv = opt.max
        if (newv == prevv)
            return
        inp.value = newv; 
        cb(newv) 
    })
    return inp
}

function add_checkbox_input(parent, label, init_val, cb, cls=null)
{
    if (cls === null)
        cls = 'in_check_cont'
    const cont = add_div(parent, cls)
    const inp = add_elem(cont, 'input', 'in_checkbox')
    inp.setAttribute('type', 'checkbox')
    const lbl = add_elem(cont, 'label', 'in_check_label')
    lbl.innerText = label

    const id = 'check_' + running_id++;
    inp.setAttribute('id', id)
    lbl.setAttribute('for', id)

    inp.checked = init_val
    inp.addEventListener('change', ()=>{ cb(inp.checked ? 1 : 0) })
}

function add_btn(parent, text, cb) 
{
    const btn = add_div(parent, 'tm_btn')
    btn.innerText = text
    btn.addEventListener('click', cb)
}



var FONT_OPT = null
function font_opts() {
    if (FONT_OPT !== null)
        return FONT_OPT
    const font_strs = Module.gfx_get_fonts()
    FONT_OPT = [{value:-1, text:'Default'}]
    for(let i = 0; i < font_strs.size(); ++i)
        FONT_OPT.push({value:i, text:font_strs.get(i)})
    return FONT_OPT
}

function color888(c) {
    const vec = Module.color888v(c);
    const r = vec.get(0), g = vec.get(1), b = vec.get(2)
    return [r, g, b]
}
function color565(r, g, b) {
    return Module.color565(r, g, b);
}

class ColorProp
{
    constructor(name, r, g, b) {
        this.name = name
        this.r = r
        this.g = g
        this.b = b
    }
    set_and_update(c, pref_update) {
        this.r = c.r
        this.g = c.g
        this.b = c.b
        pref_update(this.name, color565(this.r, this.g, this.b))
    }
    static from_json(name, pref_json) {
        const [r, g, b] = color888(pref_json[name] || 0)
        return new ColorProp(name, r, g, b)
    }
}

class NumProp
{
    constructor(name, v) {
        this.name = name
        this.v = v
        this.cb = null
    }
    set_cb(cb) {
        this.cb = cb
    }
    set_and_update(v, pref_update) {
        this.v = v
        pref_update(this.name, this.v)
        if (this.cb !== null)
            this.cb()
    }
    static from_json(name, pref_json) {
        const jv = pref_json[name]
        let v = 0
        if (jv === undefined || jv === null)
            v = 0
        else if (jv === true)
            v = 1
        else if (jv === false)
            v = 0
        else
            v = parseInt(jv)
        return new NumProp(name, v)
    }
}

const ALIGN_LEFT = 0
const ALIGN_RIGHT = 1

class TextBlock
{
    constructor(name, pref_json, text, align=ALIGN_LEFT)
    {
        this.text = text
        this.font_index = NumProp.from_json(name + "_font_idx", pref_json)
        this.x = NumProp.from_json(name + "_x", pref_json)
        this.y = NumProp.from_json(name + "_y", pref_json)
        this.color = ColorProp.from_json(name + "_color", pref_json)
        this.outline = NumProp.from_json(name + "_outline", pref_json)
        this.outline_color = ColorProp.from_json(name + "_outline_color", pref_json)
        this.double_size = NumProp.from_json(name + "_dbl", pref_json) // every font pixel is 2x2
        this.align = align
    }

    // drawers is {drawer, halo}, same logic as TextBlockN in the firmware:
    // outlined text goes through the halo drawer that draws the outline immediately and
    // keeps the text itself until flush so that it's on top of all outlines
    begin_draw(gfx, drawers) {
        if (this.outline.v) {
            drawers.halo.setMyColor(this.outline_color.r, this.outline_color.g, this.outline_color.b)
            gfx.set_drawer(drawers.halo)
        }
        else {
            gfx.set_drawer(drawers.drawer)
        }
    }
    // leave the halo drawer set so that gfx.finish() flushes the kept text
    end_draw(gfx, drawers) {
        gfx.set_drawer(drawers.halo)
    }

    text_size() {
        return this.double_size.v ? 2 : 1
    }
    set_font(gfx) {
        gfx.set_font(this.font_index.v)
        gfx.set_text_size(this.text_size())
    }

    draw(gfx, drawers) {
        this.begin_draw(gfx, drawers)
        this.set_font(gfx)
        gfx.set_text_color(this.color.r, this.color.g, this.color.b)
        gfx.print_str_at(this.x.v, this.y.v, this.text, this.align)
        this.end_draw(gfx, drawers)
    }

    add_ui(ctrl, display_cb, pref_update) 
    {
        const ctrl_line1 = add_div(ctrl, 'ctrl_line')
        const col_in = add_elem(ctrl_line1, 'input', 'clock_col_in')
        col_in.setAttribute('readonly', true)
        ColorEditBox.create_at(col_in, 300, (c)=>{ 
            this.color.set_and_update(c, pref_update)
            display_cb()
        }, {}, ColorPicker.make_hex(this.color, true))

        add_select(ctrl_line1, null, font_opts(), this.font_index.v, (value)=>{
            this.font_index.set_and_update(parseInt(value), pref_update)
            display_cb()
        })

        const ctrl_line_ol = add_div(ctrl, 'ctrl_line')
        add_checkbox_input(ctrl_line_ol, "2x", this.double_size.v, (v)=>{
            this.double_size.set_and_update(v, pref_update)
            display_cb()
        }, 'outline_check')
        add_checkbox_input(ctrl_line_ol, "Outline", this.outline.v, (v)=>{
            this.outline.set_and_update(v, pref_update)
            display_cb()
        }, 'outline_check')
        const ol_col_in = add_elem(ctrl_line_ol, 'input', 'clock_col_in')
        ol_col_in.setAttribute('readonly', true)
        ColorEditBox.create_at(ol_col_in, 300, (c)=>{
            this.outline_color.set_and_update(c, pref_update)
            display_cb()
        }, {}, ColorPicker.make_hex(this.outline_color, true))

        const ctrl_line2 = add_div(ctrl, 'ctrl_line_last')
        add_num_input(ctrl_line2, null, this.x.v, (value)=>{
            this.x.set_and_update(value, pref_update)
            display_cb()
        })
        add_num_input(ctrl_line2, null, this.y.v, (value)=>{
            this.y.set_and_update(value, pref_update)
            display_cb()
        })
    }
}

const DIGIT_PAIRS = ["00", "11", "22", "33", "44", "55", "66", "77", "88", "99"]

class ClockTextBlock extends TextBlock
{
    constructor(name, pref_json) {
        super(name, pref_json, "", ALIGN_LEFT)
        this.hour = "00"
        this.min = "00"
        this.sec = "00"
        this.tsec = "0"
        this.show_tenth = NumProp.from_json(name + "_show_tenth", pref_json)

        this.width_dbl_digit = 0
        this.width_single_digit = 0
        this.width_colon = 0
        this.width_point = 0
        this.width_digit = []
        this.prev_font = -2  // -1 is default font
        this.prev_size = 0
    }

    set_time(h, m, s, t) {
        this.hour = h
        this.min = m
        this.sec = s
        this.tsec = t
    }

    add_ui(ctrl, display_cb, pref_update) 
    {
        super.add_ui(ctrl, display_cb, pref_update)
        add_checkbox_input(ctrl, "Show MSec", this.show_tenth.v, (v)=>{
            this.show_tenth.set_and_update(v, pref_update)
            display_cb()
        }, 'msec_checkbox')
    }

    recalc_font(gfx) {
        let max_width = 0
        for(let p of DIGIT_PAIRS) {
            const w = gfx.calc_str_width(p)
            if (w > max_width)
                max_width = w
        }
        for(let i = 0; i < 9; ++i) {
            const si = "" + i
            this.width_digit[si] = gfx.calc_str_width(si)
        }
        this.width_dbl_digit = max_width
        this.width_single_digit = max_width / 2
        this.width_colon = gfx.calc_str_width(":")
        this.width_point = gfx.calc_str_width(".")
    }

    print_pair(gfx, x, s) {
        if (s.length == 1)
            gfx.print_str_at(x + this.width_single_digit, this.y.v, s, ALIGN_LEFT)
        else {
            const first_digit = s[0]
            const nx = x + this.width_single_digit - this.width_digit[first_digit]
            gfx.print_str_at(nx, this.y.v, s, ALIGN_LEFT)
        }
    }

    draw(gfx, drawers) {
        this.begin_draw(gfx, drawers)
        this.set_font(gfx)
        if (this.font_index.v != this.prev_font || this.text_size() != this.prev_size) {
            this.recalc_font(gfx)
            this.prev_font = this.font_index.v
            this.prev_size = this.text_size()
        }
        // this.x is the center of the string, calc to total width
        let tw = 0;
        if (this.hour !== null) {
            tw += this.width_dbl_digit + this.width_colon
        }
        tw += this.width_dbl_digit + this.width_colon // min
        tw += this.width_dbl_digit  // sec
        if (this.show_tenth.v)
            tw += this.width_point + this.width_single_digit // tsec
        //console.log("tw=", tw)
        
        let x = this.x.v - (tw / 2)

        gfx.set_text_color(this.color.r, this.color.g, this.color.b)
        if (this.hour !== null) {
            if (this.hour.length == 1)
                x += this.width_single_digit
            //this.print_pair(gfx, x, this.hour)
            gfx.print_str_at(x, this.y.v, this.hour, ALIGN_LEFT)
            if (this.hour.length == 1)
                x += this.width_single_digit
            else                
                x += this.width_dbl_digit
            gfx.print_str_at(x, this.y.v, ":", ALIGN_LEFT)
            x += this.width_colon
        }

        if (this.min !== null) {
            this.print_pair(gfx, x, this.min)
            x += this.width_dbl_digit
            gfx.print_str_at(x, this.y.v, ":", ALIGN_LEFT)
            x += this.width_colon
        }

        this.print_pair(gfx, x, this.sec)
        x += this.width_dbl_digit

        if (this.show_tenth.v) {
            gfx.print_str_at(x, this.y.v, ".", ALIGN_LEFT)
            x += this.width_point
            gfx.print_str_at(x, this.y.v, this.tsec, ALIGN_LEFT)
        }
        this.end_draw(gfx, drawers)
    }
}

// clock time h:mm[:ss] where every field has a fixed width slot, as wide as the widest value
// it can show in the current font, and the value is right aligned in it.
// this way the colons don't move when the digits change, also with non-monospace fonts
// same as TimeTextBlock in the firmware
class TimeTextBlock extends TextBlock
{
    constructor(name, pref_json) {
        super(name, pref_json, "", ALIGN_LEFT)
        this.hour = 0
        this.min = 0
        this.sec = 0
        this.show_sec = true

        this.hour_w = 0  // slot width of 0-23
        this.pair_w = 0  // slot width of 00-59
        this.colon_w = 0
        this.prev_font = -2  // -1 is default font
        this.prev_size = 0
    }

    set_time(h, m, s, show_sec) {
        this.hour = h
        this.min = m
        this.sec = s
        this.show_sec = show_sec
    }

    static format(v, zero_pad) {
        return (zero_pad && v < 10) ? "0" + v : "" + v
    }

    recalc_font(gfx) {
        this.hour_w = 0
        for(let i = 0; i < 24; ++i)
            this.hour_w = Math.max(this.hour_w, gfx.calc_str_width(TimeTextBlock.format(i, false)))
        this.pair_w = 0
        for(let i = 0; i < 60; ++i)
            this.pair_w = Math.max(this.pair_w, gfx.calc_str_width(TimeTextBlock.format(i, true)))
        this.colon_w = gfx.calc_str_width(":")
    }

    // prints v right aligned in the slot that starts at x, returns the x after the slot
    print_slot(gfx, x, slot_w, v, zero_pad) {
        const s = TimeTextBlock.format(v, zero_pad)
        gfx.print_str_at(x + slot_w - gfx.calc_str_width(s), this.y.v, s, ALIGN_LEFT)
        return x + slot_w
    }

    draw(gfx, drawers) {
        this.begin_draw(gfx, drawers)
        this.set_font(gfx)
        if (this.font_index.v != this.prev_font || this.text_size() != this.prev_size) {
            this.recalc_font(gfx)
            this.prev_font = this.font_index.v
            this.prev_size = this.text_size()
        }
        gfx.set_text_color(this.color.r, this.color.g, this.color.b)
        let x = this.x.v
        x = this.print_slot(gfx, x, this.hour_w, this.hour, false)
        gfx.print_str_at(x, this.y.v, ":", ALIGN_LEFT)
        x += this.colon_w
        x = this.print_slot(gfx, x, this.pair_w, this.min, true)
        if (this.show_sec) {
            gfx.print_str_at(x, this.y.v, ":", ALIGN_LEFT)
            x += this.colon_w
            this.print_slot(gfx, x, this.pair_w, this.sec, true)
        }
        this.end_draw(gfx, drawers)
    }
}

const DAY_NAMES =["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"]

function download(url, cb)
{
    const req = new XMLHttpRequest()
    req.addEventListener("load", function() {
        if (this.readyState == 4 && this.status == 200) {
            cb(this.responseText)
            return
        }
        const msg = "Error: " + this.readyState + "," + this.status + "," + this.responseText
        console.error(msg)
    })
    req.addEventListener("error", function() {
        console.error(msg)
    })
    req.open("GET", url)
    req.send()
}
