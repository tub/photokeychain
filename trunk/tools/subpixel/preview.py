import sys
import PIL
import PIL.Image

blend = False  # change this to get a slightly better-looking, but more incorrect, preview
filename = sys.argv[1]

im = PIL.Image.open( filename )

large = im.resize( (im.size[0]*3, im.size[1]), resample = 0 )

for y in range(large.size[1]):
    for x in range(large.size[0]):
        v = large.getpixel( (x,y) )
        component = ((x+2)%3)
        red = (component==0)
        green = (component==1)
        blue = (component==2)
        if blend:
            red = 0.5+0.5*red
            green = 0.5+0.5*green
            blue = 0.5+0.5*blue
        newv = ( red*v[0],  green*v[1], blue*v[2],   )
        large.putpixel( (x,y), newv )

preview = large.resize( (im.size[0]*6, im.size[1]*6) )
preview.save("preview.png")
