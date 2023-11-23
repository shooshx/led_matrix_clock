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
        this.ctx.fillStyle = '#000000'
        this.ctx.fillRect(0, 0, this.canvas_width, this.canvas_height)
        this.shadow_ctx.putImageData(this.image_data, 0, 0);
        this.ctx.drawImage(this.shadow_canvas, 0, 0, this.canvas_width, this.canvas_height)
    }

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

function add_checkbox_input(parent, label, init_val, cb)
{
    const cont = add_div(parent, 'in_check_cont')
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
        const [r, g, b] = color888(pref_json[name])
        return new ColorProp(name, r, g, b)
    }
}

class NumProp
{
    constructor(name, v) {
        this.name = name
        this.v = v
    }
    set_and_update(v, pref_update) {
        this.v = v
        pref_update(this.name, this.v)
    }
    static from_json(name, pref_json) {
        const v = pref_json[name]
        return new NumProp(name, v)
    }
}


class TextBlock
{
    constructor(name, pref_json, text)
    {
        this.text = text
        this.font_index = NumProp.from_json(name + "_font_idx", pref_json)
        this.x = NumProp.from_json(name + "_x", pref_json)
        this.y = NumProp.from_json(name + "_y", pref_json)
        this.color = ColorProp.from_json(name + "_color", pref_json)
    }
    draw(gfx) {
        gfx.set_font(this.font_index.v)
        gfx.set_text_color(this.color.r, this.color.g, this.color.b)
        gfx.print_str_at(this.x.v, this.y.v, this.text)
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

const DAY_NAMES = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"]

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
