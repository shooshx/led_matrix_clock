"use strict";

const AUDIO_MIN_FREQ = 50
const AUDIO_MAX_FREQ = 5000

// the Audio tab: a test beep out of the panel's speaker, with its frequency and volume
function audio_create(parent, ws_wrap)
{
    const top = add_div(parent, 'audio_top')
    let beeping = false
    let freq = 440
    let volume = 30  // percent of full scale

    const btn = add_btn(top, 'Start beep', ()=>{
        beeping = !beeping
        if (beeping) {  // the panel may have other values from a page opened before
            ws_wrap.send("AF " + freq)
            ws_wrap.send("AV " + volume)
        }
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
        ws_wrap.send("AV " + volume)
    })
    const vval = add_elem(vcont, 'span', 'audio_val')
    vval.innerText = volume + "%"
}
