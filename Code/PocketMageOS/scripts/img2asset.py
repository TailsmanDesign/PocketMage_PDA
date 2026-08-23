#!/usr/bin/env python3
"""Convert images into PROGMEM C arrays for src/assets.cpp.

    PNG/JPG/... --> [resize] --> 1-bit --> BMP --> BIN --> C array
    BMP ---------/
    BIN --------/  (raw GxEPD2 payload; requires --width and --height)

The BIN payload matches GxEPD2 drawBitmap expectations: rows top-to-bottom,
each row padded to a byte boundary, bits MSB-first (1 = black).

Usage:
  python3 scripts/img2asset.py INPUT NAME [--width W | --scale F]
                                [--threshold N] [--emit bmp,bin,array]
                                [--out-dir DIR]

Emits selected artifacts: NAME.bmp / NAME.bin written to --out-dir, the C
array printed to stdout ready to paste into assets.cpp.
"""

import argparse
import struct
import sys
from pathlib import Path

from PIL import Image

BMP_BI_RGB = 0


def raster_to_l(path: Path) -> Image.Image:
    """Any PIL-readable image -> 8-bit gray, alpha composited over white."""
    im = Image.open(path)
    if im.mode in ("RGBA", "LA", "PA"):
        im = im.convert("RGBA")
        base = Image.new("RGBA", im.size, (255, 255, 255, 255))
        im = Image.alpha_composite(base, im)
    return im.convert("L")


def parse_bmp(data: bytes, src: str) -> tuple[bytes, int, int]:
    """1-bit uncompressed BMP -> (top-down row-packed payload, w, h)."""
    pix_offset = struct.unpack_from("<I", data, 10)[0]
    w = struct.unpack_from("<i", data, 18)[0]
    h = struct.unpack_from("<i", data, 22)[0]
    planes, bpp = struct.unpack_from("<HH", data, 26)
    compression = struct.unpack_from("<I", data, 30)[0]
    if planes != 1 or bpp != 1 or compression != BMP_BI_RGB:
        raise ValueError(f"{src}: expected 1-bit uncompressed BMP")
    if h < 0:
        raise ValueError(f"{src}: top-down BMP rows unsupported")

    row_bytes = (w + 7) // 8
    stride = row_bytes + (4 - (row_bytes % 4)) % 4
    out = bytearray()
    for y in range(h - 1, -1, -1):  # undo bottom-up storage
        base = pix_offset + y * stride
        out += data[base : base + row_bytes]
    return bytes(out), w, h


