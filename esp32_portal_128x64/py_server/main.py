import sys, os
import threading
from socketserver import ThreadingMixIn
from http.server import HTTPServer
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler
import ssl
from base64 import b64encode
from HTTPWebSocketsHandler import HTTPWebSocketsHandler
import json

if len(sys.argv) > 1:
    port = int(sys.argv[1])
else:
    port = 8000
if len(sys.argv) > 2:
    secure = str(sys.argv[2]).lower()=="secure"
else:
    secure = False
if len(sys.argv) > 3:
    credentials = str(sys.argv[3])
else:
    credentials = ""

this_dir = os.path.dirname(os.path.abspath(__file__))

class Prop:
    def __init__(self, d):
        self.d = d

class Pref:
    def __init__(self, j):
        self.index = {}
        self.d = self.enum_fields(j)

    def enum_fields(self, j):
        curd = {}
        for k, v in j.items():
            if isinstance(v, dict):
                curd[k] = self.enum_fields(v)
            else:
                p = Prop(v)
                curd[k] = p
                assert k not in self.index, f"duplicate key in prop index {k}"
                self.index[k] = p
        return curd
    
    def dumps(self):
        return json.dumps(self, default=json_default).encode('utf-8')

def json_default(obj):
    if isinstance(obj, Prop) or isinstance(obj, Pref):
        return obj.d
    return obj

g_pref = None

class JsonPrefHandler:
    def do_GET(self, handler):
        # exact match, not a prefix, so that files like /prefs.js are still served
        if handler.path.split("?")[0] == "/pref":
            jstr = g_pref.dumps()
            handler.send_response(HTTPStatus.OK)
            handler.send_header("Content-type", "application/json")
            handler.send_header("Content-Length", len(jstr))
            #self.send_header("Last-Modified", self.date_time_string(fs.st_mtime))
            handler.end_headers()
            handler.wfile.write(jstr)
            return True
        # the draw tab's Update: there's no panel, an empty drawing in the size of the current orientation
        path = handler.path.split("?")[0]
        if path == "/drawing":
            rot = g_pref.index["rotation"].d if "rotation" in g_pref.index else 0
            w, h = (64, 128) if rot & 1 else (128, 64)
            self.send_bytes(handler, bytes([w & 0xFF, w >> 8, h & 0xFF, h >> 8]) + bytes(w * h * 3), "application/octet-stream")
            return True
        if path == "/drawing_ver":
            self.send_bytes(handler, b"0", "text/plain")
            return True
        return False

    def send_bytes(self, handler, data, content_type):
        handler.send_response(HTTPStatus.OK)
        handler.send_header("Content-type", content_type)
        handler.send_header("Content-Length", len(data))
        handler.end_headers()
        handler.wfile.write(data)
                       
g_pref_handler = JsonPrefHandler()

PREF_CUR = os.path.join(this_dir, "..", "main_ui", "pref.current")
PREF_DEFAULT = os.path.join(this_dir, "..", "main_ui", "pref.dummy")

# the json of GET /pref has these too, they aren't prefs
NOT_PREFS = ("rotation", "color_bits_actual")

# sets a pref, value is an int or a text. returns False if there's no such pref. pref.current isn't written here
def set_pref(key, value):
    if key not in g_pref.index:
        reload_prefs()  # maybe pref.dummy got new prefs since the server started
    if key not in g_pref.index and keep_unknown(key):
        # a clock face slot or period that wasn't used before (pref.dummy has the ones the panel had),
        # added to the clock section, they're in pref.current from now on
        p = Prop(value)
        g_pref.d["clock"][key] = p
        g_pref.index[key] = p
    if key not in g_pref.index:
        return False
    g_pref.index[key].d = value
    if key == 'color_bits' and 'color_bits_actual' in g_pref.index:
        g_pref.index['color_bits_actual'].d = value  # no panel here, it always works
    return True

def save_prefs():
    open(PREF_CUR, "wb").write(g_pref.dumps())

