"""Dev tool: finds the C functions behind the game's Lua globals from the code that registers them
(obj->vfunc[0x18](obj, name, func), with name and func built by lis/addi).

usage: python -I tools/find_lua_regs.py <Lua name> [...]   (run from the abt folder; reads image.bin)
Not every global is registered this way; about 130 are found.
"""
import struct,sys
img=open('image.bin','rb').read(); B=0x82000000
def u(a): return struct.unpack_from('>I',img,a-B)[0]
regs={}
lis={}
# simple linear scan over code tracking lis values per register; on 'addi r4,rX,lo' & 'addi r5,rY,lo' followed by bctrl
start,end=0x82100000,0x82730000
r4=r5=None
for a in range(start,end,4):
    w=u(a); op=w>>26
    if op==15 and ((w>>16)&31)==0:
        lis[(w>>21)&31]=(w&0xFFFF)<<16
    elif op==14:
        rd=(w>>21)&31; ra=(w>>16)&31; lo=w&0xFFFF; lo=lo-0x10000 if lo&0x8000 else lo
        if ra in lis:
            v=(lis[ra]+lo)&0xFFFFFFFF
            if rd==4: r4=v
            if rd==5: r5=v
    elif w==0x4E800421:  # bctrl
        if r4 and r5 and 0x82000000<=r4<0x82200000 and 0x82200000<=r5<0x82800000:
            e=img.index(b'\0',r4-B); name=img[r4-B:e]
            if name and all(32<c<127 for c in name): regs[name.decode()]=r5
        r4=r5=None
    elif w>>26==18 and (w&1)==0: lis={}
for n in sys.argv[1:]: print(n, '%08X'%regs[n] if n in regs else '-')
print(len(regs))
