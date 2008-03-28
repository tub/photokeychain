import sys
from math import *

if len(sys.argv)<4:
    min = 0
    max = 1
    nr = 256
else:
    min = float(sys.argv[1])
    max = float(sys.argv[2])
    nr = int(sys.argv[3])

if len(sys.argv)>4:
    tfunc = sys.argv[4]
else:
    tfunc = 'sin(x*2.0*pi)'

def func(x):
    a = eval(tfunc)
    return a


r = [min+(max-min)*(0.5+0.5*func(0.1 + x / float(nr))) for x in range(nr)]
g = [min+(max-min)*(0.5+0.5*func(0.5 + x / float(nr))) for x in range(nr)]
b = [min+(max-min)*(0.5+0.5*func(x / float(nr))) for x in range(nr)]
color = [(int(red*31) << 11) + (int(green*63) << 5) + (int(blue*31)) for red, green, blue in zip(r, g, b)]

print ";;; color lookup table generated with mkclut.py min=%d max=%d nr=%d func='%s'" % ( min, max, nr, tfunc )
tbl_hi = ""
tbl_lo = ""
for p in range(1+nr / 16):
    sub = color[p*16:(p+1)*16]
    subs_hi = [str(int(val / 256)) for val in sub]
    subs_lo = [str(int(val % 256)) for val in sub]
    if len(subs_hi)>0:
        tbl_hi += "    db " + ",".join( subs_hi ) + "\n"
    if len(subs_lo)>0:
        tbl_lo += "    db " + ",".join( subs_lo ) + "\n"

print "pp_clut_hi", tbl_hi
print "pp_clut_lo", tbl_lo
