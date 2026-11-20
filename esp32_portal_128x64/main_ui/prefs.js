"use strict";

// shows the current prefs json and allows uploading a new one, that may contain only some of the prefs
// returns a function that refreshes the shown json
// pref_json is the prefs the page was made from, shown at first. the tab and Refresh download them again
function prefs_create(parent, pref_json)
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

    const show = (j)=>{
        cur_text.value = JSON.stringify(j, null, 2)
    }
    const refresh = ()=>{
        download("/pref?t=" + new Date().getTime(), (resText)=>{ show(JSON.parse(resText)) })
    }

    // all of it in one POST, the panel says which prefs it has and which it doesn't
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
        status.innerText = "Sending..."
        post_prefs(j, (answer)=>{
            let msg = "Applied " + answer.applied + " prefs."
            if (answer.unknown.length > 0)
                msg += " Skipped unknown: " + answer.unknown.join(", ") + "."
            if (answer.bad.length > 0)
                msg += " Skipped, not a number or text: " + answer.bad.join(", ") + "."
            if (answer.applied == 0 || answer.unknown.length > 0 || answer.bad.length > 0) {
                // don't reload so the skipped list can be read
                if (answer.applied > 0)
                    msg += " Reload the page to see the changes in the other tabs."
                status.innerText = msg
                refresh()
                return
            }
            // the other tabs are built from the prefs on page load so they need a reload to show the new prefs
            status.innerText = msg + " Reloading..."
            window.setTimeout(()=>{ window.location.reload() }, 500)
        }, (err)=>{
            status.innerText = "Not sent: " + err
        })
    })
    add_btn(btns, 'Refresh', refresh)
    add_btn(btns, 'Reload page', ()=>{ window.location.reload() })

    show(pref_json)  // not downloaded again, the page was just made from it
    return refresh
}
