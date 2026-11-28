"use strict";

// ---- the background effects of the clock faces, same as Effects.h in the firmware ----
// every effect is a function of the time t (seconds), the pixel and the settings: no state from frame to frame,
// the drops, flakes, fireflies... have properties from a hash of their number and their place from the time.
// the settings: color, size (1-64, what it means is FX_LIST's size label), speed (1-10), pulse (all of it
// brightens and dims, only Border, Slide and Aurora), angle (degrees, the direction Slide moves in)
// the panel lights the effects with a gamma (led_level() in Effects.h) so they look like here

const FX_NONE = 0, FX_BORDER = 1, FX_SLIDE = 2, FX_TWINKLE = 3, FX_FIREFLIES = 4, FX_SKY_REMOVED = 5, FX_AURORA = 6,
      FX_RAIN = 7, FX_SNOW = 8, FX_FIRE = 9, FX_RIPPLES = 10, FX_RADAR = 11, FX_PLASMA = 12,
      FX_WARP = 13, FX_LAVA = 14, FX_TUNNEL = 15, FX_CUBE = 16

// in the order of the effect select. size: the label of the size setting. Sky (5) was removed
const FX_LIST = [
    {value:FX_NONE, text:"None", size:""},
    {value:FX_BORDER, text:"Border", size:"Thickness"},
    {value:FX_SLIDE, text:"Slide", size:"Width"},
    {value:FX_TWINKLE, text:"Twinkle", size:"Stars"},
    {value:FX_FIREFLIES, text:"Fireflies", size:"Fireflies"},
    {value:FX_AURORA, text:"Aurora", size:"Band width"},
    {value:FX_RAIN, text:"Rain", size:"Drops"},
    {value:FX_SNOW, text:"Snow", size:"Flakes"},
    {value:FX_FIRE, text:"Fire", size:"Height"},
    {value:FX_RIPPLES, text:"Ripples", size:"Rings"},
    {value:FX_RADAR, text:"Radar", size:"Trail"},
    {value:FX_PLASMA, text:"Plasma", size:"Scale"},
    {value:FX_WARP, text:"Warp", size:"Stars"},
    {value:FX_LAVA, text:"Lava", size:"Blob size"},
    {value:FX_TUNNEL, text:"Tunnel", size:"Stripes"},
    {value:FX_CUBE, text:"Cube", size:"Cube size"},
]

const FX_SIZE_MAX = 64

// the pulse setting is only for these, the others have their own movement
function fx_can_pulse(fx) {
    return fx == FX_BORDER || fx == FX_SLIDE || fx == FX_AURORA || fx == FX_TUNNEL
}

// it moves, it's drawn again ~25 times a second
function fx_animated(fx, pulse) {
    if (fx == FX_NONE || fx == FX_SKY_REMOVED)
        return false
    if (fx == FX_BORDER)
        return pulse != 0
    return true
}

const TAU = 2 * Math.PI

// the same 32 bit hash as fx_hash() in the firmware
function fx_hash(n) {
    n = n >>> 0
    n ^= n >>> 16
    n = Math.imul(n, 0x7feb352d) >>> 0
    n ^= n >>> 15
    n = Math.imul(n, 0x846ca68b) >>> 0
    n ^= n >>> 16
    return n >>> 0
}
function fx_rnd(n) {   // 0..1
    return fx_hash(n) / 4294967296
}

// sine from a table, like the firmware (the panel can't do thousands of sinf() a frame)
const FX_SIN_N = 1024
const FX_SIN = new Float32Array(FX_SIN_N).map((_, i)=>Math.sin(i * TAU / FX_SIN_N))
function fsin(x) {
    return FX_SIN[Math.floor(x * (FX_SIN_N / TAU)) & (FX_SIN_N - 1)]
}
function frac(x) {
    return x - Math.floor(x)
}
function clamp01(x) {
    return x < 0 ? 0 : x > 1 ? 1 : x
}

