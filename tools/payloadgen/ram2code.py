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


import sys

"""Small utility to convert a ram dump file (orig 0x400) to a binary executable.  The binary executable will set the ram contents when executed."""

class Payload:
    def __init__(self):
        self.code = ""

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
            code += newcode

        code += footer
        self.code += code

    def addjsr( self, addr ):
        code = chr(0x20) + chr(addr&255) + chr(addr>>8)
        self.code += code



poker = Payload()
poker.addmemwrite( 0x400, file( sys.argv[1] ,"rb").read() )
poker.addjsr( 0x400 )

file( sys.argv[2],"wb").write( poker.code )
