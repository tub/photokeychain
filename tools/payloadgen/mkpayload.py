# Payload Generator for execute-from-memory patched tomtec keychain
# (C) 2008 Jorik Blaas, GPL

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

from math import sin
import PIL
import PIL.Image
import array

def tolist(x):
    return [a for a in x]

def tostring(x):
    return "".join(x)

def patch( code, changes ):
    "take a dict of changes which is patched at offets:newvalue"
    outcode = ""
    for x in range(len(code)):
        ch = code[x]
        if x in changes:
            # print "patch byte at pos %d from %s to %s" % (x, hex(ord(ch)), hex(changes[x]) )
            if ch != chr(1):
                raise "invalid patchvalue, ch=%s" % (hex(ord(ch)),)
            ch = chr(changes[x])
        outcode += ch
    return outcode

class Payload(object):
    def __init__(self):
        self.code = ""
        self.rts = chr(0x60)
        pass

    

    def pad64( self,incode ):
        if len(incode)>64:
            raise "input code packet too long!"
        incode += self.rts * (64-len(incode))
        return incode

    def getcodepacket( self, filename ):
        return self.pad64( file( filename , "rb").read() )

    def addrawcodepacket( self,filename ):
        self.code += self.getcodepacket( filename )
    
    def addwritebinaryline( self,bitmask ):
        print "bitmask=",bitmask
        code = self.getcodepacket( "lcdwritebinary.bin" )
        code = tolist( code )
        ofs=10
        if code[ofs] != chr(1):
            raise "wrong code offset for patching and value"
        code[ofs] = chr(bitmask)
        self.code += tostring(code)

    def addwritetestline( self,bitmask ):
        print "bitmask=",bitmask
        code = self.getcodepacket( "lcdwritetest.bin" )
        code = tolist( code )
        ofs=18
        if code[ofs] != chr(1):
            raise "wrong code offset for patching and value"
        code[ofs] = chr(bitmask)
        self.code += tostring(code)
    
    def mkmemwrite( self, addr, value ):
        # lda i : a9 i 
        # sta addr : 8d lo(addr) hi(addr)
        code = chr(0xa9) + chr(value) + chr(0x8d)  + chr(addr&255) + chr(addr>>8)
        return code

    def addmemwrite( self, startaddr, data, fixedaddress=False, header="", footer="" ):
        code = header
        for x in range(len(data)):
            if fixedaddress:
                addr = startaddr
            else:
                addr = startaddr + x
            val = ord(data[x])
            
            newcode = self.mkmemwrite( addr, val )
            if len( code+newcode+footer ) >= 63:
                code += footer+self.rts
                self.code += self.pad64( code )
                code = header
            code += newcode

        code += footer+self.rts
        self.code += self.pad64( code )

    def addjsr( self, addr ):
        code = chr(0x20) + chr(addr&255) + chr(addr>>8)
        self.code += self.pad64( code )

    def addlcdmemwrite( self, data ):
        selectlcd = [0xa5 ,0x34 ,0x48 ,0xa5 ,0x35 ,0x48 ,0xa9 ,0x03 ,0x85 ,0x35]
        unselectlcd = [0x68 ,0x85 ,0x35 ,0x68 ,0x85 ,0x34 ]
        selectlcdcode = tostring([chr(x) for x in selectlcd])
        unselectlcdcode = tostring([chr(x) for x in unselectlcd])
        self.addmemwrite( 0xc000, data, fixedaddress=True, header=selectlcdcode, footer=unselectlcdcode )

    def addimageshow( self, filename ):
        self.addrawcodepacket( "lcdinit.bin" )
        im = PIL.Image.open( filename )
        if (im.size != (128,128)):
            raise "invalid image! size should be 128x128 but it is %dx%d" % data.size
        if im.mode != 'RGB':
            raise "image data should be in RGB format!"
        data = array.array('H', [0 for x in range(128*128)])
        for y in range(im.size[1]):
            for x in range(im.size[0]):
                v = im.getpixel( (x,y) )
                i = ((v[0]>>3)<<11) + ((v[1]>>2)<<5) + ((v[2]>>3)<<0)
                data[ x + y*128 ] = ((i&255)<<8) + (i>>8)

        self.addlcdmemwrite( data.tostring() )        
        # self.addmemwrite( 0xC000, data.tostring()[0:256], fixedaddress=True )

    def addlcdscroll( self, offset ):
        code = self.getcodepacket( "lcdscroll.bin" )
        code = patch( code, {34 : offset} )
        self.code += code


def mempeeker():

    p = Payload()
    p.addrawcodepacket( "memwriter.bin" )
    p.addrawcodepacket( "lcdinit.bin" )
    for i in range(128):
        p.addrawcodepacket( "lcdwriteline.bin" )

    p.addrawcodepacket( "lcdgettemp.bin" )
    p.addrawcodepacket( "lcdinit.bin" )
    for bit in range(8):
        for repeat in range(15):
            p.addwritebinaryline( 2**bit )
        p.addrawcodepacket( "lcdwriteline.bin" )
    return p

def memtesterpeeker():

    p = Payload()
    p.addrawcodepacket( "memwriter.bin" )
    p.addrawcodepacket( "lcdinit.bin" )
    for i in range(128):
        p.addrawcodepacket( "lcdwriteline.bin" )

    p.addrawcodepacket( "lcdinit.bin" )
    for bit in range(8):
        for repeat in range(15):
            p.addwritetestline( 2**bit )
        p.addrawcodepacket( "lcdwriteline.bin" )
    return p



# p = Payload()
# p.addlcdscroll( 0 )
# p.addimageshow( "rgb.png" )
# for x in range(5120):
#     p.addlcdscroll( int((132+64*sin(x*0.01))%132) )
# p.addlcdscroll( 0 )

# #p = mempeeker()

# print "generated %d byte payload in %d packets" % ( len(p.code), len(p.code)/64 )
# file("payload.bin","wb").write(p.code)


booter = Payload()
booter.addmemwrite( 0x400, file("bootcode.bin.ram","rb").read() )
booter.addjsr( 0x400 )
print "generated bootcode payload in %d byte / %d packets" % ( len(booter.code), len(booter.code)/64 )
file("payload-bootcode.bin","wb").write(booter.code)