class WSPrefHandler(HTTPWebSocketsHandler):
    def get_ex_handler(self):
        return g_pref_handler

    # many prefs at once, like the panel's POST /pref: a json like GET /pref, nested or not, any part of it.
    # answers {"applied": n, "unknown": [names], "bad": [names]}
    def do_POST(self):
        if self.path.split("?")[0] != "/pref":
            self.send_error(HTTPStatus.NOT_FOUND)
            return
        n = int(self.headers.get("Content-Length", 0))
        try:
            j = json.loads(self.rfile.read(n))
        except ValueError as e:
            self.send_error(HTTPStatus.BAD_REQUEST, "bad json: " + str(e))
            return
        answer = {"applied": 0, "unknown": [], "bad": []}
        def apply(obj):
            for k, v in obj.items():
                if isinstance(v, dict):
                    apply(v)
                elif k in NOT_PREFS:
                    continue
                elif isinstance(v, bool) or isinstance(v, int) or isinstance(v, str):
                    value = int(v) if isinstance(v, bool) else v
                    if set_pref(k, value):
                        answer["applied"] += 1
                    else:
                        answer["unknown"].append(k)
                else:
                    answer["bad"].append(k)
        if isinstance(j, dict):
            apply(j)
        save_prefs()
        self.log_message("POST /pref: %d applied, %d unknown, %d bad", answer["applied"], len(answer["unknown"]), len(answer["bad"]))
        g_pref_handler.send_bytes(self, json.dumps(answer).encode('utf-8'), "application/json")

    def on_ws_message(self, message):
        if message is None:
            return
        # the pref updates come together, one per line (WsWrap in main.html)
        for line in str(message).split('\n'):
            if line.strip():
                self.on_ws_line(line)

    def on_ws_line(self, message):
        self.log_message('WS received "%s"',str(message))
        sp = message.split()
        if sp[0] == 'UT':
            # a text pref, the text is to the end of the line (the names of the clock faces)
            parts = message.split(' ', 2)
            if not set_pref(parts[1], parts[2] if len(parts) > 2 else ""):
                self.log_message(f"WS unknown pref '{parts[1]}'")
                return
            save_prefs()
        elif sp[0] == 'U':
            if not set_pref(sp[1], int(sp[2])):
                self.log_message(f"WS unknown pref '{sp[1]}'")
                return
            save_prefs()
        elif sp[0] in ('D', 'S'):
            pass  # drawing, the simulated time: there's no panel here
        else:
            self.log_message(f"WS unknown command '{sp[0]}' in {message}")

    def on_ws_connected(self):
        self.log_message('%s','websocket connected')

    def on_ws_closed(self):
        self.log_message('%s','websocket closed')

class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    """Handle requests in a separate thread."""
    pass

# go to main_ui and run python ..\py_server\main.py 80

# values from cur override default, prefs that only exist in default (newly added) are kept.
# prefs that are only in cur are dropped: pref.dummy has the prefs the panel has, the others were removed from
# it (like the clock prefs from before the clock faces). except the clock face and period slots, pref.dummy
# doesn't have all of them, they're added when the web-ui uses a new slot
def keep_unknown(k):
    return k.startswith(("lf", "per"))

def merge_prefs(default, cur):
    merged = dict(default)
    for k, v in cur.items():
        if isinstance(v, dict) and isinstance(merged.get(k), dict):
            merged[k] = merge_prefs(merged[k], v)
        elif k in merged or keep_unknown(k):
            merged[k] = v
    return merged

# pref.dummy with the saved values over it, the saved ones are from memory once the server runs
def reload_prefs():
    global g_pref
    prefs = json.load(open(PREF_DEFAULT))
    if g_pref is not None:
        cur = json.loads(g_pref.dumps())
    elif os.path.exists(PREF_CUR):
        cur = json.load(open(PREF_CUR))
    else:
        cur = {}
    g_pref = Pref(merge_prefs(prefs, cur))

def main():
    reload_prefs()
    try:
        #Replace WSSimpleEcho with your own subclass of HTTPWebSocketHandler
        server = ThreadedHTTPServer(('', port), WSPrefHandler)
        server.daemon_threads = True
        server.auth = b64encode(credentials.encode('utf-8'))
        if secure:
            server.socket = ssl.wrap_socket(server.socket, keyfile='./key.pem', certfile='./server.pem', server_side=True)
            print('started secure https server at port %d' % (port,))
        else: 
            print('started http server at port %d' % (port,))
        server.serve_forever()
    except KeyboardInterrupt:
        print('^C received, shutting down server')
        server.socket.close()

if __name__ == '__main__':
    main()
    