// smooth noise 0..1 on a grid of random values
function fx_noise(x, y) {
    const xi = Math.floor(x), yi = Math.floor(y)
    const fx = x - xi, fy = y - yi
    const u = fx * fx * (3 - 2 * fx), v = fy * fy * (3 - 2 * fy)
    const at = (i, j)=>fx_rnd(((i & 0xffff) | ((j & 0xffff) << 16)) >>> 0)
    const a = at(xi, yi), b = at(xi + 1, yi), c = at(xi, yi + 1), d = at(xi + 1, yi + 1)
    return (a + (b - a) * u) + ((c + (d - c) * u) - (a + (b - a) * u)) * v
}

// rgb (0-255) to hue (0-360), and back with saturation / value 0-1
function fx_hue(r, g, b) {
    const mx = Math.max(r, g, b), mn = Math.min(r, g, b), d = mx - mn
    if (d == 0)
        return 0
    let h = mx == r ? ((g - b) / d) % 6 : mx == g ? (b - r) / d + 2 : (r - g) / d + 4
    return ((h * 60) + 360) % 360
}
function fx_hsv(h, s, v) {
    h = ((h % 360) + 360) % 360
    const c = v * s, x = c * (1 - Math.abs((h / 60) % 2 - 1)), m = v - c
    const [r, g, b] = h < 60 ? [c, x, 0] : h < 120 ? [x, c, 0] : h < 180 ? [0, c, x] :
                      h < 240 ? [0, x, c] : h < 300 ? [x, 0, c] : [c, 0, x]
    return [Math.round((r + m) * 255), Math.round((g + m) * 255), Math.round((b + m) * 255)]
}

// the radar's angle of each pixel from the middle, in turns, 1/256 steps. for each size
const g_radar_angles = new Map()
function radar_angles(w, h) {
    const key = w + "x" + h
    if (!g_radar_angles.has(key)) {
        const a = new Uint8Array(w * h)
        const cx = (w - 1) / 2, cy = (h - 1) / 2
        for (let y = 0; y < h; ++y)
            for (let x = 0; x < w; ++x)
                a[y * w + x] = Math.floor(frac(Math.atan2(y - cy, x - cx) / TAU) * 256) & 255
        g_radar_angles.set(key, a)
    }
    return g_radar_angles.get(key)
}

// the tunnel's angle (1/256 turns), depth (1 / the distance from the middle, 1/64 steps) and brightness (0-255,
// dark in the middle, far away) of each pixel of an area twice the display's size in each direction, its middle
// in the area's middle. the tunnel's middle moves around by drawing from another place of it (the demoscene way).
// for each size
const TUNNEL_DEPTH = 32
const g_tunnel = new Map()
function tunnel_tables(w, h) {
    const key = w + "x" + h
    if (!g_tunnel.has(key)) {
        const tw = 2 * w, th = 2 * h
        const angle = new Uint8Array(tw * th), depth = new Uint16Array(tw * th), fog = new Uint8Array(tw * th)
        const cx = w - 0.5, cy = h - 0.5  // at the display's middle when it isn't moved
        const edge = Math.min(w, h) * 0.45
        for (let y = 0; y < th; ++y)
            for (let x = 0; x < tw; ++x) {
                const p = y * tw + x
                const dx = x - cx, dy = y - cy
                const d = Math.max(0.75, Math.sqrt(dx * dx + dy * dy))
                depth[p] = Math.trunc(Math.min(65535, TUNNEL_DEPTH / d * 64))
                const f = clamp01(d / edge)
                fog[p] = Math.trunc(f * f * 255)
                angle[p] = Math.floor(frac(Math.atan2(dy, dx) / TAU) * 256) & 255
            }
        g_tunnel.set(key, {angle, depth, fog, tw})
    }
    return g_tunnel.get(key)
}

