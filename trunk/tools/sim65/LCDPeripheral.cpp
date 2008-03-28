/*
 * This file is part of the sim65/photoframe project.
 *
 * Copyright 2008 Jorik Blaas
 * Other parts of this project are (C) 2003 William Sheldon Simms III
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "LCDPeripheral.h"

#include <string.h>
#include <stdlib.h>

// ap_reader_t ap_reader [ADDRESS_SPACE_SIZE];
// ap_writer_t ap_writer [ADDRESS_SPACE_SIZE];

LCDPeripheral::LCDPeripheral()
{
  command = 0x2c;
  for(int x=0;x<sizeof(framebufferBYTE)/sizeof(framebufferBYTE[0]);x++)
    framebufferBYTE[x] = 0;
  write_x = 0;
  write_y = 0;
  x_adr_start = 0;
  x_adr_end = 131;
  y_adr_start = 0;
  y_adr_end = 131;

  tfa=0;  // top fixed area
  vsa=132;  // verical scrolling area
  bfa=0;  // bottom fixed area

  madctr_y = 0;
  madctr_x = 0;
  madctr_rowcol = 0;
  

  scroll_offset = 0;

  color_interface = 5;

  for(int c=0;c<16;c++)
    {
      colorlookup[c + 0] = c;
      colorlookup[c + 16] = c<<1;
      colorlookup[c + 32] = c;
    }
}

void LCDPeripheral::dumpPPM( char* filename )
{
  FILE * fp = fopen(filename,"wb");
  fprintf(fp, "P6\n132 132\n255\n");
  for( int x=0;x<132*132;x++)
    {
      unsigned short v = (framebufferBYTE[x*2+0]<<8) + framebufferBYTE[x*2+1];
      unsigned char red = ( v >> 11) << 3;
      unsigned char green = (v >> 5) << 2;
      unsigned char blue = (v) << 3;
      fputc( red, fp );
      fputc( green, fp );
      fputc( blue, fp );
    }
  fclose(fp);
}


ap_reader_t LCDPeripheral::GetReader (unsigned int idx)
{
  assert(idx < 1);

  ap_reader_t reader;
  reader.ap = this;
  reader.read = (ap_rfunc_t)&LCDPeripheral::Read;

  return reader;
}

ap_writer_t LCDPeripheral::GetWriter (unsigned int idx)
{
  assert(idx < 1);

  ap_writer_t writer;
  writer.ap = this;
  writer.write = (ap_wfunc_t)&LCDPeripheral::Write;

  return writer;
}

unsigned char LCDPeripheral::Read (unsigned short addr)
{
  printf("LCD read from addr %04x\n",addr);
  return 0;
}

void LCDPeripheral::nextWritePixel()
{
  write_x++;
  if (write_x > x_adr_end) 
    {
      write_x = x_adr_start;
      write_y++;
      if (write_y > y_adr_end) 
        {
          write_y = y_adr_start;
        }
    }
}

void LCDPeripheral::map8bitRGB( unsigned char byte, unsigned char* _lo, unsigned char* _hi)
{
  // first we have to extract red,green,blue from the packed byte. these
  // numbers are used to index into the color lookup table.
  unsigned char red = (byte>>5);
  unsigned char green = ((byte>>2) & 0x7);
  unsigned char blue =  (byte & 0x3);

  red = colorlookup[red];
  green = colorlookup[16+green];
  blue = colorlookup[32+blue];

  //  unsigned short w = (red<<12) + (green<<6) + (blue<<1);
  unsigned short w = (red<<11) + (green<<5) + (blue);

  *_lo = w & 255;
  *_hi = w >> 8;
}


void LCDPeripheral::map12bitRGB( unsigned short rgbword, unsigned char* _lo, unsigned char* _hi)
{
  unsigned char red = (rgbword>>8) & 0xf;
  unsigned char green = (rgbword>>4) & 0xf;
  unsigned char blue = (rgbword>>0) & 0xf;

  red = colorlookup[red];
  green = colorlookup[16+green];
  blue = colorlookup[32+blue];

  unsigned short w = (red<<11) + (green<<5) + (blue);

  *_lo = w & 255;
  *_hi = w >> 8;
}

void LCDPeripheral::getXYAfterMadctr( int& x, int& y )
{
  if (madctr_rowcol) {
    y = write_x;
    x = write_y;
  } else {
    x = write_x;
    y = write_y;
  }
  
  if (madctr_x) {
    x = WIDTH - x;
  }
  if (madctr_y) {
    y = HEIGHT - y;
  }

}

void LCDPeripheral::setPixel( unsigned int x, unsigned int y, unsigned char lo, unsigned char hi )
{
  if (x<0 || x>=WIDTH || y<0 || y>=HEIGHT)
    return;

  framebufferBYTE[ 0 + (x + y * WIDTH) * 2 ] = hi;
  framebufferBYTE[ 1 + (x + y * WIDTH) * 2 ] = lo;
}



void LCDPeripheral::Write (unsigned short addr, unsigned char byte)
{
  if ((addr&0xf000)==0x0) // control command port
    {
      command = byte;
      commandseq = 0; // sequence number for multi-byte write commands
      switch( command )
        {
        case 0x2a: // column address set (X)
          break;
        case 0x2b: // row address set (Y)
          break;
        case 0x2c:  // data write mode
          write_x = x_adr_start;
          write_y = y_adr_start;
          break;
        case 0x2d: // RGBSET for color lookup tables
          break;
        case 0x33: // scroll area
          break;
        case 0x36: // MADCTR
          break;
        case 0x37: // scroll address
          break;
        case 0x3a: // color interface mode
          break;
        default:
          printf("unsupported LCD control code 0x%02x\n", byte );
        }
    }
  else if ( (addr&0xf000)==0x4000) // data port
    {
      switch(command)
        {
        case 0x2a: // column address set (X)
          if (commandseq == 0) {
            x_adr_start = byte;
            // this was removed since the device itself doesn't clip the
            // rectangle, but instead clips the written pixels

            // if (x_adr_start >= WIDTH)
            // x_adr_start = WIDTH-1;
          } else {
            x_adr_end = byte;
            // this was removed since the device itself doesn't clip the
            // rectangle, but instead clips the written pixels

            //             if (x_adr_end >= WIDTH)
            //               x_adr_end = WIDTH-1;
          }
          break;
        case 0x2b: // column address set (Y)
          if (commandseq == 0) {
            y_adr_start = byte;
            // this was removed since the device itself doesn't clip the
            // rectangle, but instead clips the written pixels
            //             if (y_adr_start >= HEIGHT)
            //               y_adr_start = HEIGHT-1;
          } else {
            y_adr_end = byte;
            // this was removed since the device itself doesn't clip the
            // rectangle, but instead clips the written pixels
            //             if (y_adr_end >= HEIGHT)
            //               y_adr_end = HEIGHT-1;
          }
          break;
        case 0x2c: // streaming data write RAMWR
          int x, y;
          
          getXYAfterMadctr( x, y );

          switch( color_interface )
            {
            case 2: // 8bit
              {
                unsigned char lobyte, hibyte;
                map8bitRGB( byte, &lobyte, &hibyte );
                setPixel( x,y, lobyte, hibyte );
                nextWritePixel();
              }
              break;
            case 3: // 12bit mode A
              pixwritebuffer[commandseq%3] = byte;
              // 3 bytes are 2 pixels
              if ((commandseq % 3) == 2)
                {
                  // unpack the received data into a 24bit word, which contains RRRRGGGGBBBBRRRRGGGGBBBB (two pixels total)
                  unsigned int pixels = 
                    (((unsigned int)pixwritebuffer[0])<<16) +
                    (((unsigned int)pixwritebuffer[1])<<8) +
                    (((unsigned int)pixwritebuffer[2])<<0);
                  {
                    unsigned char lobyte, hibyte;
                    map12bitRGB( (pixels>>12) & 0xfff , &lobyte, &hibyte );
                    setPixel( x,y, lobyte, hibyte );

                    nextWritePixel();
                    getXYAfterMadctr( x, y );

                    map12bitRGB( pixels & 0xfff , &lobyte, &hibyte );
                    setPixel( x,y, lobyte, hibyte );

                    nextWritePixel();
                  }
                  
                }
              break;
            case 4: // 12bit mode B
              pixwritebuffer[commandseq%2] = byte;
              // 2 bytes are 1 pixel
              if ((commandseq % 2) == 1)
                {
                  // unpack the received data into a 12bit word, which contains RRRRGGGGBBBB (one pixels total)
                  unsigned int pixels = 
                    (((unsigned int)pixwritebuffer[0])<<8) +
                    (((unsigned int)pixwritebuffer[1])<<0);
                  {
                    unsigned char lobyte, hibyte;
                    map12bitRGB( pixels & 0xfff , &lobyte, &hibyte );
                    setPixel( x,y, lobyte, hibyte );
                    nextWritePixel();
                  }
                  
                }
              break;
            case 5: // 16bit
              pixwritebuffer[commandseq%2] = byte;
              // framebufferBYTE[ (commandseq%2) + (x + y * WIDTH) * 2 ] = byte;
              if ( (commandseq % 2) == 1) 
                {
                  setPixel( x,y, pixwritebuffer[1], pixwritebuffer[0] );
                  nextWritePixel();
                }
              break;
            case 6: // 18bit truncated mode, which is broken and revert to 24bit per pixel mode
            case 7: // 24bit truncated mode, one byte per color component
              pixwritebuffer[commandseq%3] = byte;
              // 3 bytes is only one pixel
              if ((commandseq % 3) == 2)
                {
                  // unpack the received data into a 32bit word, which contains RRRRRRRRGGGGGGGGBBBBBBBB (one pixels total)
                  unsigned int pixels = 
                    (((unsigned int)pixwritebuffer[0])<<16) +
                    (((unsigned int)pixwritebuffer[1])<<8) +
                    (((unsigned int)pixwritebuffer[2])<<0);
                  {
                    unsigned char red =   (pixels&0xff0000) >> (24-5);
                    unsigned char green = (pixels&0x00ff00) >> (16-6);
                    unsigned char blue =  (pixels&0x0000ff) >> (8-5);
                    unsigned short col = (red << 11) + (green<<5) + blue;
                    unsigned char lobyte, hibyte;
                    hibyte = col>>8;
                    lobyte = col&255;
                    setPixel( x , y, lobyte, hibyte );
                    nextWritePixel();
                  }
                }
              break;
            default:
              printf("invalid color_interface mode used %d\n", color_interface );
            }
          
          break;
        case 0x2d:
          // this command recevies a lookup table, consisting of 16 entries for each color (48 in total)
          // each entry is a partially used bytes, for each value, the lower 5 bits represent the color intensity (6 bits in the case of green)
          if (commandseq<48)
            colorlookup[commandseq] = byte;
          break;
        case 0x33:
          if (commandseq == 0)
            tfa = byte;
          else if (commandseq == 1)
            vsa = byte;
          else if (commandseq == 2)
            bfa = byte;
          break;
        case 0x36: // memory data access control MADCTR
          madctr_y = (byte >> 7) & 1;
          madctr_x = (byte >> 6) & 1;
          madctr_rowcol = (byte >> 5) & 1;
          break;
        case 0x37: // scroll address
          scroll_offset = byte;
          break;
        case 0x3a: // color interface mode setting
          color_interface = byte & 7;
          printf("switch lcd color interface mode to 0x%x\n",  color_interface );
          break;
        default:
          printf("unsupported LCD memwrite while command = 0x%02x (wrote 0x%02x)\n", command, byte );
        }
      commandseq++;
    }
}


// render a 128x128 version of the fb
unsigned short * LCDPeripheral::getRenderedFB()
{
  const int FBW = 128;
  const int FBH = 128;

  const int offsetX = 4;  // device shift, our tomtec displays the bottom-right 128x128 corner of the 132x132 buffer
  const int offsetY = 4; 

  unsigned short * b = (unsigned short*)malloc( 128*128*2 );
  unsigned short * wfb = (unsigned short*)framebufferBYTE;
  int x,y;
  

  // contrary to popular belief, the Top Fixed Area is located at the bottom of the screen
  for( y=0; y<tfa; y++)
    {
      memcpy( b + (127-y)*FBW, wfb + offsetX + WIDTH*(127-y+offsetY), 128 * 2 );
    }

  // the bfa is at the top of the screen
  for( y=0; y<bfa; y++)
    {
      memcpy( b + y*FBW, wfb + offsetX + WIDTH*(y), 128 * 2 );
    }

  //  printf("%d %d %d\n", tfa, vsa, bfa );
  // now for the scrolling middle part

  int scrollheight = 132 - tfa - bfa;
  
  // this anticipates a bug in the lcd controller that causes some memory
  // wraparounds, it is closely linked to the reason to why the if in the
  // innerloop isn't a while
  int capped_scroll_offset = scroll_offset;
  if ( 132-tfa+offsetY <= scroll_offset )
    {
      capped_scroll_offset = 132-tfa+offsetY-1;
    }

  for( y=0; y<128 - tfa - bfa ; y++)
    {
      int fromY = y - capped_scroll_offset;
      if (fromY < 0)
        fromY += scrollheight;
      fromY += offsetY;
      fromY = fromY % scrollheight;
      fromY += bfa;
      
      memcpy( b + (y+bfa)*FBW, wfb + offsetX + WIDTH*(fromY), 128 * 2 );
    }

  return b;
}
