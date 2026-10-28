"""List or extract files from an Xbox 360 disc image (XDVDFS / XGD2 / XGD3).

Usage:
  python -I xiso_extract.py <image.iso> list
  python -I xiso_extract.py <image.iso> extract <out_dir> [name ...]

With names, only those files (paths relative to the disc root, case insensitive) are extracted.
"""
import os
import struct
import sys

SECTOR = 2048
MAGIC = b"MICROSOFT*XBOX*MEDIA"
# Game partition offsets: plain XISO, XGD2, XGD1, XGD3
PARTITION_OFFSETS = (0, 0xFD90000, 0x18300000, 0x2080000)


def find_partition(f):
    for off in PARTITION_OFFSETS:
        f.seek(off + 32 * SECTOR)
        if f.read(20) == MAGIC:
            return off
    raise SystemExit("not an Xbox disc image (no XDVDFS volume descriptor found)")


def walk(f, part, sector, size, prefix, out):
    f.seek(part + sector * SECTOR)
    data = f.read(size)
    seen = set()

    def node(offset):
        if offset in seen or offset + 14 > len(data):
            return
        seen.add(offset)
        left, right, start, length, attr, namelen = struct.unpack_from("<HHIIBB", data, offset)
        if left == 0xFFFF:
            return
        name = data[offset + 14:offset + 14 + namelen].decode("latin-1")
        if left:
            node(left * 4)
        path = prefix + name
        if attr & 0x10:
            if length:
                walk(f, part, start, length, path + "/", out)
        else:
            out.append((path, part + start * SECTOR, length))
        if right:
            node(right * 4)

    node(0)


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    image, cmd = sys.argv[1], sys.argv[2]
    with open(image, "rb") as f:
        part = find_partition(f)
        f.seek(part + 32 * SECTOR + 20)
        root_sector, root_size = struct.unpack("<II", f.read(8))
        files = []
        walk(f, part, root_sector, root_size, "", files)
        files.sort()
        if cmd == "list":
            print(f"partition offset 0x{part:X}, {len(files)} files, {sum(s for _, _, s in files)} bytes")
            for path, _, size in files:
                print(f"{size:12d} {path}")
            return
        if cmd != "extract" or len(sys.argv) < 4:
            raise SystemExit(__doc__)
        out_dir = sys.argv[3]
        wanted = {n.lower().replace("\\", "/") for n in sys.argv[4:]}
        for path, offset, size in files:
            if wanted and path.lower() not in wanted:
                continue
            dest = os.path.join(out_dir, *path.split("/"))
            if os.path.exists(dest) and os.path.getsize(dest) == size:
                continue
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            f.seek(offset)
            remaining = size
            with open(dest, "wb") as o:
                while remaining:
                    chunk = f.read(min(remaining, 1 << 22))
                    if not chunk:
                        raise SystemExit(f"short read in {path}")
                    o.write(chunk)
                    remaining -= len(chunk)
            print(f"{size:12d} {path}")


if __name__ == "__main__":
    main()
