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

function add_num_input(parent, label, init_val, cb)
{
    if (label != null) {
        const lbl = add_elem(parent, 'label', 'num_in_label')
        lbl.innerText = label
    }
    const cont = add_elem(parent, 'div', 'num_in_cont')
    const minus_btn = add_elem(cont, 'div', ['num_in_minus', 'num_in_btn'])
    minus_btn.innerHTML = '&ndash;'

    const inp = add_elem(cont, 'input', 'num_input')
    inp.setAttribute('type', 'number')
    inp.setAttribute('readonly', true)
    inp.value = init_val
    inp.addEventListener('change', ()=>{ cb(inp.value) } )

    const plus_btn = add_elem(cont, 'div', ['num_in_plus', 'num_in_btn'])
    plus_btn.innerText = '+'

    minus_btn.addEventListener('click', ()=>{ inp.value = parseInt(inp.value) - 1; cb(inp.value) })
    plus_btn.addEventListener('click', ()=>{ inp.value = parseInt(inp.value) + 1; cb(inp.value) })
    return inp
}

function add_checkbox_input(parent, label, init_val, cb)
{
    const cont = add_elem(parent, 'div', 'in_check_cont')
    const inp = add_elem(cont, 'input', 'in_checkbox')
    inp.setAttribute('type', 'checkbox')
    const lbl = add_elem(cont, 'label', 'in_check_label')
    lbl.innerText = label

    const id = 'check_' + running_id++;
    inp.setAttribute('id', id)
    lbl.setAttribute('for', id)

    inp.checked = init_val
    inp.addEventListener('change', ()=>{ cb(inp.checked) })
}