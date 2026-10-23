"""Stamp "STARGATETC REDIAL <version> ::" onto the StargateTC menu/loading background.

StargateTC has no `hd_background "1"` in liblist.gam, so HL25's main menu (GameUI) uses
resource/BackgroundLayout.txt and the loading screen (hw.dll) uses
resource/BackgroundLoadingLayout.txt. Both come from valve/ and place the same twelve
800x600 tiles: stargatetc/resource/background/800_<row>_<col>_loading.tga.

This script:
  - reads the stock tiles (it prefers *.sgtcbots.bak, the installer's backup of the original,
    and checks every tile it changes against the SHA-256 of the stock StargateTC 2.0 tile);
  - draws the text with a small pixel font made to look like the art's own lettering
    (":: WORMHOLE ENGAGED", "STARGATE TC 2.0 FINAL ::"): white, soft blue glow, scanlines;
  - writes only the tiles it changed into <out>/resource/background/, keeping the original
    TGA header and footer byte for byte (uncompressed 24-bit, bottom-left origin) and
    replacing only the pixel data;
  - writes a PNG preview of the whole 800x600 screen.

usage:
  python stamp_splash.py --game <...\\Half-Life\\stargatetc> --out <payload dir>
                         [--version-file VERSION] [--place corner|title]
                         [--text "StargateTC Redial {version}"] [--keep-case]
                         [--preview preview.png] [--force]

Needs Pillow (py -m pip install --user pillow).
"""
import argparse
import hashlib
import os
import struct
import sys

from PIL import Image, ImageFilter

TILE_W = {"a": 256, "b": 256, "c": 256, "d": 32}
TILE_X = {"a": 0, "b": 256, "c": 512, "d": 768}
TILE_H = {1: 256, 2: 256, 3: 88}
TILE_Y = {1: 0, 2: 256, 3: 512}

# SHA-256 of the stock StargateTC 2.0 tiles ("Absolute final" patch on top of the 1.0 installer).
STOCK_SHA256 = {
    "800_1_a_loading.tga": "89e2f04bb3cc2069c0f1d47c7c0e87dbff1b8ca74b4ebb36a44d3b71a446c5b9",
    "800_1_b_loading.tga": "9f833253ae8ecc630f0e6c4bc67184a70b12488cb31c3c96ce31bc2283c99e29",
    "800_1_c_loading.tga": "f120173965b7686ea249a1d6f7a0237d3ae50a2894465a372d582c0d67a1384b",
    "800_1_d_loading.tga": "873b7db88081762631cecd502a6b47ce9be4296728f227394f4d47e27f5e543a",
    "800_2_a_loading.tga": "93abc7bf82d9c78c00dcc152bb6296a7141ce7b517b5e6be556dfc60c62e5826",
    "800_2_b_loading.tga": "1e4b5ff48b0c366d030a124c60a8b525ba48a3e075d3e457bd38002a383998f4",
    "800_2_c_loading.tga": "e8f81069d72754bab4aecf2fb45b89fdcfc8efbd793392a2a52ce4391cf10081",
    "800_2_d_loading.tga": "556e490a38485ca2c23ef2c60ffdde054dd4c55ddf383c807fb87b4b6c2e5fcf",
    "800_3_a_loading.tga": "1c068ec6ef0812052f84649d1e215db7c30c40e915df848445f67d076013bc4b",
    "800_3_b_loading.tga": "af77015623ef2f6a6d7955f3757b2253e1b2be3d0837c7636692ddaae69d2b08",
    "800_3_c_loading.tga": "8e5c0172adf1e26b825b532f0c63bcf890fd4a30ea1be29f4e746ea009abbfee",
    "800_3_d_loading.tga": "8dec7aae6eabbbf050163034677e5aa876d7b59ac2cf64a22d9274c7d1ca4776",
}

