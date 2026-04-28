#!/usr/bin/env python3
"""
Embed icon.png as a C byte array for Textures_GetOrCreateFromMemory.
Run from the project root:  python3 gen_icon.py
Output: src/icon_png.h
"""
import os
import sys

ROOT     = os.path.dirname(os.path.abspath(__file__))
SRC_PNG  = os.path.join(ROOT, 'icon.png')
OUT_H    = os.path.join(ROOT, 'src', 'icon_png.h')
ARR_NAME = 'icon_png'

def embed(src: str, out: str, name: str) -> None:
    data = open(src, 'rb').read()
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, 'w') as f:
        f.write('#pragma once\n\n')
        f.write(f'// Auto-generated from icon.png — do not edit by hand.\n')
        f.write(f'// Re-run gen_icon.py after changing icon.png.\n\n')
        f.write(f'static const unsigned char {name}[] = {{\n    ')
        hex_bytes = [f'0x{b:02x}' for b in data]
        for i, h in enumerate(hex_bytes):
            f.write(h)
            if i < len(hex_bytes) - 1:
                f.write(', ')
            if (i + 1) % 16 == 0:
                f.write('\n    ')
        f.write(f'\n}};\n\n')
        f.write(f'static const unsigned int {name}_len = {len(data)}u;\n')
    print(f'Wrote {len(data):,} bytes  →  {os.path.relpath(out, ROOT)}')

if __name__ == '__main__':
    if not os.path.exists(SRC_PNG):
        print(f'ERROR: icon.png not found at {SRC_PNG}')
        sys.exit(1)
    embed(SRC_PNG, OUT_H, ARR_NAME)
    print('Done. Rebuild the DLL to pick up the new icon.')
