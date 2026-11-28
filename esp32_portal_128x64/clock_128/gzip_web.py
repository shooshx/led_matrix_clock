# PlatformIO pre script: makes the web UI for LittleFS from ../main_ui into .pio/web_data, which is the
# data_dir that "pio run -t uploadfs" uploads. main_ui stays as it is, the py_server dev server uses it directly.
#
# - main.html gets all the js and the css in it, so the page is one file (and the wasm): the <script src>
#   in a row are joined and minified together with terser (if it's installed, npm install in
#   esp32_portal_128x64), asm_gfx.js is put in as it is (emscripten minified it), the stylesheets as <style>
# - everything is gzipped: the web server sends file.gz for file, compressed
# - the sounds: ../sounds/convert_sounds.py makes mono mp3s of them in ../sounds/mp3, they go in as /sounds,
#   not gzipped (the firmware plays them from there)
# the files that went into main.html aren't uploaded. other files of main_ui that the page could load
# (.html .js .css .wasm) are gzipped as they are. pref.current / pref.dummy of the py_server aren't taken
import gzip
import os
import re
import shutil
import subprocess

Import("env")  # noqa: F821, PlatformIO's

EXTS = (".html", ".js", ".css", ".wasm")
PAGE = "main.html"
NOT_MINIFIED = ("asm_gfx.js",)  # made by emscripten, already minified

proj = env.subst("$PROJECT_DIR")  # noqa: F821
src_dir = os.path.normpath(os.path.join(proj, "..", "main_ui"))
out_dir = os.path.join(proj, ".pio", "web_data")
os.makedirs(out_dir, exist_ok=True)

terser = os.path.normpath(os.path.join(proj, "..", "node_modules", "terser", "bin", "terser"))
node = shutil.which("node")
can_minify = node is not None and os.path.exists(terser)
if not can_minify:
    print("web_data: terser not found, the js isn't minified (npm install in esp32_portal_128x64)")

SCRIPT_RE = re.compile(r'<script src="([^"/:]+)"></script>')
STYLE_RE = re.compile(r'<link rel="stylesheet"[^>]*href="([^"/:]+)"[^>]*>')


def read(name):
    with open(os.path.join(src_dir, name), "rb") as f:
        return f.read().decode("utf-8")


