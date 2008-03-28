import sys
from math import *

if len(sys.argv)<4:
    min = 0
    max = 255
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


x = [min+(max-min)*(0.5+0.5*func(x / float(nr))) for x in range(nr)]

print ";;; sine table generated with mksintab min=%d max=%d nr=%d func='%s'" % ( min, max, nr, tfunc )
print "sindata ",
for p in range(1+nr / 16):
    sub = x[p*16:(p+1)*16]
    subs = [str(int(val)) for val in sub]
    if len(subs)>0:
        print "    db", ",".join( subs )