# Squared pixel font, 5 rows. Each "#" is a CELL x CELL block, so with CELL = 2 the bars are
# 2 px thick and capitals are 10 px tall, like the art's lettering (about 9 px).
GLYPHS = {
    "A": ["####", "#..#", "####", "#..#", "#..#"],
    "B": ["###.", "#..#", "####", "#..#", "####"],
    "C": ["####", "#...", "#...", "#...", "####"],
    "D": ["###.", "#..#", "#..#", "#..#", "###."],
    "E": ["####", "#...", "###.", "#...", "####"],
    "F": ["####", "#...", "###.", "#...", "#..."],
    "G": ["####", "#...", "#.##", "#..#", "####"],
    "H": ["#..#", "#..#", "####", "#..#", "#..#"],
    "I": ["#", "#", "#", "#", "#"],
    "J": ["...#", "...#", "...#", "#..#", "####"],
    "K": ["#..#", "#.#.", "##..", "#.#.", "#..#"],
    "L": ["#...", "#...", "#...", "#...", "####"],
    "M": ["#...#", "##.##", "#.#.#", "#...#", "#...#"],
    "N": ["#..#", "##.#", "#.##", "#..#", "#..#"],
    "O": ["####", "#..#", "#..#", "#..#", "####"],
    "P": ["####", "#..#", "####", "#...", "#..."],
    "Q": ["####", "#..#", "#..#", "#.##", "####"],
    "R": ["####", "#..#", "###.", "#..#", "#..#"],
    "S": ["####", "#...", "####", "...#", "####"],
    "T": ["###", ".#.", ".#.", ".#.", ".#."],
    "U": ["#..#", "#..#", "#..#", "#..#", "####"],
    "V": ["#...#", "#...#", "#...#", ".#.#.", "..#.."],
    "W": ["#...#", "#...#", "#.#.#", "##.##", "#...#"],
    "X": ["#..#", "#..#", ".##.", "#..#", "#..#"],
    "Y": ["#..#", "#..#", "####", "..#.", "..#."],
    "Z": ["####", "...#", ".##.", "#...", "####"],
    "0": ["####", "#..#", "#..#", "#..#", "####"],
    "1": ["##.", ".#.", ".#.", ".#.", "###"],
    "2": ["####", "...#", "####", "#...", "####"],
    "3": ["####", "...#", ".###", "...#", "####"],
    "4": ["#..#", "#..#", "####", "...#", "...#"],
    "5": ["####", "#...", "####", "...#", "####"],
    "6": ["####", "#...", "####", "#..#", "####"],
    "7": ["####", "...#", "...#", "...#", "...#"],
    "8": ["####", "#..#", "####", "#..#", "####"],
    "9": ["####", "#..#", "####", "...#", "####"],
    ".": [".", ".", ".", ".", "#"],
    ":": [".", "#", ".", "#", "."],
    "-": ["...", "...", "###", "...", "..."],
    "+": ["...", ".#.", "###", ".#.", "..."],
    " ": ["..", "..", "..", "..", ".."],
}
CELL = 2

# Where the text goes: right-aligned at x = right, glyph top at y = top (800x600 coordinates).
PLACES = {
    # bottom right; with the glow it stays inside 800_3_c (x 512-767), so one tile changes
    "corner": {"right": 760, "top": 566},
    # just under "STARGATE TC 2.0 FINAL ::" (which ends at x = 748); changes 800_2_b and 800_2_c
    "title": {"right": 748, "top": 316},
}


