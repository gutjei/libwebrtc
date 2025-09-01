#!/usr/bin/env python3

import argparse
import os
import urllib.request
import zipfile
import shutil
import tempfile

def main():
    out = "src/codecs/ffmpeg/third/ffmpeg"
    os.makedirs(out, exist_ok=True)

    tmp_dir = tempfile.mkdtemp(prefix="ffmpeg_")
    tmp_zip = os.path.join(out, "ffmpeg.zip")
    url = "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-n7.1-latest-win64-gpl-shared-7.1.zip"

    print(f"[+] downloading {url} → {tmp_zip}")
    urllib.request.urlretrieve(url, tmp_zip)

    print(f"[+] unpacking {out}")
    with zipfile.ZipFile(tmp_zip, "r") as zf:
        zf.extractall(tmp_dir)

    entries = [e for e in os.listdir(tmp_dir) if e != "ffmpeg.zip"]
    dirs = [e for e in entries if os.path.isdir(os.path.join(tmp_dir, e))]
    files = [e for e in entries if os.path.isfile(os.path.join(tmp_dir, e))]
    src_root = os.path.join(tmp_dir, dirs[0]) if (len(dirs) == 1 and not files) else tmp_dir

    for name in os.listdir(src_root):
       src = os.path.join(src_root, name)
       dst = os.path.join(out, name)
       if os.path.isdir(dst) and not os.path.islink(dst):
           shutil.rmtree(dst)
       elif os.path.exists(dst):
           os.remove(dst)
       shutil.move(src, dst)


    shutil.rmtree(tmp_dir, ignore_errors=True)
    os.remove(tmp_zip)

if __name__ == "__main__":
    main()