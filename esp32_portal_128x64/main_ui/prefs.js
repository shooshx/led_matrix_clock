"use strict";

// flatten the nested pref json to {name: value}, pref names are unique across the nested objects
function flatten_prefs(j, out={})
{
    for (const [k, v] of Object.entries(j)) {
        if (v !== null && typeof v === 'object' && !Array.isArray(v))
            flatten_prefs(v, out)
        else
            out[k] = v
    }
    return out
}

// shows the current prefs json and allows uploading a new one, that may contain only some of the prefs
// when_sent(cb) calls cb once all updates were sent
// returns a function that refreshes the shown json
function prefs_create(parent, send_update, when_sent)
{
    const top = add_div(parent, 'prefs_top')
    add_div(top, 'prefs_label').innerText = 'Current prefs:'
    const cur_text = add_elem(top, 'textarea', 'prefs_text')
    cur_text.setAttribute('readonly', true)
    cur_text.setAttribute('spellcheck', false)

    add_div(top, 'prefs_label').innerText = 'New prefs (can have only some of the prefs):'
    const new_text = add_elem(top, 'textarea', 'prefs_text')
    new_text.setAttribute('spellcheck', false)

    const btns = add_div(top, 'prefs_btns')
    const status = add_div(top, 'prefs_status')

    let cur_flat = null
    const refresh = ()=>{
        download("/pref?t=" + new Date().getTime(), (resText)=>{
            const j = JSON.parse(resText)
            cur_flat = flatten_prefs(j)
            cur_text.value = JSON.stringify(j, null, 2)
        })
    }

    add_btn(btns, 'Upload', ()=>{
        let j = null
        try {
            j = JSON.parse(new_text.value)
        }
        catch(e) {
            status.innerText = "Invalid JSON: " + e.message
            return
        }
        if (j === null || typeof j !== 'object' || Array.isArray(j)) {
            status.innerText = "Expected a JSON object"
            return
        }
        let count = 0
        const unknown = [], bad_value = []
        for (const [name, v] of Object.entries(flatten_prefs(j))) {
            if (cur_flat !== null && !(name in cur_flat)) {
                unknown.push(name)
                continue
            }
            let iv = null
            if (v === true)
                iv = 1
            else if (v === false)
                iv = 0
            else if (Number.isInteger(v))
                iv = v
            if (iv === null) {
                bad_value.push(name)
                continue
            }
            send_update(name, iv)
            ++count
        }
        let msg = "Sent " + count + " prefs."
        if (unknown.length > 0)
            msg += " Skipped unknown: " + unknown.join(", ") + "."
        if (bad_value.length > 0)
            msg += " Skipped non-integer values: " + bad_value.join(", ") + "."
        if (count == 0) {
            status.innerText = msg
            return
        }
        if (unknown.length > 0 || bad_value.length > 0) {
            // don't reload so the skipped list can be read
            msg += " Reload the page to see the changes in the other tabs."
            status.innerText = msg
            window.setTimeout(refresh, 1000)
            return
        }
        // the other tabs are built from the prefs on page load so they need a reload to show the new prefs
        status.innerText = msg + " Reloading..."
        when_sent(()=>{
            // give the device some time to process the updates
            window.setTimeout(()=>{ window.location.reload() }, 500)
        })
    })
    add_btn(btns, 'Refresh', refresh)
    add_btn(btns, 'Reload page', ()=>{ window.location.reload() })

    refresh()
    return refresh
}