// a circle of radius r around cx, cy (midpoint), set(x, y) for each pixel
function fx_circle(cx, cy, r, set) {
    if (r <= 0) {
        set(cx, cy)
        return
    }
    let x = r, y = 0, err = 1 - r
    while (x >= y) {
        set(cx + x, cy + y); set(cx + y, cy + x); set(cx - y, cy + x); set(cx - x, cy + y)
        set(cx - x, cy - y); set(cx - y, cy - x); set(cx + y, cy - x); set(cx + x, cy - y)
        ++y
        if (err < 0)
            err += 2 * y + 1
        else {
            --x
            err += 2 * (y - x) + 1
        }
    }
}

// the background effect into the canvas, t in seconds.
// sp is like ClockFace: fx, fx_color, fx_size, fx_speed, fx_pulse, fx_angle
function draw_effect(canvas, w, h, sp, t)
{
    const fx = sp.fx.v
    const {r, g, b} = sp.fx_color
    const size = Math.min(FX_SIZE_MAX, Math.max(1, sp.fx_size.v))
    const speed = Math.min(10, Math.max(1, sp.fx_speed.v))
    const angle = sp.fx_angle ? sp.fx_angle.v : 0
    const k = (sp.fx_pulse.v && fx_can_pulse(fx)) ? 0.6 + 0.4 * Math.sin(TAU * t * speed / 6.0) : 1.0
    // the effect's color at a brightness 0..1, nothing when it's ~black (the background stays)
    const dot = (x, y, a)=>{
        a *= k
        if (a < 0.004 || x < 0 || y < 0 || x >= w || y >= h)
            return
        a = Math.min(1, a)
        canvas.setPixel(x, y, Math.trunc(r * a), Math.trunc(g * a), Math.trunc(b * a))
    }
    // a color of its own
    const rgb = (x, y, cr, cg, cb)=>{
        if (x < 0 || y < 0 || x >= w || y >= h)
            return
        canvas.setPixel(x, y, Math.trunc(Math.min(255, cr * k)), Math.trunc(Math.min(255, cg * k)), Math.trunc(Math.min(255, cb * k)))
    }

    if (fx == FX_BORDER) {
        // fades to black towards the middle
        for (let y = 0; y < h; ++y)
            for (let x = 0; x < w; ++x) {
                const d = Math.min(x, y, w - 1 - x, h - 1 - y)
                if (d >= size)
                    continue
                const a = 1.0 - d / size
                dot(x, y, a * a)
            }
    }
    else if (fx == FX_SLIDE) {
        // a band that sweeps from side to side, across the direction of the angle (0: left - right, 90: up - down)
        const ang = angle * TAU / 360
        const ca = Math.cos(ang), sa = Math.sin(ang)
        // where the pixels are along the direction: from the panel's corners
        const ps = [0, w * ca, h * sa, w * ca + h * sa]
        const p0 = Math.min(...ps), p1 = Math.max(...ps)
        const period = 12.0 / speed
        const ph = (t % period) / period
        const tri = ph < 0.5 ? ph * 2 : 2 - ph * 2
        const half = size / 2.0
        const cp = p0 - half + tri * (p1 - p0 + size)
        for (let y = 0; y < h; ++y) {
            const py = (y + 0.5) * sa
            for (let x = 0; x < w; ++x) {
                const a = 1.0 - Math.abs((x + 0.5) * ca + py - cp) / half
                if (a > 0)
                    dot(x, y, a * a)
            }
        }
    }
    else if (fx == FX_TWINKLE) {
        // stars in fixed places, each fades in and out at its own rate, mostly dark
        const n = size * 6
        for (let i = 0; i < n; ++i) {
            const x = fx_hash(i * 3 + 1) % w, y = fx_hash(i * 3 + 2) % h
            const rate = (0.3 + 0.7 * fx_rnd(i * 3 + 3)) * speed / 5
            const s = fsin(TAU * (t * rate + fx_rnd(i * 7 + 5)))
            if (s > 0)
                dot(x, y, s * s * s)
        }
    }
    else if (fx == FX_FIREFLIES) {
        // soft dots that wander (two slow waves each) and glow
        const n = size
        for (let i = 0; i < n; ++i) {
            const sp5 = speed / 5
            const fx_ = 0.05 + 0.1 * fx_rnd(i * 11 + 1), fy_ = 0.05 + 0.1 * fx_rnd(i * 11 + 2)
            const x = Math.round((w - 1) / 2 + (w - 3) / 2 * fsin(TAU * (t * fx_ * sp5 + fx_rnd(i * 11 + 3))))
            const y = Math.round((h - 1) / 2 + (h - 3) / 2 * fsin(TAU * (t * fy_ * sp5 + fx_rnd(i * 11 + 4))))
            const glow = 0.5 + 0.5 * fsin(TAU * (t * (0.3 + 0.5 * fx_rnd(i * 11 + 6)) + fx_rnd(i * 11 + 5)))
            dot(x, y, glow)
            for (const [dx, dy] of [[1, 0], [-1, 0], [0, 1], [0, -1]])
                dot(x + dx, y + dy, glow * 0.35)
            for (const [dx, dy] of [[1, 1], [-1, 1], [1, -1], [-1, -1]])
                dot(x + dx, y + dy, glow * 0.12)
        }
    }
    else if (fx == FX_AURORA) {
        // three soft bands. each is two waves that travel at their own speeds in opposite directions, drifts up and
        // down slowly, gets wider and narrower along it, and a brighter part runs along it
        const wd = Math.max(1, size / 2)
        const band_shine = [1, 0.7, 0.5]
        const sp = speed / 3
        const drift = [0, 1, 2].map((j)=>h * (0.25 + 0.25 * j) + h * 0.1 * fsin(TAU * t * sp * (0.011 + 0.007 * j) + j * 2.1))
        for (let x = 0; x < w; ++x) {
            const u = x / w
            const centers = [], shine = [], width = []
            for (let j = 0; j < 3; ++j) {
                centers.push(drift[j]
                    + h * 0.13 * fsin(TAU * (u * (0.8 + 0.45 * j) + t * sp * (0.05 + 0.03 * j)) + j * 1.7)
                    + h * 0.07 * fsin(TAU * (u * (2.1 + 0.6 * j) - t * sp * (0.11 + 0.05 * j)) + j * 4.1))
                width.push(wd * (0.7 + 0.3 * fsin(TAU * (u * 1.3 + t * sp * (0.04 + 0.02 * j)) + j * 2.9)))
                const sh = 0.6 + 0.25 * fsin(TAU * (u * 1.7 - t * sp * (0.09 + 0.04 * j)) + j)
                               + 0.15 * fsin(TAU * (u * 3.3 + t * sp * (0.15 - 0.03 * j)) + j * 3.3)
                shine.push(sh * band_shine[j])
            }
            for (let y = 0; y < h; ++y) {
                let a = 0
                for (let j = 0; j < 3; ++j) {
                    const d = 1 - Math.abs(y - centers[j]) / width[j]
                    if (d > 0)
                        a += d * d * shine[j]
                }
                if (a > 0)
                    dot(x, y, Math.min(1, a))
            }
        }
    }
    else if (fx == FX_RAIN) {
        // drops falling in columns, a bright head and a fading tail
        const n = size * 2
        for (let i = 0; i < n; ++i) {
            const x = fx_hash(i * 13 + 1) % w
            const v = (20 + 40 * fx_rnd(i * 13 + 2)) * speed / 5
            const len = 3 + fx_hash(i * 13 + 3) % 6
            const period = h + len
            const head = Math.floor(((t * v + fx_rnd(i * 13 + 4) * period) % period + period) % period)
            for (let j = 0; j < len; ++j) {
                const a = 1 - j / len
                dot(x, head - j, a * a)
            }
        }
    }
    else if (fx == FX_SNOW) {
        // flakes falling slowly, swaying a little
        const n = size * 2
        for (let i = 0; i < n; ++i) {
            const v = (4 + 8 * fx_rnd(i * 17 + 2)) * speed / 5
            const period = h + 2
            const y = Math.round(((t * v + fx_rnd(i * 17 + 3) * period) % period + period) % period - 1)
            const sway = 1.5 * fsin(TAU * (t * (0.1 + 0.2 * fx_rnd(i * 17 + 4)) + fx_rnd(i * 17 + 5)))
            const x = ((Math.round(fx_rnd(i * 17 + 1) * w + sway) % w) + w) % w
            dot(x, y, 0.6 + 0.4 * fx_rnd(i * 17 + 6))
        }
    }
    else if (fx == FX_FIRE) {
        // flames from the bottom: noise that rises, hotter below. the color, then towards white where it's hottest
        const fh = Math.min(h, Math.max(4, size * 2))
        for (let y = h - fh; y < h; ++y) {
            const rel = (h - 1 - y) / fh
            for (let x = 0; x < w; ++x) {
                const n = 0.65 * fx_noise(x * 0.25, (y + t * speed * 3) * 0.25) + 0.35 * fx_noise(x * 0.6, (y + t * speed * 5) * 0.6)
                const heat = clamp01(n * 1.4 - rel * 1.2 + 0.05)
                if (heat < 0.02)
                    continue
                if (heat < 0.5)
                    rgb(x, y, r * heat / 0.5, g * heat / 0.5, b * heat / 0.5)
                else {
                    const wt = (heat - 0.5) / 0.5 * 0.7
                    rgb(x, y, r + (255 - r) * wt, g + (255 - g) * wt, b + (255 - b) * wt)
                }
            }
        }
    }
    else if (fx == FX_RIPPLES) {
        // rings that grow from random places and fade, a few at a time
        const n = Math.max(1, Math.floor(size / 4))
        const life = 4.0 * 5 / speed
        const max_r = Math.max(w, h) * 0.6
        for (let j = 0; j < n; ++j) {
            const s = t / life + j / n
            const c = Math.floor(s), age = s - c
            const id = (c * 16 + j) >>> 0
            const cx = fx_hash(id * 2 + 1) % w, cy = fx_hash(id * 2 + 2) % h
            const a = (1 - age) * (1 - age)
            fx_circle(cx, cy, Math.round(age * max_r), (x, y)=>dot(x, y, a))
        }
    }
    else if (fx == FX_RADAR) {
        // a sweep around the middle with a fading trail, a whole turn at 64
        const angles = radar_angles(w, h)
        const sweep = frac(t * speed * 0.05)
        const trail = Math.min(1, Math.max(0.03, size / FX_SIZE_MAX))
        for (let y = 0; y < h; ++y)
            for (let x = 0; x < w; ++x) {
                const behind = frac(sweep - angles[y * w + x] / 256 + 1)
                if (behind < trail) {
                    const a = 1 - behind / trail
                    dot(x, y, a * a)
                }
            }
        dot(Math.floor((w - 1) / 2), Math.floor((h - 1) / 2), 1)
    }
    else if (fx == FX_PLASMA) {
        // waves of hues around the color's hue. the place is warped by slow waves (not straight lines), two waves
        // that turn around slowly, and rings from a point that wanders
        // the size: how big the waves are, the bigger ones grow slower (64 is a few blobs, not one)
        const sc = 3.0 / Math.sqrt(size)
        // a palette that goes around: from the color's hue to 90 degrees each way and back, darker and brighter
        // twice on the way. the plasma's value goes around it twice, every value is colorful
        const base = fx_hue(r, g, b)
        const pal = []
        for (let i = 0; i < 64; ++i) {
            const u = i / 64
            const tri = 1 - Math.abs(2 * u - 1)  // 0..1..0
            pal.push(fx_hsv(base + (tri - 0.5) * 180, 1, 0.35 + 0.45 * (0.5 - 0.5 * Math.cos(TAU * 2 * u))))
        }
        const ts = t * speed
        // two directions that turn, one each way
        const a1 = ts * 0.021, a2 = 2.0 - ts * 0.016
        const d1x = Math.cos(a1) * 0.23 * sc, d1y = Math.sin(a1) * 0.23 * sc
        const d2x = Math.cos(a2) * 0.17 * sc, d2y = Math.sin(a2) * 0.17 * sc
        // the rings' middle
        const cx = w * (0.5 + 0.4 * Math.sin(ts * 0.037)), cy = h * (0.5 + 0.4 * Math.sin(ts * 0.029 + 1.3))
        const t1 = ts * 0.31, t2 = ts * 0.23, t3 = ts * 0.27, t4 = ts * 0.13
        const rk = 0.25 * sc
        // the warp of a row depends on its y, of a column on its x
        const warp_x = [], warp_y = []
        for (let x = 0; x < w; ++x)
            warp_x.push(4.0 * fsin(x * 0.11 * sc + t4))
        for (let y = 0; y < h; ++y)
            warp_y.push(4.0 * fsin(y * 0.13 * sc - t4 * 1.3))
        for (let y = 0; y < h; ++y)
            for (let x = 0; x < w; ++x) {
                const wx = x + warp_y[y], wy = y + warp_x[x]
                const dx = wx - cx, dy = wy - cy
                const v = fsin(wx * d1x + wy * d1y + t1) + fsin(wx * d2x + wy * d2y - t2)
                        + fsin(Math.sqrt(dx * dx + dy * dy) * rk - t3) + fsin(wx * 0.07 * sc - wy * 0.11 * sc + t4)
                // around the palette twice from v = -4 to 4, and it drifts slowly
                const c = pal[Math.floor(frac(v * 0.25 + ts * 0.004) * 64) & 63]
                rgb(x, y, c[0], c[1], c[2])
            }
    }
    else if (fx == FX_WARP) {
        // stars that start near the middle in a direction of their own and go faster as they get out, brighter and
        // with a longer streak. when it's out it starts again in another direction
        const cx = (w - 1) / 2, cy = (h - 1) / 2
        const out = Math.sqrt(cx * cx + cy * cy) + 2
        const n = size * 2
        for (let i = 0; i < n; ++i) {
            const rate = (0.12 + 0.2 * fx_rnd(i * 19 + 1)) * speed / 5
            const s = t * rate + fx_rnd(i * 19 + 2)
            const c = Math.floor(s), age = s - c
            const ang = TAU * fx_rnd((Math.imul(c, 977) + i * 19 + 3) >>> 0)
            const dx = fsin(ang + TAU / 4), dy = fsin(ang)
            const d = 1 + age * age * out
            const a = Math.min(1, 0.15 + age * 1.3)
            const len = 1 + Math.trunc(age * age * 8)
            for (let j = 0; j < len; ++j) {
                const dj = d - j
                if (dj < 0)
                    break
                dot(Math.round(cx + dx * dj), Math.round(cy + dy * dj), a * (1 - j / len))
            }
        }
    }
    else if (fx == FX_LAVA) {
        // blobs that float on slow waves. where they are near each other they melt together: every pixel adds up
        // how near it is to each, the shape is where that's over a threshold, with a soft edge. hues around the color
        const nb = 6
        const rad = 3 + size * 0.35, r2 = rad * rad
        const sp = speed / 5
        const bx = [], by = []
        for (let j = 0; j < nb; ++j) {
            const fx_ = 0.015 + 0.03 * fx_rnd(j * 23 + 1), fy_ = 0.015 + 0.03 * fx_rnd(j * 23 + 2)
            bx.push((w - 1) / 2 + (w * 0.42) * fsin(TAU * (t * fx_ * sp + fx_rnd(j * 23 + 3))))
            by.push((h - 1) / 2 + (h * 0.42) * fsin(TAU * (t * fy_ * sp + fx_rnd(j * 23 + 4))))
        }
        const base = fx_hue(r, g, b)
        const pal = []
        for (let i = 0; i < 32; ++i)
            pal.push(fx_hsv(base + (1 - i / 31) * 25, 1, 0.25 + 0.5 * i / 31))
        for (let y = 0; y < h; ++y)
            for (let x = 0; x < w; ++x) {
                let f = 0
                for (let j = 0; j < nb; ++j) {
                    const dx = x - bx[j], dy = y - by[j]
                    f += r2 / (dx * dx + dy * dy + 1)
                }
                if (f < 0.6)
                    continue
                // the edge from 0.6 to 1, then brighter towards the middles
                const edge = clamp01((f - 0.6) / 0.4)
                const c = pal[Math.trunc(clamp01(1 - 1 / f) * 31)]
                const e = edge * edge
                rgb(x, y, c[0] * e, c[1] * e, c[2] * e)
            }
    }
    else if (fx == FX_TUNNEL) {
        // stripes in the depth (rings coming out of the middle) and around it (the size: how many), twisting,
        // dark far away in the middle. now and then the tube turns: its far end bends to a side (mostly left or
        // right) while the near part stays, then it straightens again or turns the other way
        const {angle, depth, fog, tw} = tunnel_tables(w, h)
        const fly = t * speed * 0.12, spin = t * speed * 0.03
        const around = TAU * size / 256  // per 1/256 turn
        // the bend: one for each stretch of time, straight about half the times. it stays for a while, then
        // changes smoothly to the next one
        const seg = 3 + 30 / speed
        const s = t / seg
        const i = Math.floor(s)
        let k = clamp01((s - i - 0.55) / 0.45)
        k = k * k * (3 - 2 * k)
        const bend = (n)=>{
            const u = (n * 7) >>> 0
            const straight = fx_rnd(u + 1) < 0.45
            return straight ? [0, 0] : [(fx_rnd(u + 2) * 2 - 1) * w * 0.4, (fx_rnd(u + 3) * 2 - 1) * h * 0.15]
        }
        const [ax, ay] = bend(i), [bx2, by2] = bend(i + 1)
        const bend_x = ax + (bx2 - ax) * k, bend_y = ay + (by2 - ay) * k  // where the far end's middle is
        // the middle of the ring at depth z is moved by bend * (1 - BEND_NEAR / z) from the display's middle: the
        // rings at the display's edges (z ~ BEND_NEAR) stay, the deep ones go all the way. it never moves a ring
        // past a nearer one (the bend grows slower than the rings shrink), so every pixel is on one ring, found by
        // going from the depth of the straight tube to the bent one a few times (it gets closer each time)
        const BEND_NEAR = 0.4, BEND_ROUNDS = 5
        const cx = Math.floor(w / 2), cy = Math.floor(h / 2)
        for (let y = 0; y < h; ++y) {
            for (let x = 0; x < w; ++x) {
                const p0 = (y + cy) * tw + x + cx
                let p = p0
                for (let it = 0; it < BEND_ROUNDS; ++it) {
                    const g = 1 - BEND_NEAR / Math.max(BEND_NEAR, depth[p] / 64)
                    p = p0 - Math.floor(bend_y * g + 0.5) * tw - Math.floor(bend_x * g + 0.5)
                }
                const z = depth[p] / 64
                const rings = fsin(TAU * (z * 1.5 + fly))
                const spokes = fsin(angle[p] * around + TAU * (z * 0.12 + spin))
                const a = 0.5 + 0.5 * rings * spokes
                dot(x, y, a * a * fog[p] / 255)
            }
        }
    }
    else if (fx == FX_CUBE) {
        // a wireframe cube that tumbles and bounces off the sides, its far edges dimmer. a straight path that
        // bounces off the sides is a zigzag in x and another in y, so it's from the time too. it bounces where the
        // sphere around its drawing touches the side
        const bound = Math.min(Math.min(w, h) / 2 - 1, 4 + size * 0.45)  // the most its drawing reaches from its middle
        const rad = bound * 0.75             // its corners from its middle
        const half = rad / 1.7320508         // half its edge
        const cam = rad * 4                  // the eye's distance: the nearest corner is drawn 4/3 bigger
        const v = speed * 3.0                // pixels a second
        // where it is along the zigzag doesn't depend on the size (the room it has): a new size doesn't make it
        // jump to another place, it stays as far along between the sides
        const zigzag = (lo, hi, vv, ph, full)=>{
            const range = hi - lo
            if (range <= 0)
                return (lo + hi) / 2
            const u = frac(t * vv / (2 * full) + ph)
            return lo + range * (1 - Math.abs(1 - 2 * u))
        }
        const mx = zigzag(bound, w - 1 - bound, v, 0.13, w - 1)
        const my = zigzag(bound, h - 1 - bound, v * 0.83, 0.61, h - 1)
        const ax = t * speed * 0.11, ay = t * speed * 0.17, az = t * speed * 0.05
        const sxa = Math.sin(ax), cxa = Math.cos(ax), sya = Math.sin(ay), cya = Math.cos(ay), sza = Math.sin(az), cza = Math.cos(az)
        const px = [], py = [], pz = []
        for (let i = 0; i < 8; ++i) {
            const x = i & 1 ? half : -half, y = i & 2 ? half : -half, z = i & 4 ? half : -half
            const y1 = y * cxa - z * sxa, z1 = y * sxa + z * cxa      // around x
            const x2 = x * cya + z1 * sya, z2 = -x * sya + z1 * cya   // around y
            const x3 = x2 * cza - y1 * sza, y3 = x2 * sza + y1 * cza  // around z
            const s = cam / (cam + z2)  // z towards the eye is negative
            px.push(mx + x3 * s)
            py.push(my + y3 * s)
            pz.push(z2)
        }
        // the lines into a buffer, the brightest where they meet (a corner of a dim line doesn't hide a bright one)
        const lines = new Uint8Array(w * h), touched = []
        const plot = (x, y, a)=>{
            if (x < 0 || y < 0 || x >= w || y >= h || a <= 0)
                return
            const p = y * w + x
            const c = Math.trunc(Math.min(255, a * 255))
            if (lines[p] == 0)
                touched.push(p)
            if (c > lines[p])
                lines[p] = c
        }
        // anti-aliased: each step along the line lights the two pixels around it, by how near they are
        const line = (x0, y0, x1, y1, a)=>{
            const steep = Math.abs(y1 - y0) > Math.abs(x1 - x0)
            if (steep)
                [x0, y0, x1, y1] = [y0, x0, y1, x1]
            if (x0 > x1)
                [x0, y0, x1, y1] = [x1, y1, x0, y0]
            const g = x1 - x0 > 0 ? (y1 - y0) / (x1 - x0) : 0
            for (let x = Math.round(x0); x <= Math.round(x1); ++x) {
                const y = y0 + (x - x0) * g
                const yi = Math.floor(y), f = y - yi
                if (steep) {
                    plot(yi, x, a * (1 - f))
                    plot(yi + 1, x, a * f)
                }
                else {
                    plot(x, yi, a * (1 - f))
                    plot(x, yi + 1, a * f)
                }
            }
        }
        for (let i = 0; i < 8; ++i)
            for (let bit = 1; bit < 8; bit <<= 1) {
                if (i & bit)
                    continue
                const j = i | bit
                const near = (1 - (pz[i] + pz[j]) / 2 / rad) / 2  // 0 far .. 1 near
                line(px[i], py[i], px[j], py[j], 0.3 + 0.7 * near)
            }
        for (const p of touched)
            dot(p % w, Math.floor(p / w), lines[p] / 255)
    }
}
