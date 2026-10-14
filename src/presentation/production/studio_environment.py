"""Build-only procedural studio source and exact pinned cmgen artifact receiver.

This deliberately receives one fixed KTX closure, not a general KTX loader. Runtime
has no file parser, allocations or third-party image-library dependency.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

WIDTH, HEIGHT = 256, 128
FACE_SIZE, LEVELS = 32, 6
MAX_FILE_BYTES = 1024 * 1024


def bounded_read(path):
    with path.open("rb") as stream:
        data = stream.read(MAX_FILE_BYTES + 1)
    if len(data) > MAX_FILE_BYTES:
        raise ValueError("Studio artifact exceeds fixed cold bound")
    return data


def rgbe(rgb):
    largest = max(rgb)
    if not all(math.isfinite(v) and 0 <= v <= 8 for v in rgb):
        raise ValueError("Nonfinite or unbounded procedural radiance")
    if largest < 1e-32:
        return bytes(4)
    mantissa, exponent = math.frexp(largest)
    scale = mantissa * 256 / largest
    return bytes([min(255, int(v * scale)) for v in rgb] + [exponent + 128])


def generate(path):
    # Three broad neutral/cool studio boxes over a diffuse studio surround.
    # This is authored procedural radiance, not a photographic HDR environment.
    lights = [((0.0, 0.85, 0.53), (3.0, 3.1, 3.3), 0.42),
              ((-0.78, 0.42, 0.46), (1.4, 1.6, 1.8), 0.35),
              ((0.70, 0.35, -0.62), (1.4, 1.6, 1.8), 0.35)]
    lights = [(tuple(v / math.sqrt(sum(x*x for x in direction)) for v in direction), color, spread)
              for direction, color, spread in lights]
    data = bytearray(b"#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y 128 +X 256\n")
    for y in range(HEIGHT):
        theta = math.pi * (y + 0.5) / HEIGHT
        row = []
        for x in range(WIDTH):
            phi = 2 * math.pi * (x + 0.5) / WIDTH
            direction = (math.sin(theta)*math.cos(phi), math.cos(theta), math.sin(theta)*math.sin(phi))
            rgb = [0.45, 0.50, 0.60]
            for light, color, spread in lights:
                weight = math.exp((sum(a*b for a, b in zip(direction, light)) - 1) / spread**2)
                for channel in range(3):
                    rgb[channel] += weight * color[channel]
            row.append(rgbe(rgb))
        # Radiance scanline RLE: literal packets only, bounded at128 bytes each.
        data.extend(bytes((2, 2, WIDTH >> 8, WIDTH & 255)))
        for channel in range(4):
            for start in range(0, WIDTH, 128):
                count = min(128, WIDTH-start)
                data.append(count)
                data.extend(pixel[channel] for pixel in row[start:start+count])
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def pack(ktx_path, sh_path, output):
    ktx, sh = bounded_read(ktx_path), bounded_read(sh_path)
    if len(ktx) < 64 or ktx[:12] != b"\xabKTX 11\xbb\r\n\x1a\n":
        raise ValueError("Expected exact cmgen KTX1 artifact")
    header = struct.unpack_from("<13I", ktx, 12)
    expected = (0x04030201, 0x8c3a, 1, 0x1907, 0x8c3a, 0x8c3a, FACE_SIZE, FACE_SIZE, 0, 0, 6, LEVELS)
    if header[:12] != expected:
        raise ValueError(f"Unexpected fixed studio KTX closure: {header}")
    if len(sh) != 9*3*4 or not all(math.isfinite(v) and abs(v) <= 64 for v in struct.unpack("<27f", sh)):
        raise ValueError("Expected bounded nine pre-scaled RGB SH coefficients")
    offset = 64 + header[12]
    if offset > len(ktx):
        raise ValueError("Truncated studio metadata")
    raw = bytearray(sh)
    for level in range(LEVELS):
        dimension = FACE_SIZE >> level
        face_bytes = dimension*dimension*4
        if offset+4 > len(ktx) or struct.unpack_from("<I", ktx, offset)[0] != face_bytes:
            raise ValueError("Unexpected studio face/mip size")
        offset += 4
        for face in range(6):
            if offset+face_bytes > len(ktx):
                raise ValueError("Truncated studio face")
            face_data = ktx[offset:offset+face_bytes]
            for (pixel,) in struct.iter_unpack("<I", face_data):
                if ((pixel >> 6) & 31) == 31 or ((pixel >> 17) & 31) == 31 or ((pixel >> 27) & 31) == 31:
                    raise ValueError("Nonfinite packed studio radiance")
            raw.extend(face_data)
            offset += face_bytes
    if offset != len(ktx) or len(raw) != 108+sum(6*4*(FACE_SIZE >> level)**2 for level in range(LEVELS)):
        raise ValueError("Trailing or incomplete studio closure")
    output.write_bytes(raw)
    receipt = {"source": "procedural studio; cmgen GGX prefilter and pre-scaled SH",
               "face_size": FACE_SIZE, "levels": LEVELS, "faces": 6,
               "cpu_packaged_bytes": len(raw), "gpu_texture_bytes": len(raw)-108,
               "generator_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
               "ktx_sha256": hashlib.sha256(ktx).hexdigest(),
               "sh_sha256": hashlib.sha256(sh).hexdigest(),
               "packed_sha256": hashlib.sha256(raw).hexdigest()}
    output.with_suffix(".json").write_text(json.dumps(receipt, indent=2)+"\n")


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="action", required=True)
    generate_parser = sub.add_parser("generate")
    generate_parser.add_argument("output", type=Path)
    pack_parser = sub.add_parser("pack")
    for argument in ("ktx", "sh", "output"):
        pack_parser.add_argument(argument, type=Path)
    arguments = parser.parse_args()
    if arguments.action == "generate":
        generate(arguments.output)
    else:
        pack(arguments.ktx, arguments.sh, arguments.output)


if __name__ == "__main__":
    main()
