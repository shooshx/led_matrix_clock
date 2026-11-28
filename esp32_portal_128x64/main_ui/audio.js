"use strict";

const AUDIO_MIN_FREQ = 50
const AUDIO_MAX_FREQ = 5000

// the names of the sounds on the panel (its /sounds folder), asked once
let g_sounds_promise = null
function get_sounds() {
    if (g_sounds_promise === null)
        g_sounds_promise = fetch('sounds').then((r)=>r.json()).catch((e)=>{
            g_sounds_promise = null  // asked again next time
            throw e
        })
    return g_sounds_promise
}

// plays a sound of the panel now, at the panel's volume
let g_audio_ws = null
function audio_play_now(name) {
    if (g_audio_ws !== null)
        g_audio_ws.send("AP " + name)
}

// the Audio tab: a test beep out of the panel's speaker, with its frequency and volume, and the sounds of the
// panel's /sounds folder (sounds/convert_sounds.py makes them), a button each. one plays at a time
function audio_create(parent, ws_wrap, pref_json)
{
    const top = add_div(parent, 'audio_top')
    let beeping = false
    let freq = 440
    // percent, the panel's volume pref (the remote's Volume +/- change it too, a reload shows that)
    let volume = (pref_json['volume'] !== undefined) ? pref_json['volume'] : 30
    g_audio_ws = ws_wrap

    const set_beeping = (on)=>{
        beeping = on
        btn.innerText = beeping ? 'Stop beep' : 'Start beep'
    }
    const btn = add_btn(top, 'Start beep', ()=>{
        beeping = !beeping
        if (beeping)  // the panel may have another frequency from a page opened before
            ws_wrap.send("AF " + freq)
        ws_wrap.send("AB " + (beeping ? 1 : 0))
        btn.innerText = beeping ? 'Stop beep' : 'Start beep'
    })

    // the frequency slider is logarithmic, 0..1000 from AUDIO_MIN_FREQ to AUDIO_MAX_FREQ,
    // so that each octave gets the same length
    const ratio = AUDIO_MAX_FREQ / AUDIO_MIN_FREQ
    const pos_to_freq = (p)=>Math.round(AUDIO_MIN_FREQ * Math.pow(ratio, p / 1000))
    const freq_to_pos = (f)=>Math.round(1000 * Math.log(f / AUDIO_MIN_FREQ) / Math.log(ratio))

    const fcont = add_div(top, 'audio_row')
    add_elem(fcont, 'span', 'audio_label').innerText = 'Frequency:'
    add_slider(fcont, 'audio_slider', 0, 1000, freq_to_pos(freq), (p)=>{
        freq = pos_to_freq(p)
        fval.innerText = freq + " Hz"
        ws_wrap.send("AF " + freq)
    })
    const fval = add_elem(fcont, 'span', 'audio_val')
    fval.innerText = freq + " Hz"

    const vcont = add_div(top, 'audio_row')
    add_elem(vcont, 'span', 'audio_label').innerText = 'Volume:'
    add_slider(vcont, 'audio_slider', 0, 100, volume, (v)=>{
        volume = v
        vval.innerText = volume + "%"
        ws_wrap.send("U volume " + volume)  // a pref, saved on the panel
    })
    const vval = add_elem(vcont, 'span', 'audio_val')
    vval.innerText = volume + "%"

    // the sounds: one picked in the list, the Play button
    const srow = add_div(parent, 'audio_row')
    add_elem(srow, 'span', 'audio_label').innerText = 'Sound:'
    const sel = add_select(srow, null, [{value:'', text:'loading...'}], '', ()=>{})
    sel.classList.add('audio_sound_sel')
    add_btn(srow, 'Play', ()=>{
        if (!sel.value)
            return
        ws_wrap.send("AP " + sel.value)
        set_beeping(false)  // a sound stops the beep
    })
    get_sounds().then((names)=>{
        sel.innerHTML = ''
        for (const name of names) {
            const opt = add_elem(sel, 'option', 'combo_opt')
            opt.value = name
            opt.innerText = name.replace(/\.mp3$/i, '')
        }
        if (names.length == 0)
            add_elem(sel, 'option', 'combo_opt').innerText = 'none in the file system'
    }).catch((e)=>{
        sel.innerHTML = ''
        add_elem(sel, 'option', 'combo_opt').innerText = "couldn't get the sounds"
        console.log("sounds:", e)
    })
}
