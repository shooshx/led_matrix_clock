"use strict";

const DRAW_LINE_WIDTH = 2
const DRAW_PIXEL_SIZE = 20

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


    addPixel(x, y, r, g, b, a)
    {
        if (x < 0 || y < 0 || x >= this.width || y >= this.height)
            return        
        let i = (y * this.width + x)*4
        const prev_a = this.pixels[i+3]
        const na = Math.min(prev_a + a, 1.0)
        const nr = (this.pixels[i] *   (1 - a) + r * a)/na
        const ng = (this.pixels[i+1] * (1 - a) + g * a)/na
        const nb = (this.pixels[i+2] * (1 - a) + b * a)/na
        
        this.pixels[i] = nr
        this.pixels[i+1] = ng
        this.pixels[i+2] = nb
        this.pixels[i+3] = na * 255;

        this.cmd_cb.add(x, y, Math.trunc(nr * na), Math.trunc(ng * na), Math.trunc(nb * na))
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
    clear() {
        for(let i = 0; i < this.pixels.length; ++i)
            this.pixels[i] = 0
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

    canvas.addEventListener('mousedown', (e)=>{
        isDown = true
        prev_x = null
        prev_y = null
        call_tool(e);
    })
    document.addEventListener('mouseup', (e)=>{
        isDown = false;
        call_tool(e);
    })
    canvas.addEventListener('mouseleave', (e)=>{
        call_tool(e);
    })


    canvas.addEventListener('mousemove', (e)=>{
        call_tool(e)
    })

    canvas.addEventListener("touchstart", (e)=>{
        //console.log('t-start')
        e.preventDefault()
        isTouch = true
        prev_x = null
        prev_y = null
        call_tool(e.touches[0]);
    })
    canvas.addEventListener("touchend", (e)=>{
        //console.log('t-end')
        e.preventDefault()
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
    }
    clear() {
        this.accum.clear()
        this.ws.send("DC")
    }
}

function draw_pixel_create(root, w, h, ws)
{
    const top_elem = add_elem(root, 'div', 'draw_top')
    const control_elem = add_elem(top_elem, 'div', 'draw_control')

    const uq = new UpdatesQueue(ws)
    const s = new DrawState(top_elem, 'draw_canvas', w, h, uq)

    const clear_but = add_elem(control_elem, 'div', 'button')
    clear_but.innerText = 'Clear'
    clear_but.addEventListener('click', (e)=>{
        s.clear();
    })
    const col_in = add_elem(control_elem, 'input', 'draw_col_in')
    ColorEditBox.create_at(col_in, 300, (c)=>{ s.set_color(c) }, {}, ColorPicker.make_hex(s.tool_color, true))
    const size_in = add_elem(control_elem, 'input', ['draw_size_in', 'slider'])
    size_in.setAttribute('type', 'range')
    size_in.setAttribute('min', 0)
    size_in.setAttribute('max', 50)
    size_in.setAttribute('value', s.tool_radius * 10)
    size_in.addEventListener('change', (e)=>{
        s.set_tool_size(parseInt(size_in.value)/10)
    })

    //s.test_pattern()
    s.draw_cavnvas()

    connect_events(s.canvas, s)
}


