"""Finds functions reached only through pointers (vtables, callbacks), which
rexglue's analysis did not register, and prints [functions] entries for them.

  python -I tools/find_indirect_funcs.py sptr/image.bin sptr/generated/default 82150000 8282F3DC

image.bin comes from SPTR_DUMP_IMAGE=<file>. A candidate is a code address that
is stored in data or built with lis/addi(ori), starts right after a terminator
(blr, bctr, b, or zero padding), and is neither a known function nor a label
inside one (labels of switch tables are reached the same way).
"""
import json, os, re, struct, sys

img_path, gen, lo, hi = sys.argv[1], sys.argv[2], int(sys.argv[3], 16), int(sys.argv[4], 16)
IMG = open(img_path, 'rb').read()
BASE = 0x82000000


def u32(a):
    return struct.unpack_from('>I', IMG, a - BASE)[0]


def in_code(a):
    return lo <= a < hi and (a & 3) == 0


def is_terminator(w):
    return w in (0x4E800020, 0x4E800420, 0) or ((w >> 26) == 18 and (w & 3) == 0)


known = {int(k, 16) for k in json.load(open(os.path.join(gen, 'codegen.partition.json')))['assignments']}
labels = set()
for f in os.listdir(gen):
    if '_recomp.' in f and f.endswith('.cpp'):
        labels.update(int(x, 16) for x in re.findall(r'^loc_([0-9A-F]{8}):', open(os.path.join(gen, f)).read(), re.M))

# SEH scope tables: {count, count x {begin, end, filter, handler}}. Their
# addresses are blocks inside the function they belong to, not functions.
scope = set()
for a in range(BASE, BASE + len(IMG) - 3, 4):
    n = u32(a)
    if lo <= a < hi or not 1 <= n <= 32 or a + 4 + n * 16 > BASE + len(IMG):
        continue
    words = [u32(a + 4 + i * 4) for i in range(n * 4)]
    if all(in_code(w) or (i % 4 >= 2 and w <= 1) for i, w in enumerate(words)):
        scope.update(range(a + 4, a + 4 + n * 16, 4))

refs = {}
for a in range(BASE, BASE + len(IMG) - 3, 4):
    if lo <= a < hi or a in scope:
        continue
    v = u32(a)
    if in_code(v):
        refs.setdefault(v, f'data pointer at 0x{a:08X}')
lis = [None] * 32
for a in range(lo, hi, 4):
    w = u32(a)
    op, rd, ra, imm = w >> 26, (w >> 21) & 31, (w >> 16) & 31, w & 0xFFFF
    if op == 15 and ra == 0:
        lis[rd] = (imm << 16) & 0xFFFFFFFF
        continue
    if op in (14, 24) and lis[ra] is not None:
        v = (lis[ra] + (imm - 0x10000 if imm & 0x8000 else imm)) & 0xFFFFFFFF if op == 14 else lis[ra] | imm
        if in_code(v):
            refs.setdefault(v, f'lis/{"addi" if op == 14 else "ori"} at 0x{a:08X}')
    if is_terminator(w):
        lis = [None] * 32

found = []
for target, why in sorted(refs.items()):
    if target in known or target in labels or not is_terminator(u32(target - 4)) or u32(target) == 0:
        continue
    w = u32(target - 12)
    if u32(target - 4) == 0x4E800420 and w >> 26 == 31 and (w >> 1) & 0x3FF == 23:
        continue  # lwzx; mtctr; bctr: an inline jump table follows, not code
    found.append((target, why))
for target, why in found:
    print(f'"0x{target:08X}" = {{}}  # {why}')
print(f'# {len(found)} candidates', file=sys.stderr)
