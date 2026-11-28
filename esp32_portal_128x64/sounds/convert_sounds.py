"""
Converts the sounds in this folder (.wav, and anything else ffmpeg reads) to mono mp3s in mp3/, for the panel.
gzip_web.py (the PlatformIO pre script of clock_128) runs it and puts mp3/ in the file system as /sounds, so
"pio run -t buildfs / uploadfs" has them. it can also be run by itself:

    python convert_sounds.py [--rate 22050] [--bitrate 48]

- only what changed is converted again, the mp3s of sounds that were deleted are deleted
- mono: the amplifier has one speaker. 22 kHz and 48 kbps sound the same as more on a small speaker
- the names are cut to MAX_NAME characters (LittleFS's limit, mklittlefs fails on longer ones), keeping a number
  at their end: chime_tinkle_wood_bell_positive_07 -> chime_tinkle_wood_bell_p_07
needs ffmpeg with libmp3lame in the PATH
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = os.path.join(HERE, "mp3")
SOURCE_EXTS = (".wav", ".flac", ".ogg", ".aiff", ".aif", ".m4a", ".mp3")
MAX_NAME = 31  # with ".mp3"
DEFAULT_RATE = 22050
DEFAULT_BITRATE = 48


def out_names(sources):
    # source file name -> mp3 name, cut short, unique
    names = {}
    used = set()
    for src in sources:
        stem = os.path.splitext(src)[0].replace(" ", "_")
        base = stem
        if len(base) > MAX_NAME - 4:
            m = re.search(r"_\d+$", stem)
            tail = m.group(0) if m else ""
            base = stem[:MAX_NAME - 4 - len(tail)].rstrip("_") + tail
        name = base + ".mp3"
        n = 2
        while name.lower() in used:
            suffix = "_%d" % n
            name = base[:MAX_NAME - 4 - len(suffix)] + suffix + ".mp3"
            n += 1
        used.add(name.lower())
        names[src] = name
    return names


def convert(rate=DEFAULT_RATE, bitrate=DEFAULT_BITRATE, verbose=True):
    """returns the mp3 folder. raises if ffmpeg is missing and something has to be converted"""
    os.makedirs(OUT_DIR, exist_ok=True)
    sources = sorted(f for f in os.listdir(HERE)
                     if os.path.isfile(os.path.join(HERE, f)) and f.lower().endswith(SOURCE_EXTS))
    names = out_names(sources)
    script_time = os.path.getmtime(os.path.abspath(__file__))
    ffmpeg = shutil.which("ffmpeg")
    for src, name in names.items():
        src_path = os.path.join(HERE, src)
        out_path = os.path.join(OUT_DIR, name)
        if os.path.exists(out_path) and os.path.getmtime(out_path) >= max(os.path.getmtime(src_path), script_time):
            continue
        if ffmpeg is None:
            raise Exception("ffmpeg isn't in the PATH, can't convert " + src)
        cmd = [ffmpeg, "-hide_banner", "-loglevel", "error", "-y", "-i", src_path,
               "-ac", "1", "-ar", str(rate), "-codec:a", "libmp3lame", "-b:a", "%dk" % bitrate,
               # no tags and no Xing header frame, only audio frames
               "-map_metadata", "-1", "-id3v2_version", "0", "-write_id3v1", "0", "-write_xing", "0",
               out_path]
        r = subprocess.run(cmd)
        if r.returncode != 0:
            raise Exception("ffmpeg failed on " + src)
        if verbose:
            print("sounds: %s %d -> %s %d bytes" % (src, os.path.getsize(src_path), name, os.path.getsize(out_path)))
    # the mp3s of sounds that aren't here any more
    wanted = set(names.values())
    for f in os.listdir(OUT_DIR):
        if f not in wanted:
            os.remove(os.path.join(OUT_DIR, f))
            if verbose:
                print("sounds: removed " + f)
    return OUT_DIR


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="converts the sounds to mono mp3s in mp3/")
    ap.add_argument("--rate", type=int, default=DEFAULT_RATE, help="sample rate, Hz")
    ap.add_argument("--bitrate", type=int, default=DEFAULT_BITRATE, help="kbps")
    args = ap.parse_args()
    try:
        convert(args.rate, args.bitrate)
    except Exception as e:
        print(e)
        sys.exit(1)
    total = sum(os.path.getsize(os.path.join(OUT_DIR, f)) for f in os.listdir(OUT_DIR))
    print("sounds: %d files, %d KB" % (len(os.listdir(OUT_DIR)), total // 1024))
