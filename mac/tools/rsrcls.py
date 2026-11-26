#!/usr/bin/env python3
"""List a resource fork's resources: type, id, size, first bytes.
usage: rsrcls.py <file>   (reads <file>/..namedfork/rsrc on macOS if the
data file itself is not a raw resource fork)."""
import os, struct, sys
p = sys.argv[1]
d = open(p, 'rb').read()
if len(d) < 16:
    d = open(os.path.join(p, '..namedfork', 'rsrc'), 'rb').read()
doff, moff, dlen, mlen = struct.unpack('>IIII', d[:16])
m = d[moff:moff + mlen]
tlo = struct.unpack('>H', m[24:26])[0]
t = m[tlo:]
for i in range(struct.unpack('>h', t[:2])[0] + 1):
    typ, cnt, ref = struct.unpack('>4shH', t[2 + 8 * i:10 + 8 * i])
    for j in range(cnt + 1):
        rid, noff, a = struct.unpack('>hhI', t[ref + 12 * j:ref + 12 * j + 8])
        off = doff + (a & 0xFFFFFF)
        ln = struct.unpack('>I', d[off:off + 4])[0]
        print("%-6s %6d %6d  %s" % (typ.decode('mac_roman'), rid, ln, d[off + 4:off + 4 + min(ln, 24)].hex()))
