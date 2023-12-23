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
        if handler.path.startswith("/pref"):
            jstr = g_pref.dumps()
            handler.send_response(HTTPStatus.OK)
            handler.send_header("Content-type", "application/json")
            handler.send_header("Content-Length", len(jstr))
            #self.send_header("Last-Modified", self.date_time_string(fs.st_mtime))
            handler.end_headers()
            handler.wfile.write(jstr)
            return True
        return False
                       
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

def main():
    global g_pref
    g_pref = Pref(json.load(open(PREF_CUR if os.path.exists(PREF_CUR) else PREF_DEFAULT)))
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
    