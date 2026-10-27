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

class WSPrefHandler(HTTPWebSocketsHandler):
    def get_ex_handler(self):
        return g_pref_handler

    def on_ws_message(self, message):
        if message is None:
            return
        # echo message back to client
        #self.send_message(str(message))
        self.log_message('WS received "%s"',str(message))
        sp = message.split()
        if sp[0] == 'U':
            key = sp[1]
            value = sp[2]
            if key not in g_pref.index:
                reload_prefs()  # maybe pref.dummy got new prefs since the server started
            if key not in g_pref.index:
                self.log_message(f"WS unknown pref '{key}'")
                return
            g_pref.index[key].d = int(value)
            open(PREF_CUR, "wb").write(g_pref.dumps())
        elif sp[0] == 'D':
            pass
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

# values from cur override default, prefs that only exist in default (newly added) are kept
def merge_prefs(default, cur):
    merged = dict(default)
    for k, v in cur.items():
        if isinstance(v, dict) and isinstance(merged.get(k), dict):
            merged[k] = merge_prefs(merged[k], v)
        else:
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
    