def minify(names):
    # all together, like one file. -c compress, -m mangle the local names: the top level names stay,
    # the inline scripts of main.html use them
    paths = [os.path.join(src_dir, n) for n in names]
    r = subprocess.run([node, terser] + paths + ["-c", "-m"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if r.returncode != 0:
        raise Exception("terser failed on %s: %s" % (names, r.stderr.decode(errors="replace")))
    return r.stdout.decode("utf-8")


def inline_js(code):
    # "</script" in the code would end the <script> element
    return "<script>\n" + code.replace("</script", "<\\/script") + "\n</script>"


# main.html with all its scripts and styles in it. returns it and the names of the files put in it
def make_page():
    html = read(PAGE)
    used = set()

    # the <script src> one after the other (only spaces between) are minified together
    out = []
    pos = 0
    matches = list(SCRIPT_RE.finditer(html))
    i = 0
    while i < len(matches):
        m = matches[i]
        out.append(html[pos:m.start()])
        group = [m]
        while (i + len(group) < len(matches) and matches[i + len(group)].group(1) not in NOT_MINIFIED
               and m.group(1) not in NOT_MINIFIED
               and html[group[-1].end():matches[i + len(group)].start()].strip() == ""):
            group.append(matches[i + len(group)])
        names = [g.group(1) for g in group]
        used.update(names)
        if can_minify and names[0] not in NOT_MINIFIED:
            code = minify(names)
        else:
            code = "\n".join(read(n) for n in names)
        out.append(inline_js(code))
        pos = group[-1].end()
        i += len(group)
    out.append(html[pos:])
    html = "".join(out)

    def style(m):
        used.add(m.group(1))
        return "<style>\n" + read(m.group(1)) + "\n</style>"
    html = STYLE_RE.sub(style, html)
    return html.encode("utf-8"), used


def write_gz(out_name, data, src_size):
    # mtime=0: the same file gives the same .gz. the server's ETag is the CRC of the content anyway
    with open(os.path.join(out_dir, out_name), "wb") as f:
        f.write(gzip.compress(data, compresslevel=9, mtime=0))
    print("web_data: %s %d -> %d bytes" % (out_name, src_size, os.path.getsize(os.path.join(out_dir, out_name))))


def mtime(name):
    return os.path.getmtime(os.path.join(src_dir, name))


# made again when a source, this script, or whether it minifies changed
mode_file = out_dir + ".mode"  # next to it, not uploaded
mode = "inline-minified" if can_minify else "inline-plain"
old_mode = open(mode_file).read() if os.path.exists(mode_file) else ""
script_time = os.path.getmtime(os.path.join(proj, "gzip_web.py"))


def up_to_date(out_name, sources):
    out = os.path.join(out_dir, out_name)
    if old_mode != mode or not os.path.exists(out):
        return False
    t = os.path.getmtime(out)
    return t >= script_time and all(t >= mtime(s) for s in sources)


sources = [n for n in sorted(os.listdir(src_dir)) if os.path.isfile(os.path.join(src_dir, n)) and n.endswith(EXTS)]
inlined = set(m.group(1) for m in SCRIPT_RE.finditer(read(PAGE))) | set(m.group(1) for m in STYLE_RE.finditer(read(PAGE)))
wanted = set()

page_gz = PAGE + ".gz"
wanted.add(page_gz)
if not up_to_date(page_gz, [PAGE] + sorted(inlined)):
    data, _ = make_page()
    write_gz(page_gz, data, sum(os.path.getsize(os.path.join(src_dir, n)) for n in [PAGE] + sorted(inlined)))

for name in sources:
    if name == PAGE or name in inlined:
        continue
    out_name = name + ".gz"
    wanted.add(out_name)
    if up_to_date(out_name, [name]):
        continue
    with open(os.path.join(src_dir, name), "rb") as f:
        data = f.read()
    write_gz(out_name, data, len(data))

with open(mode_file, "w") as f:
    f.write(mode)

# the sounds, in their own folder
SOUNDS_DIR = "sounds"
wanted.add(SOUNDS_DIR)
sounds_src = os.path.normpath(os.path.join(proj, "..", "sounds"))
sounds_out = os.path.join(out_dir, SOUNDS_DIR)
os.makedirs(sounds_out, exist_ok=True)
mp3_dir = os.path.join(sounds_src, "mp3")
if os.path.exists(os.path.join(sounds_src, "convert_sounds.py")):
    import importlib.util
    spec = importlib.util.spec_from_file_location("convert_sounds", os.path.join(sounds_src, "convert_sounds.py"))
    convert_sounds = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(convert_sounds)
    try:
        mp3_dir = convert_sounds.convert()
    except Exception as e:
        print("sounds: not converted, %s. the mp3s there already are used" % e)
mp3s = sorted(f for f in os.listdir(mp3_dir) if f.lower().endswith(".mp3")) if os.path.isdir(mp3_dir) else []
for name in mp3s:
    src = os.path.join(mp3_dir, name)
    dst = os.path.join(sounds_out, name)
    if not os.path.exists(dst) or os.path.getmtime(dst) < os.path.getmtime(src):
        shutil.copy2(src, dst)
for name in os.listdir(sounds_out):
    if name not in mp3s:
        os.remove(os.path.join(sounds_out, name))
        print("web_data: removed sounds/" + name)

# files that aren't needed any more
for name in os.listdir(out_dir):
    if name not in wanted:
        path = os.path.join(out_dir, name)
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)
        print("web_data: removed " + name)
