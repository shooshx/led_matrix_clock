"use strict";

const DRAW_LINE_WIDTH = 2
const DRAW_PIXEL_SIZE = 10

class DrawState extends GfxCanvas
{
    constructor(parent_elem, canvas_name, w, h, cmd_cb)
    {
        super(w, h, DRAW_PIXEL_SIZE, parent_elem, DRAW_LINE_WIDTH, canvas_name)

        this.LINE_WIDTH = DRAW_LINE_WIDTH
        this.PIXEL_SIZE = DRAW_PIXEL_SIZE

        const l_tool_sz = localStorage['draw_cur_tool_sz']
        this.tool_radius = (l_tool_sz === undefined) ? 2 : parseFloat(l_tool_sz)
        this.tool_color = ColorPicker.parse_hex(localStorage['draw_cur_color'] || '#22AACC')

        this.tool_pradius = this.tool_radius * this.PIXEL_SIZE
        this.cmd_cb = cmd_cb

        // two layers: base is the brush drawing and what was loaded from the panel, the texts of the
        // text tool are on top of it. this.pixels is what's shown and sent to the panel
        this.base = new Uint8ClampedArray(this.pixels.length)
        this.text_px = new Map()  // pixel number -> {rgb, el} of the texts
        this.overlay_cb = null    // draws over the canvas, for the text cursor
        this.text_tool = null
        this.on_clear = null
        
        //for(let i = 0; i < this.pixels.length; ++i)
        //    this.pixels[i] = 0
        //for(let i = 3; i < this.pixels.length; i += 4)
        //    this.pixels[i] = 255; // all alphas
    }

    test_pattern()
    {
        for(let i = 0; i < 10; ++i)
            this.setPixel(i, i, 255, 0, 0)
    }


    // brush pixel with coverage a, into the base layer
    addPixel(x, y, r, g, b, a)
    {
        if (x < 0 || y < 0 || x >= this.width || y >= this.height)
            return
        const p = y * this.width + x, i = p * 4
        const prev_a = this.base[i+3]
        const na = Math.min(prev_a + a, 1.0)
        this.base[i] = (this.base[i] *   (1 - a) + r * a)/na
        this.base[i+1] = (this.base[i+1] * (1 - a) + g * a)/na
        this.base[i+2] = (this.base[i+2] * (1 - a) + b * a)/na
        this.base[i+3] = na * 255;
        this.show(p)
    }

    setPixel(x, y, r, g, b)
    {
        if (x < 0 || y < 0 || x >= this.width || y >= this.height)
            return
        const p = y * this.width + x, i = p * 4
        this.base[i] = r
        this.base[i+1] = g
        this.base[i+2] = b
        this.base[i+3] = 255
        this.show(p)
    }

    // updates the shown pixel from the layers and sends it to the panel if it changed
    show(p)
    {
        const i = p * 4
        const t = this.text_px.get(p)
        let r, g, b, a
        if (t !== undefined) {
            [r, g, b] = t.rgb
            a = 255
        }
        else {
            r = this.base[i]
            g = this.base[i+1]
            b = this.base[i+2]
            a = this.base[i+3]
        }
        const px = this.pixels
        if (px[i] == r && px[i+1] == g && px[i+2] == b && px[i+3] == a)
            return
        px[i] = r
        px[i+1] = g
        px[i+2] = b
        px[i+3] = a
        const f = a / 255
        this.cmd_cb.add(p % this.width, Math.floor(p / this.width), Math.trunc(r * f), Math.trunc(g * f), Math.trunc(b * f))
    }

    // a new text layer, the pixels that changed are shown and sent
    set_text_layer(m)
    {
        const old = this.text_px
        this.text_px = m
        for (const p of old.keys())
            this.show(p)
        for (const p of m.keys())
            if (!old.has(p))
                this.show(p)
    }

    // the pixels of these texts, as shown (which has what the panel changed), become part of the base layer,
    // before the texts are removed
    flatten(els)
    {
        for (const [p, t] of this.text_px) {
            if (!els.has(t.el))
                continue
            const i = p * 4
            for (let k = 0; k < 4; ++k)
                this.base[i + k] = this.pixels[i + k]
        }
    }

