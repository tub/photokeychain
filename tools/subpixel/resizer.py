import PIL
import PIL.Image

im = PIL.Image.open("input.png").convert("RGB")
resizedNormal = im.resize( (128,128), resample=1 )
resizedClear = im.resize( (128*3,128), resample=1 )

def compactRGB( im ):
    newimage = im.resize( (128,128) )
    for y in range(newimage.size[1]):
        for x in range(newimage.size[0]):
            compV = [0,0,0]
            for component in range(3):
                v = im.getpixel( (x*3+component, y) )
                compV[component] = v[component]
            newimage.putpixel( (x,y), tuple(compV) )
    return newimage

test = compactRGB( resizedClear )
test.save("rgb.png")

        

