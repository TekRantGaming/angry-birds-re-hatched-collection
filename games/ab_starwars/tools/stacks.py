"""List the game functions on every thread's stack in a minidump, named from the build's linker map.

usage: python -I tools/stacks.py <dump.dmp> <ab_starwars.map> [max per thread]

A heuristic stack scan: every 8 byte value on the stack that falls inside ab_starwars.exe is looked up in the map
(the nearest symbol below it). The guest functions show as sub_XXXXXXXX / __imp__sub_XXXXXXXX. Write the dump with
tools/live_dump.ps1 (no need to stop the game).
"""
import bisect
import re
import struct
import sys

from minidump.minidumpfile import MinidumpFile

dmp, map_path = sys.argv[1], sys.argv[2]
limit = int(sys.argv[3]) if len(sys.argv) > 3 else 14

syms = []
pref = 0x140000000
for line in open(map_path, errors='ignore'):
    m = re.match(r'\s*0001:[0-9a-f]+\s+(\S+)\s+([0-9a-f]{16})\s', line)
    if m:
        syms.append((int(m.group(2), 16), m.group(1)))
syms.sort()
addrs = [a for a, _ in syms]

md = MinidumpFile.parse(dmp)
reader = md.get_reader()
mod = next(m for m in md.modules.modules if m.name.lower().endswith('ab_starwars.exe'))
lo, hi = mod.baseaddress, mod.baseaddress + mod.size


def name(va):
    i = bisect.bisect_right(addrs, va - lo + pref) - 1
    return syms[i][1] if i >= 0 else '?'


for th in md.threads.threads:
    ctx = th.ContextObject
    data = b''
    for off in range(0, 0x80000, 0x200):  # small steps: the dump holds only the used part of each stack
        try:
            data += reader.read(ctx.Rsp + off, 0x200)
        except Exception:
            break
    seen = []
    for i in range(0, len(data) - 8, 8):
        v = struct.unpack_from('<Q', data, i)[0]
        if lo <= v < hi:
            n = name(v)
            if n not in seen:
                seen.append(n)
        if len(seen) >= limit:
            break
    if seen:
        print(f'thread {th.ThreadId}: ' + ' < '.join(seen))
