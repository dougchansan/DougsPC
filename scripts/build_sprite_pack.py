"""Build fixed-size offline RGBA packs from a pinned PokéSprite checkout.

No HTTP access at runtime. Base species icons only; shiny appearance is a
request preview, never a legality or availability claim.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

SOURCE_REVISION = 'c5aaa610ff2acdf7fd8e2dccd181bca8be9fcb3e'
HEADER = struct.pack('<8sHHI', b'LBXS1\0\0\0', 40, 30, 807)

def build(source: Path, destination: Path):
    from PIL import Image
    database = json.loads((source / 'data/pokemon.json').read_text())
    packs = {}
    for variant in ('regular', 'shiny'):
        data = bytearray(HEADER)
        for dex in range(1, 808):
            slug = database[f'{dex:03}']['slug']['eng']
            path = source / 'icons/pokemon' / variant / f'{slug}.png'
            with Image.open(path) as image:
                image = image.convert('RGBA')
                if image.size != (40, 30):
                    raise ValueError(f'Unexpected dimensions: {path}: {image.size}')
                data.extend(image.tobytes())
        packs[variant] = bytes(data)
    # Nothing written until all 1614 source images have been checked.
    destination.mkdir(parents=True, exist_ok=False)
    hashes = {}
    for variant, data in packs.items():
        (destination / f'{variant}.bin').write_bytes(data)
        hashes[variant] = hashlib.sha256(data).hexdigest()
    notice = ('PokéSprite by Mike Sikma and contributors\n'
              'https://github.com/msikma/pokesprite\n'
              f'Source revision: {SOURCE_REVISION}\n'
              'Pokemon artwork belongs to its respective rights holders.\n'
              'Shiny icons include custom artwork; display is not eligibility evidence.\n')
    (destination / 'NOTICE.txt').write_text(notice)
    for name in ('LICENSE', 'LICENSE.txt', 'LICENSE.md', 'license'):
        if (source / name).is_file():
            (destination / 'POKESPRITE-LICENSE.txt').write_bytes((source / name).read_bytes())
            break
    (destination / 'sprite-pack.json').write_text(json.dumps({
        'schema': 'LBXS1', 'source_revision': SOURCE_REVISION,
        'count': 807, 'width': 40, 'height': 30, 'sha256': hashes,
    }, indent=2) + '\n')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    build(args.source, args.destination)
