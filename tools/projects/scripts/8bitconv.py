import PIL
import PIL.Image
import sys
from array import array
# convert images to 8bit 332 color


def pack332( r,g,b ):
    return (r<<5) + (g<<2) + b;

print "opening input image %s" % sys.argv[1]

im = PIL.Image.open(sys.argv[1]).convert("RGB")
ar = array('B',[0 for x in range( im.size[0] * im.size[1] )] )
print "size = %dx%d" % im.size

for y in range(im.size[1]):
    for x in range(im.size[0]):
        r,g,b = im.getpixel( (x,y) )
        col = pack332( r>>5, g>>5, b>>6 )
        ar[x+y*im.size[0]] = col

print "writing output to %s" % sys.argv[2]

file(sys.argv[2],"wb").write( ar.tostring() )