def read_tga(path):
    """Return (header+id+colormap bytes, pixel bytes, footer bytes, width, height, bottom_up)."""
    data = open(path, "rb").read()
    idlen, cmtype, itype, _cmo, cmlen, cmdepth, _xo, _yo, w, h, bpp, desc = struct.unpack_from("<BBBHHBHHHHBB", data, 0)
    if itype != 2 or cmtype != 0 or bpp != 24:
        sys.exit(f"{path}: expected an uncompressed 24-bit TGA (type 2), got type {itype}, {bpp} bpp")
    start = 18 + idlen + cmlen * ((cmdepth + 7) // 8)
    end = start + w * h * 3
    return data[:start], data[start:end], data[end:], w, h, not (desc & 0x20)


def tga_to_image(pixels, w, h, bottom_up):
    im = Image.frombytes("RGB", (w, h), pixels, "raw", "BGR", 0, -1 if bottom_up else 1)
    return im


def image_to_tga_pixels(im, bottom_up):
    return im.tobytes("raw", "BGR", 0, -1 if bottom_up else 1)


def text_mask(text, cell=CELL):
    """1-bit mask of the text drawn with GLYPHS, 1 cell between glyphs, as an L image."""
    cols = []
    for i, ch in enumerate(text):
        g = GLYPHS.get(ch)
        if g is None:
            sys.exit(f"no glyph for {ch!r}; add it to GLYPHS")
        for x in range(len(g[0])):
            cols.append([row[x] == "#" for row in g])
        if i != len(text) - 1:
            cols.append([False] * 5)
    w, h = len(cols) * cell, 5 * cell
    mask = Image.new("L", (w, h), 0)
    px = mask.load()
    for cx, col in enumerate(cols):
        for cy, on in enumerate(col):
            if on:
                for dx in range(cell):
                    for dy in range(cell):
                        px[cx * cell + dx, cy * cell + dy] = 255
    return mask


def scanline_phase(screen, box):
    """The row phase (y mod 4) of the art's dark scanlines inside box."""
    x0, y0, x1, y1 = box
    lum = screen.convert("L").load()
    sums = [0.0] * 4
    counts = [0] * 4
    for y in range(y0, y1):
        for x in range(x0, x1):
            sums[y % 4] += lum[x, y]
            counts[y % 4] += 1
    means = [s / c if c else 999 for s, c in zip(sums, counts)]
    return means.index(min(means))


def stamp(screen, text, place):
    mask = text_mask(text)
    left, top = place["right"] - mask.width, place["top"]
    pad = 6
    box = (left - pad, top - pad, place["right"] + pad, top + mask.height + pad)
    if box[0] < 0 or box[2] > 800 or box[1] < 0 or box[3] > 600:
        sys.exit(f"text does not fit at {place}: box {box}")
    big = Image.new("L", (box[2] - box[0], box[3] - box[1]), 0)
    big.paste(mask, (pad, pad))

    # soft blue glow, like the art's lettering
    glow = big.filter(ImageFilter.GaussianBlur(2.4)).point(lambda v: min(255, int(v * 1.5)))
    # slightly soft core (the art's letters are not razor sharp) ...
    core = big.filter(ImageFilter.GaussianBlur(0.45))
    # ... dimmed on the art's dark scanline rows
    phase = scanline_phase(screen, box)
    cpx = core.load()
    for y in range(core.height):
        if (box[1] + y) % 4 == phase:
            for x in range(core.width):
                cpx[x, y] = int(cpx[x, y] * 0.78)

    region = screen.crop(box)
    region.paste((150, 178, 232), (0, 0), glow.point(lambda v: int(v * 0.85)))
    region.paste((255, 255, 255), (0, 0), core)
    screen.paste(region, box[:2])
    return box


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--game", required=True, help="the stargatetc folder (read only)")
    ap.add_argument("--out", required=True, help="payload folder; tiles go to <out>/resource/background/")
    ap.add_argument("--version-file", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "VERSION"))
    ap.add_argument("--text", default="StargateTC Redial {version}")
    ap.add_argument("--keep-case", action="store_true", help="don't upper-case the text (the art is all capitals)")
    ap.add_argument("--no-dots", action="store_true", help="don't end the text with ' ::' like the art's labels")
    ap.add_argument("--place", choices=sorted(PLACES), default="corner")
    ap.add_argument("--preview", help="PNG preview of the whole 800x600 screen")
    ap.add_argument("--force", action="store_true", help="accept tiles that are not the stock 2.0 ones")
    args = ap.parse_args()

    version = open(args.version_file, encoding="ascii").read().strip()
    text = args.text.format(version=version)
    if not args.keep_case:
        text = text.upper()
    if not args.no_dots:
        text += " ::"

    src_dir = os.path.join(args.game, "resource", "background")
    screen = Image.new("RGB", (800, 600))
    tiles = {}
    for row in (1, 2, 3):
        for col in "abcd":
            name = f"800_{row}_{col}_loading.tga"
            path = os.path.join(src_dir, name)
            if os.path.exists(path + ".sgtcbots.bak"):
                path += ".sgtcbots.bak"   # the original, saved by install.ps1
            head, pixels, foot, w, h, bottom_up = read_tga(path)
            if (w, h) != (TILE_W[col], TILE_H[row]):
                sys.exit(f"{path}: unexpected size {w}x{h}")
            sha = hashlib.sha256(head + pixels + foot).hexdigest()
            tiles[name] = (head, foot, bottom_up, sha, path, (TILE_X[col], TILE_Y[row], w, h))
            screen.paste(tga_to_image(pixels, w, h, bottom_up), (TILE_X[col], TILE_Y[row]))

    before = screen.copy()
    box = stamp(screen, text, PLACES[args.place])
    print(f"text: {text!r}  box: {box}")

    out_dir = os.path.join(args.out, "resource", "background")
    os.makedirs(out_dir, exist_ok=True)
    for name, (head, foot, bottom_up, sha, path, (x, y, w, h)) in sorted(tiles.items()):
        old, new = before.crop((x, y, x + w, y + h)), screen.crop((x, y, x + w, y + h))
        if old.tobytes() == new.tobytes():
            continue
        if sha != STOCK_SHA256[name] and not args.force:
            sys.exit(f"{path} is not the stock StargateTC 2.0 tile (sha256 {sha}); use --force to stamp it anyway")
        data = head + image_to_tga_pixels(new, bottom_up) + foot
        out = os.path.join(out_dir, name)
        open(out, "wb").write(data)
        # check: same size and header, and it reads back as the stamped picture
        h2, p2, f2, w2, hh2, bu2 = read_tga(out)
        assert (h2, f2, w2, hh2, bu2) == (head, foot, w, h, bottom_up) and len(data) == os.path.getsize(path)
        assert tga_to_image(p2, w2, hh2, bu2).tobytes() == new.tobytes()
        print(f"wrote {out} ({len(data)} bytes, sha256 {hashlib.sha256(data).hexdigest()})")

    if args.preview:
        screen.save(args.preview)
        print(f"preview: {args.preview}")


if __name__ == "__main__":
    main()
