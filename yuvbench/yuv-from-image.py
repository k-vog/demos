#!/usr/bin/env python3

import json
import struct
import subprocess
import sys

def main():
  if len(sys.argv) != 3:
    print(f'Usage: {sys.argv[0]} <input> <output.kyuv>')
    exit(1)
  probe = json.loads(subprocess.check_output([
    'ffprobe',
      '-v', 'error',
      '-show_entries', 'stream=width,height',
      '-of', 'json',
      sys.argv[1]
  ]))['streams'][0]
  subprocess.run([
    'ffmpeg',
      '-i', sys.argv[1],
      '-pix_fmt', 'yuv420p',
      sys.argv[2]
  ], check=True)
  yuv = None
  with open(sys.argv[2], 'rb') as f:
    yuv = f.read()
  with open(sys.argv[2], 'wb') as f:
    f.write(b'kyuv')
    f.write(struct.pack('<II', probe['width'], probe['height']))
    f.write(yuv)

if __name__ == '__main__':
  main()