    // what was loaded from the panel (RGB per pixel). where it's what we show (in the panel's 565 precision)
    // the layers stay, the rest goes into the base layer. returns the texts that were changed on the panel
    merge_loaded(d)
    {
        const q = (r, g, b)=>((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
        const px = this.pixels
        const lost = new Set()
        for (let p = 0; p < this.width * this.height; ++p) {
            const i = p * 4
            const lr = d[p * 3], lg = d[p * 3 + 1], lb = d[p * 3 + 2]
            const f = px[i+3] / 255
            if (q(lr, lg, lb) == q(Math.trunc(px[i] * f), Math.trunc(px[i+1] * f), Math.trunc(px[i+2] * f)))
                continue
            const la = (lr | lg | lb) ? 255 : 0  // black is empty, like after Clear
            for (const arr of [this.base, px]) {
                arr[i] = lr
                arr[i+1] = lg
                arr[i+2] = lb
                arr[i+3] = la
            }
            const t = this.text_px.get(p)
            if (t !== undefined)
                lost.add(t.el)
        }
        return lost
    }

    draw_cavnvas()
    {
        this.draw()

        this.ctx.beginPath();
        for(let x = 1; x < this.canvas_width + 1; x += this.PIXEL_SIZE)
        {
            this.ctx.moveTo(x, 0)
            this.ctx.lineTo(x, this.canvas_height)
        }
        for(let y = 1; y < this.canvas_height + 1; y += this.PIXEL_SIZE)
        {
            this.ctx.moveTo(0, y)
            this.ctx.lineTo(this.canvas_width, y)
        }
        this.ctx.lineWidth = this.LINE_WIDTH
        this.ctx.strokeStyle = '#444444'
        this.ctx.stroke()
        if (this.overlay_cb !== null)
            this.overlay_cb(this.ctx)
    }

    brush(x, y)
    {
        if (this.tool_radius == 0) {
            const ix = Math.trunc(x), iy = Math.trunc(y)
            this.setPixel(ix, iy, this.tool_color.r, this.tool_color.g, this.tool_color.b)
            return
        }

        const x1 = Math.floor(x - this.tool_radius), x2 = Math.ceil(x + this.tool_radius)
        const y1 = Math.floor(y - this.tool_radius), y2 = Math.ceil(y + this.tool_radius)
        for(let ix = x1; ix <= x2; ++ix)
        {
            for(let iy = y1; iy <= y2; ++iy)
            {
                const dx = x - ix-0.5, dy = y - iy-0.5
                const d = Math.min(1.0, this.tool_radius - Math.sqrt(dx*dx + dy*dy))
                if (d < 0)
                    continue
                this.addPixel(ix, iy, this.tool_color.r, this.tool_color.g, this.tool_color.b, d)
                //this.setPixel(ix, iy, this.tool_color.r, this.tool_color.g, this.tool_color.b)
            }
        }

    }

    set_color(c) {
        this.tool_color = { r: c.r, g: c.g, b:c.b }
        localStorage['draw_cur_color'] = c.hex
    }
    set_tool_size(v) {
        this.tool_radius = v
        this.tool_pradius = this.tool_radius * this.PIXEL_SIZE
        localStorage['draw_cur_tool_sz'] = v
    }
    // also the texts, the panel clears its drawing and text
    clear() {
        for(let i = 0; i < this.pixels.length; ++i) {
            this.pixels[i] = 0
            this.base[i] = 0
        }
        this.text_px = new Map()
        if (this.on_clear !== null)
            this.on_clear()
        this.draw_cavnvas()
        this.cmd_cb.clear()
    }

    tool(x, y, px, py, do_step, act) {
        if (act && do_step)
        {
            this.brush(x, y, do_step)
        }

        this.draw_cavnvas()
    }

    show_tool(px, py) {
        this.ctx.beginPath();
        const r = (this.tool_pradius == 0) ? (0.5 * this.PIXEL_SIZE) : this.tool_pradius
        this.ctx.moveTo(px + r, py)
        this.ctx.arc(px, py, r, 0, 2*Math.PI);
        this.ctx.strokeStyle = '#666666'
        this.ctx.stroke()
    }
}



function connect_events(canvas, s)
{
    let isDown = false, isTouch = false
    let prev_x = null, prev_y = null
    const STEP_SIZE_FACTOR = 0.6
    const call_tool = (e)=>{
        if (e.target.getBoundingClientRect === undefined)
            return
        const rect = e.target.getBoundingClientRect();
        const v_scale = canvas.width / canvas.clientWidth
        const px = (e.clientX - rect.left) * v_scale 
        const py = (e.clientY - rect.top) * v_scale
        const in_canvas = (px >= 0 && py >= 0 && px < s.canvas_width && py < s.canvas_height)
        const x = px / s.PIXEL_SIZE
        const y = py / s.PIXEL_SIZE
        const dx = (x - prev_x), dy = (y - prev_y)
        const d = Math.sqrt(dx*dx + dy*dy)
        const step_pass = (d > STEP_SIZE_FACTOR * s.tool_radius)
        s.tool(x, y, px, py, step_pass, isDown || isTouch);
        if (step_pass)
        {
            prev_x = x
            prev_y = y
        }
        if (in_canvas)
            s.show_tool(px, py)
    }

    // while the text tool is on it gets the input instead of the brush
    const text = (kind, e)=>{
        if (s.text_tool === null || !s.text_tool.active)
            return false
        s.text_tool.on_pointer(kind, e)
        return true
    }

    canvas.addEventListener('mousedown', (e)=>{
        if (text('down', e)) {
            e.preventDefault()  // keep the focus in the text input
            return
        }
        isDown = true
        prev_x = null
        prev_y = null
        call_tool(e);
    })
    document.addEventListener('mouseup', (e)=>{
        if (text('up', e))
            return
        isDown = false;
        call_tool(e);
    })
    canvas.addEventListener('mouseleave', (e)=>{
        if (s.text_tool !== null && s.text_tool.active)
            return
        call_tool(e);
    })


    canvas.addEventListener('mousemove', (e)=>{
        if (text('move', e))
            return
        call_tool(e)
    })
    window.addEventListener('mousemove', (e)=>{
        if (e.target !== canvas && s.text_tool !== null && s.text_tool.active)
            s.text_tool.on_pointer('move', e)  // dragging a text outside the canvas
    })

    canvas.addEventListener("touchstart", (e)=>{
        //console.log('t-start')
        e.preventDefault()
        if (text('down', e.touches[0]))
            return
        isTouch = true
        prev_x = null
        prev_y = null
        call_tool(e.touches[0]);
    })
    canvas.addEventListener("touchend", (e)=>{
        //console.log('t-end')
        e.preventDefault()
        if (text('up', e))
            return
        isTouch = false
        call_tool(e);
    })
    canvas.addEventListener("touchcancel", (e)=>{
        //console.log('t-cancel')
        e.preventDefault()
        isTouch = false
        call_tool(e);
    });
    canvas.addEventListener("touchmove", (e)=>{
        //console.log('t-move')
        e.preventDefault()
        if (text('move', e.touches[0]))
            return
        call_tool(e.touches[0])
    });
}

function make_buf_cmd(cmd_t, elem_count, elem_sz)
{
    const cmd = new Uint8Array(2 + 4 + elem_count * elem_sz)
    let offset = 0
    cmd[offset++] = cmd_t.charCodeAt(0)
    cmd[offset++] = cmd_t.charCodeAt(1)
    const count_buf = new Uint32Array(1)
    count_buf[0] = elem_count
    const count_bufb = new Uint8Array(count_buf.buffer, 0, 4)
    for(let i = 0; i < 4; ++i)
        cmd[offset++] = count_bufb[i]
    return [cmd, offset]
}

// accumulate pixel changes to a single ws command that's send once a second
class UpdatesQueue {
    constructor(ws) {
        this.accum = new Map()
        this.ws = ws

        window.setInterval(()=>{
            if (this.accum.size == 0)
                return
            let [cmd, offset] = make_buf_cmd('DP', this.accum.size, 5)
            for (let [k, c] of this.accum) {
                for(let i = 0; i < 5; ++i)
                    cmd[offset++] = c[i];
            }
            this.accum.clear()
            ws.send(cmd)
            console.log("sent DP " + offset + " bytes")
        }, 500)
    }
    add(x, y, r, g, b) {
        this.accum.set("" + x + " " + y, [x, y, r, g, b])
        this.last_add = Date.now()
    }
    // clears the drawing on the panel, not the text
    clear() {
        this.accum.clear()
        this.ws.send("DX")
    }
    // drawing now or has pixels not sent yet
    busy() {
        return this.accum.size > 0 || (this.last_add !== undefined && Date.now() - this.last_add < 2000)
    }
}



function draw_pixel_create(root, w, h, ws)
{
    const top_elem = add_div(root, 'draw_top')
    const control_elem = add_div(top_elem, 'draw_control')

    const uq = new UpdatesQueue(ws)
    const s = new DrawState(top_elem, 'draw_canvas', w, h, uq)

    const clear_but = add_div(control_elem, 'button')
    clear_but.innerText = 'Clear'
    clear_but.addEventListener('click', (e)=>{
        s.clear();
    })
    const col_in = add_elem(control_elem, 'input', 'draw_col_in')
    const picker = ColorEditBox.create_at(col_in, 300, (c)=>{
        s.set_color(c)
        if (s.text_tool !== null)
            s.text_tool.set_color(c)
    }, {}, ColorPicker.make_hex(s.tool_color, true))

    add_slider(control_elem, 'draw_size_in', 0, 50, s.tool_radius * 10, (v)=>{
        s.set_tool_size(v/10)
    })

    s.text_tool = new TextTool(top_elem, control_elem, s, picker)
    add_panel_update(control_elem, s, uq)
    add_full_screen(top_elem, control_elem, s.canvas)

    //s.test_pattern()
    s.draw_cavnvas()

    connect_events(s.canvas, s)
}

// loads what the panel shows in the draw mode, the drawing and the text, into the canvas so that
// it can be drawn around. done(ok, msg)
function load_from_panel(s, done)
{
    const req = new XMLHttpRequest()
    req.open("GET", "/drawing?t=" + new Date().getTime())
    req.responseType = "arraybuffer"
    req.addEventListener("load", ()=>{
        if (req.status != 200) {
            done(false, "not available")
            return
        }
        const d = new Uint8Array(req.response)
        const w = d[0] | (d[1] << 8), h = d[2] | (d[3] << 8)
        if (w != s.width || h != s.height || d.length < 4 + w * h * 3) {
            done(false, "the panel turned, reload the page")
            return
        }
        const lost = s.merge_loaded(d.subarray(4))
        if (s.text_tool !== null)
            s.text_tool.forget(lost)  // changed on the panel, now just pixels
        s.draw_cavnvas()
        done(true, "")
    })
    req.addEventListener("error", ()=>{ done(false, "failed") })
    req.send()
}

// Update: shows what the panel shows. Live: does it whenever it changes on the panel (drawing with the remote,
// typing), checked every 1.5 seconds and not while drawing here
function add_panel_update(control_elem, s, uq)
{
    const btn = add_div(control_elem, 'button')
    btn.innerText = 'Update'
    let live = false
    let loaded_ver = null
    let loading = false
    const status = add_elem(control_elem, 'span', 'draw_status')

    const load = (ver)=>{
        loading = true
        load_from_panel(s, (ok, msg)=>{
            loading = false
            status.innerText = msg
            if (ok && ver !== null)
                loaded_ver = ver
        })
    }
    const get_ver = (cb)=>{
        const req = new XMLHttpRequest()
        req.open("GET", "/drawing_ver?t=" + new Date().getTime())
        req.addEventListener("load", ()=>{ if (req.status == 200) cb(req.responseText) })
        req.send()
    }
    btn.addEventListener('click', ()=>{ get_ver((v)=>{ load(v) }) })

    add_checkbox_input(control_elem, "Live", false, (v)=>{ live = v != 0 }, 'draw_live')
    window.setInterval(()=>{
        if (!live || loading || uq.busy() || s.canvas.offsetParent === null)
            return
        get_ver((v)=>{
            if (v != loaded_ver && !uq.busy())
                load(v)
        })
    }, 1500)
}

// a button that makes the draw area cover the whole window with the canvas as big as fits.
// where the browser allows it (not on iPhone) it also goes to real full screen, without the browser's bars
function add_full_screen(top_elem, control_elem, canvas)
{
    const btn = add_div(control_elem, 'button')
    btn.innerText = 'Full screen'
    let full = false

    // the biggest size with the canvas proportions that fits under the toolbar
    const fit = ()=>{
        if (!full)
            return
        const bar_h = control_elem.getBoundingClientRect().height
        const avail_w = window.innerWidth - 8
        const avail_h = window.innerHeight - bar_h - 12
        const aspect = canvas.width / canvas.height
        let w = avail_w, h = avail_w / aspect
        if (h > avail_h) {
            h = avail_h
            w = avail_h * aspect
        }
        canvas.style.width = Math.floor(w) + 'px'
        canvas.style.height = Math.floor(h) + 'px'
    }

    const real_full_elem = ()=>document.fullscreenElement || document.webkitFullscreenElement
    const set_full = (v)=>{
        if (v == full)
            return
        full = v
        top_elem.classList.toggle('draw_full', v)
        btn.innerText = v ? 'Exit full screen' : 'Full screen'
        document.body.style.overflow = v ? 'hidden' : ''  // no page scrolling under it
        if (v) {
            const req = top_elem.requestFullscreen || top_elem.webkitRequestFullscreen
            if (req) {
                try {
                    const p = req.call(top_elem)
                    if (p && p.catch)
                        p.catch(()=>{})  // not allowed, stays covering the window
                }
                catch(e) {}
            }
            fit()
        }
        else {
            canvas.style.width = ''
            canvas.style.height = ''
            if (real_full_elem()) {
                const exit = document.exitFullscreen || document.webkitExitFullscreen
                if (exit)
                    exit.call(document)
            }
        }
    }

    btn.addEventListener('click', ()=>{ set_full(!full) })
    window.addEventListener('resize', fit)
    window.addEventListener('orientationchange', ()=>{ window.setTimeout(fit, 200) })
    // leaving real full screen with the browser (Esc, swipe) leaves our mode too
    const on_fs_change = ()=>{
        if (!real_full_elem())
            set_full(false)
        else
            fit()
    }
    document.addEventListener('fullscreenchange', on_fs_change)
    document.addEventListener('webkitfullscreenchange', on_fs_change)
}




// Free text in the draw tab. the texts are drawn as pixels on top of the brush drawing and sent to the panel
// like the brush, but the web-ui remembers them so that they can be selected again, edited and moved
// (until the page is reloaded). with Text checked, pressing the canvas selects the text there or starts
// a new one where pressed, dragging a text moves it. a hidden input does the typing so that phones show
// their keyboard and the arrow keys move in the text.
class TextTool
{
    constructor(top_elem, control_elem, s, picker)
    {
        this.s = s
        this.picker = picker
        this.active = false
        this.texts = []        // {text, x, y, font, dbl, color:{r,g,b}, bbox}
        this.selected = null
        this.font = -1         // for new texts
        this.dbl = false
        this.cursor_on = true
        this.drag = null
        this.extents = {}      // font,size -> {top, bottom} of a line relative to y

        // renders the texts into a pixel map with the same fonts as the panel
        this.target = null
        this.target_el = null
        const collector = { setPixel: (x, y, r, g, b)=>{ this.collect(x, y, r, g, b) } }
        this.gfx = Module.gfx_init_display(s.width, s.height)
        // kept in a member: the display only has a pointer to it, if the js object is garbage collected
        // the wasm object is deleted and drawing crashes
        this.drawer = new Module.JsDraw(collector)
        this.gfx.set_drawer(this.drawer)

        const line = add_div(control_elem, 'draw_text_ctrl')
        this.make_check(line, "Text", (v)=>{ this.set_active(v) })
        this.font_sel = add_select(line, null, font_opts(), this.font, (v)=>{
            this.font = parseInt(v)
            if (this.selected !== null) {
                this.selected.font = this.font
                this.update()
            }
        })
        this.dbl_in = this.make_check(line, "2x", (v)=>{
            this.dbl = v
            if (this.selected !== null) {
                this.selected.dbl = v
                this.update()
            }
        })

        this.input = add_elem(top_elem, 'input', 'draw_text_input')
        this.input.setAttribute('type', 'text')
        this.input.setAttribute('autocomplete', 'off')
        this.input.setAttribute('spellcheck', 'false')
        this.input.addEventListener('input', ()=>{
            if (this.selected === null)
                return
            this.selected.text = this.input.value
            this.cursor_on = true
            this.update()
        })
        for (const ev of ['keyup', 'select', 'click'])
            this.input.addEventListener(ev, ()=>{ this.show_cursor() })
        this.input.addEventListener('keydown', (e)=>{
            if (e.key == 'Escape' || e.key == 'Enter') {
                this.select(null)
                this.input.blur()
            }
        })

        s.overlay_cb = (ctx)=>{ this.draw_overlay(ctx) }
        s.on_clear = ()=>{
            this.texts = []
            this.selected = null
            this.input.blur()
        }
        window.setInterval(()=>{
            if (this.selected !== null && this.active) {
                this.cursor_on = !this.cursor_on
                this.s.draw_cavnvas()
            }
        }, 500)
    }

    make_check(parent, label, cb) {
        const cont = add_div(parent, 'draw_live')
        const inp = add_elem(cont, 'input', 'in_checkbox')
        inp.setAttribute('type', 'checkbox')
        const lbl = add_elem(cont, 'label', 'in_check_label')
        lbl.innerText = label
        const id = 'check_' + running_id++
        inp.setAttribute('id', id)
        lbl.setAttribute('for', id)
        inp.addEventListener('change', ()=>{ cb(inp.checked) })
        return inp
    }

    set_active(v) {
        this.active = v
        if (!v) {
            this.select(null)
            this.input.blur()
        }
        this.s.draw_cavnvas()
    }

    // a pixel of the text being rendered
    collect(x, y, r, g, b) {
        if (x < 0 || y < 0 || x >= this.s.width || y >= this.s.height)
            return
        const el = this.target_el
        this.target.set(y * this.s.width + x, {rgb:[r, g, b], el:el})
        if (el.bbox === null)
            el.bbox = {x0:x, y0:y, x1:x, y1:y}
        else {
            el.bbox.x0 = Math.min(el.bbox.x0, x)
            el.bbox.y0 = Math.min(el.bbox.y0, y)
            el.bbox.x1 = Math.max(el.bbox.x1, x)
            el.bbox.y1 = Math.max(el.bbox.y1, y)
        }
    }

    set_font(el) {
        this.gfx.set_font(el.font)
        this.gfx.set_text_size(el.dbl ? 2 : 1)
    }

    render(el, m) {
        this.target = m
        this.target_el = el
        el.bbox = null
        this.set_font(el)
        this.gfx.set_text_color(el.color.r, el.color.g, el.color.b)
        this.gfx.print_str_at(el.x, el.y, el.text, ALIGN_LEFT)
        this.target = null
        this.target_el = null
    }

    // the text layer from all the texts, sends what changed
    update() {
        const m = new Map()
        for (const el of this.texts)
            this.render(el, m)
        this.s.set_text_layer(m)
        this.s.draw_cavnvas()
    }

    // top and bottom of a line of text relative to its y, for the cursor and for placing new texts
    line_extent(el) {
        const key = el.font + "," + (el.dbl ? 2 : 1)
        if (this.extents[key] === undefined) {
            const y = Math.floor(this.s.height / 2)
            const probe = {text:"Ay", x:0, y:y, font:el.font, dbl:el.dbl, color:{r:255, g:255, b:255}, bbox:null}
            this.render(probe, new Map())
            const b = probe.bbox
            this.extents[key] = (b === null) ? {top:0, bottom:7} : {top:b.y0 - y, bottom:b.y1 - y}
        }
        return this.extents[key]
    }

    // the text at panel pixel (x, y), the top one
    hit(x, y) {
        for (let i = this.texts.length - 1; i >= 0; --i) {
            const b = this.texts[i].bbox
            if (b !== null && x >= b.x0 - 1 && x <= b.x1 + 1 && y >= b.y0 - 1 && y <= b.y1 + 1)
                return this.texts[i]
        }
        return null
    }

    select(el) {
        const prev = this.selected
        this.selected = el
        if (prev !== null && prev !== el && prev.text == '') {
            this.texts = this.texts.filter((t)=>t !== prev)  // nothing was typed
            this.update()
        }
        if (el !== null) {
            this.font = el.font
            this.dbl = el.dbl
            this.font_sel.value = el.font
            this.dbl_in.checked = el.dbl
            this.picker.set_color({r:el.color.r, g:el.color.g, b:el.color.b, alpha:1})
            this.input.value = el.text
            this.input.focus()
            this.input.setSelectionRange(el.text.length, el.text.length)
        }
        this.cursor_on = true
        this.s.draw_cavnvas()
    }

    // the color picker changed
    set_color(c) {
        if (this.selected === null || !this.active)
            return
        const col = this.selected.color
        if (col.r == c.r && col.g == c.g && col.b == c.b)
            return
        this.selected.color = {r:c.r, g:c.g, b:c.b}
        this.update()
    }

    // the texts were changed on the panel, they stay as pixels but can't be edited any more
    forget(lost) {
        if (lost.size == 0)
            return
        this.s.flatten(lost)
        this.texts = this.texts.filter((t)=>!lost.has(t))
        if (this.selected !== null && lost.has(this.selected)) {
            this.selected = null
            this.input.blur()
        }
        this.update()
    }

    to_px(e) {
        const canvas = this.s.canvas
        const rect = canvas.getBoundingClientRect()
        const v_scale = canvas.width / canvas.clientWidth
        return [Math.floor((e.clientX - rect.left) * v_scale / this.s.PIXEL_SIZE),
                Math.floor((e.clientY - rect.top) * v_scale / this.s.PIXEL_SIZE)]
    }

    // mouse / touch on the canvas while Text is checked
    on_pointer(kind, e) {
        if (kind == 'up') {
            this.drag = null
            return
        }
        if (e === undefined || e.clientX === undefined)
            return
        const [x, y] = this.to_px(e)
        if (kind == 'down') {
            const el = this.hit(x, y)
            if (el !== null) {
                this.select(el)
                this.drag = {sx:x, sy:y, ox:el.x, oy:el.y}
                return
            }
            const nel = {text:'', x:x, y:y, font:this.font, dbl:this.dbl,
                         color:{r:this.s.tool_color.r, g:this.s.tool_color.g, b:this.s.tool_color.b}, bbox:null}
            nel.y = y - this.line_extent(nel).top  // the top of the text where pressed, also for fonts drawn from the baseline
            this.texts.push(nel)
            this.select(nel)
            this.drag = null
        }
        else if (kind == 'move' && this.drag !== null && this.selected !== null) {
            const nx = this.drag.ox + x - this.drag.sx, ny = this.drag.oy + y - this.drag.sy
            if (nx == this.selected.x && ny == this.selected.y)
                return
            this.selected.x = nx
            this.selected.y = ny
            this.update()
        }
    }

    show_cursor() {
        this.cursor_on = true
        this.s.draw_cavnvas()
    }

    // a dashed box around the selected text and the blinking cursor
    draw_overlay(ctx) {
        const el = this.selected
        if (el === null || !this.active)
            return
        const sx = this.s.canvas_width / this.s.width, sy = this.s.canvas_height / this.s.height
        if (el.bbox !== null) {
            ctx.save()
            ctx.strokeStyle = '#ff40ff'
            ctx.lineWidth = 2
            ctx.setLineDash([6, 4])
            ctx.strokeRect((el.bbox.x0 - 1) * sx, (el.bbox.y0 - 1) * sy, (el.bbox.x1 - el.bbox.x0 + 3) * sx, (el.bbox.y1 - el.bbox.y0 + 3) * sy)
            ctx.restore()
        }
        if (!this.cursor_on)
            return
        const idx = (document.activeElement === this.input && this.input.selectionStart !== null) ? this.input.selectionStart : el.text.length
        this.set_font(el)
        const cx = el.x + this.gfx.calc_str_width(el.text.slice(0, idx))
        const ext = this.line_extent(el)
        ctx.fillStyle = '#ffffff'
        ctx.fillRect(cx * sx - 1, (el.y + ext.top) * sy, 3, (ext.bottom - ext.top + 1) * sy)
    }
}
