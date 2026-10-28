"""Dev tool: lists a Lua 5.1 bytecode chunk (the game's menu scripts, saved from guest memory with ABT_DEV_FIND).

usage: python -I tools/luadis.py <file> [function filter] [chunk name part]

Finds the first "\\x1bLuaQ" in the file (or the one whose source name contains the chunk name part) and prints every function: its source lines, constants, locals, upvalues and
instructions, with constants and globals filled in. The game's chunks use 4 byte ints, size_t, instructions and float
numbers; the byte order comes from the header.
"""
import struct
import sys

OPS = ['MOVE', 'LOADK', 'LOADBOOL', 'LOADNIL', 'GETUPVAL', 'GETGLOBAL', 'GETTABLE', 'SETGLOBAL', 'SETUPVAL',
       'SETTABLE', 'NEWTABLE', 'SELF', 'ADD', 'SUB', 'MUL', 'DIV', 'MOD', 'POW', 'UNM', 'NOT', 'LEN', 'CONCAT',
       'JMP', 'EQ', 'LT', 'LE', 'TEST', 'TESTSET', 'CALL', 'TAILCALL', 'RETURN', 'FORLOOP', 'FORPREP', 'TFORLOOP',
       'SETLIST', 'CLOSE', 'CLOSURE', 'VARARG']


class Reader:
    def __init__(self, data, pos):
        self.d, self.p = data, pos

    def take(self, n):
        b = self.d[self.p:self.p + n]
        self.p += n
        return b

    def byte(self):
        return self.take(1)[0]

    def int(self):
        return struct.unpack(self.e + 'i', self.take(4))[0]

    def uint(self):
        return struct.unpack(self.e + 'I', self.take(4))[0]

    def num(self):
        return struct.unpack(self.e + 'f', self.take(4))[0]

    def string(self):
        n = self.uint()
        return self.take(n)[:-1].decode('latin1') if n else None


def function(r, depth, out):
    f = {'source': r.string(), 'line': r.int(), 'lastline': r.int(), 'nups': r.byte(), 'params': r.byte(),
         'vararg': r.byte(), 'stack': r.byte()}
    f['code'] = [r.uint() for _ in range(r.int())]
    consts = []
    for _ in range(r.int()):
        t = r.byte()
        consts.append(None if t == 0 else bool(r.byte()) if t == 1 else r.num() if t == 3 else r.string())
    f['consts'] = consts
    f['protos'] = [function(r, depth + 1, out) for _ in range(r.int())]
    f['lines'] = [r.int() for _ in range(r.int())]
    f['locals'] = []
    for _ in range(r.int()):
        f['locals'].append((r.string(), r.int(), r.int()))
    f['upvals'] = [r.string() for _ in range(r.int())]
    out.append(f)
    return f


def rk(f, x):
    return repr(f['consts'][x - 256]) if x >= 256 else f'r{x}'


def show(f, idx):
    print(f"\n=== function {idx}: lines {f['line']}-{f['lastline']} params {f['params']} upvalues {f['upvals']}")
    print('    locals:', [l[0] for l in f['locals']])
    for pc, i in enumerate(f['code']):
        op, a = i & 0x3F, (i >> 6) & 0xFF
        c, b = (i >> 14) & 0x1FF, (i >> 23) & 0x1FF
        bx = i >> 14
        sbx = bx - 131071
        name = OPS[op] if op < len(OPS) else f'op{op}'
        line = f['lines'][pc] if pc < len(f['lines']) else 0
        k = f['consts']
        if name in ('LOADK', 'GETGLOBAL', 'SETGLOBAL'):
            arg = f'r{a} {k[bx]!r}'
        elif name in ('JMP', 'FORLOOP', 'FORPREP'):
            arg = f'r{a} -> {pc + 1 + sbx}'
        elif name in ('GETTABLE', 'SELF'):
            arg = f'r{a} = r{b}[{rk(f, c)}]'
        elif name == 'SETTABLE':
            arg = f'r{a}[{rk(f, b)}] = {rk(f, c)}'
        elif name in ('GETUPVAL', 'SETUPVAL'):
            arg = f"r{a} up {f['upvals'][b] if b < len(f['upvals']) else b}"
        elif name == 'CLOSURE':
            arg = f'r{a} function {bx}'
        elif name in ('EQ', 'LT', 'LE', 'ADD', 'SUB', 'MUL', 'DIV'):
            arg = f'{a} {rk(f, b)} {rk(f, c)}'
        else:
            arg = f'{a} {b} {c}'
        print(f'    {pc:4} [{line:4}] {name:9} {arg}')


def main():
    data = open(sys.argv[1], 'rb').read()
    flt = sys.argv[2] if len(sys.argv) > 2 else None
    pos = data.find(b'\x1bLuaQ')
    if len(sys.argv) > 3:
        pos = data.rfind(b'\x1bLuaQ', 0, data.find(sys.argv[3].encode()))
    r = Reader(data, pos + 12)
    r.e = '<' if data[pos + 6] == 1 else '>'
    out = []
    try:
        function(r, 0, out)
    except Exception as e:  # the saved window may cut the chunk short
        print('stopped:', e)
    for idx, f in enumerate(out):
        if not flt or any(flt == c for c in f['consts'] if isinstance(c, str)) or flt in [l[0] for l in f['locals']]:
            show(f, idx)


main()
