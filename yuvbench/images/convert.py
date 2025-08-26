#!/usr/bin/env python3

import json
import struct
import subprocess
import sys
import os

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
      '-y',
      '-i', sys.argv[1],
      '-pix_fmt', 'yuv420p',
      '-color_range', 'full',
      'tmp.yuv'
  ], check=True)
  with open('tmp.yuv', 'rb') as f:
    with open(sys.argv[2], 'wb') as f2:
      f2.write(b'kYUV')
      f2.write(struct.pack('<II', probe['width'], probe['height']))
      f2.write(f.read())
  os.remove('tmp.yuv')

if __name__ == '__main__':
  main()