def payload_to_image(payload: bytes, w: int, h: int) -> Image.Image:
    """Row-packed MSB-first payload -> 8-bit gray image."""
    im = Image.new("L", (w, h), 255)
    px = im.load()
    row_bytes = (w + 7) // 8
    for y in range(h):
        row = payload[y * row_bytes : (y + 1) * row_bytes]
        for x in range(w):
            if row[x // 8] & (0x80 >> (x % 8)):
                px[x, y] = 0
    return im


def load_input(path: Path, bin_w: int | None, bin_h: int | None) -> Image.Image:
    ext = path.suffix.lower()
    if ext == ".bin":
        if not bin_w or not bin_h:
            raise ValueError("BIN input carries no dimensions; pass --width and --height")
        payload = path.read_bytes()
        expect = ((bin_w + 7) // 8) * bin_h
        if len(payload) != expect:
            raise ValueError(f"{path.name}: {len(payload)} bytes, expected {expect} "
                             f"for {bin_w}x{bin_h}")
        return payload_to_image(payload, bin_w, bin_h)
    if ext == ".bmp":
        payload, w, h = parse_bmp(path.read_bytes(), path.name)
        return payload_to_image(payload, w, h)
    return raster_to_l(path)


def resize_proportional(im: Image.Image, width: int | None, scale: float | None) -> Image.Image:
    if width is not None:
        target_w = width
    elif scale is not None:
        target_w = max(1, round(im.width * scale))
    else:
        return im
    target_h = max(1, round(im.height * target_w / im.width))
    # BOX = area average; survives 1-bit art far better than nearest/lanczos.
    return im.resize((target_w, target_h), Image.BOX)


def write_bmp(im: Image.Image, path: Path) -> None:
    """Write a standard 1-bit uncompressed BMP (bottom-up, 4-byte row pad)."""
    w, h = im.size
    row_bytes = (w + 7) // 8
    pad_bytes = (4 - (row_bytes % 4)) % 4
    stride = row_bytes + pad_bytes
    pixel_data_size = stride * h

    px = im.load()
    rows = []
    for y in range(h - 1, -1, -1):  # BMP is bottom-up
        row = bytearray(row_bytes)
        for x in range(w):
            if px[x, y] == 0:
                row[x // 8] |= 0x80 >> (x % 8)
        rows.append(bytes(row) + b"\x00" * pad_bytes)

    palette = struct.pack("<II", 0x00000000, 0x00FFFFFF)  # color 0=black, 1=white
    offset = 14 + 40 + len(palette)
    header = b"BM" + struct.pack("<IHHI", offset + pixel_data_size, 0, 0, offset)
    dib = struct.pack(
        "<IiiHHIIiiII",
        40, w, h, 1, 1, BMP_BI_RGB,
        pixel_data_size, 2835, 2835, 2, 0,
    )
    path.write_bytes(header + dib + palette + b"".join(rows))


def c_array(name: str, payload: bytes, w: int, h: int) -> str:
    lines = [f"// '{name}', {w}x{h}px", f"const unsigned char {name} [] PROGMEM = {{\t"]
    for i in range(0, len(payload), 16):
        chunk = ", ".join(f"0x{b:02X}" for b in payload[i : i + 16])
        lines.append(f"\t{chunk}, ")
    lines[-1] = lines[-1].rstrip(", ")
    lines.append("};")
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input", type=Path,
                    help="entry point: PNG/JPG (any PIL format), 1-bit BMP, or raw BIN")
    ap.add_argument("name", help="C symbol name for the generated array")
    group = ap.add_mutually_exclusive_group()
    group.add_argument("--width", type=int,
                       help="optional proportional resize to width px (raster/BMP input)")
    group.add_argument("--scale", type=float,
                       help="optional proportional scale factor, e.g. 0.75 (raster/BMP input)")
    ap.add_argument("--height", type=int,
                    help="pixel height (BIN input only; width also required)")
    ap.add_argument("--threshold", type=int, default=128,
                    help="ink threshold 0-255 (default 128)")
    ap.add_argument("--emit", default="bmp,bin,array",
                    help="comma list of stages to emit: bmp,bin,array (default all)")
    ap.add_argument("--out-dir", type=Path, default=Path("."),
                    help="directory for bmp/bin artifacts (default: cwd)")
    args = ap.parse_args()

    try:
        emits = {s.strip() for s in args.emit.split(",") if s.strip()}
        unknown = emits - {"bmp", "bin", "array"}
        if unknown:
            raise ValueError(f"unknown --emit stage(s): {', '.join(sorted(unknown))}")

        im = load_input(args.input, args.width, args.height)
        im = resize_proportional(im, args.width, args.scale)
        im = im.point(lambda p: 0 if p < args.threshold else 1, mode="1")

        w, h = im.size
        if "bmp" in emits or "bin" in emits:
            args.out_dir.mkdir(parents=True, exist_ok=True)

        if "bmp" in emits:
            write_bmp(im, args.out_dir / f"{args.name}.bmp")

        if "bin" in emits or "array" in emits:
            # Reuse the BMP parser on our own output: single source of truth
            # for the byte layout, and proves the emitted BMP round-trips.
            if "bmp" in emits:
                payload, w, h = parse_bmp((args.out_dir / f"{args.name}.bmp").read_bytes(),
                                          f"{args.name}.bmp")
            else:
                px = im.load()
                row_bytes = (w + 7) // 8
                payload = bytearray(row_bytes * h)
                for y in range(h):
                    for x in range(w):
                        if px[x, y] == 0:
                            payload[y * row_bytes + x // 8] |= 0x80 >> (x % 8)
                payload = bytes(payload)
            if "bin" in emits:
                (args.out_dir / f"{args.name}.bin").write_bytes(payload)

        if "array" in emits:
            print(c_array(args.name, payload, w, h))

        note = f"img2asset: {args.name} {w}x{h} ({(w + 7) // 8 * h} bytes)"
        if "bmp" in emits:
            note += f" | wrote {args.out_dir / (args.name + '.bmp')}"
        if "bin" in emits:
            note += f" | wrote {args.out_dir / (args.name + '.bin')}"
        print(note, file=sys.stderr)
        return 0
    except Exception as exc:  # tool failure surface
        print(f"img2asset: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
