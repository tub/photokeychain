#ifndef _LCDPERIPHERAL
#define _LCDPERIPHERAL

#include "AddressPeripheral.h"

const int WIDTH = 132;
const int HEIGHT = 132;

class LCDPeripheral: public AddressPeripheral
{
public:
  LCDPeripheral();
  unsigned int NumReaders () { return 1; }
  unsigned int NumWriters () { return 1; }

  ap_reader_t GetReader (unsigned int);
  ap_writer_t GetWriter (unsigned int);

  unsigned char Read (unsigned short);
  void Write (unsigned short, unsigned char);

  void dumpPPM( char* filename );
  unsigned short * getFrameBufferPointer() { return (unsigned short*)framebufferBYTE; };

  unsigned short * getRenderedFB();
 private:
  unsigned char command;
  unsigned int commandseq;
  unsigned char framebufferBYTE[WIDTH * HEIGHT * 2];
  
 
  void setPixel( unsigned int x, unsigned int y, unsigned char lo, unsigned char hi );

  int write_x;
  int write_y;

  // CASET: Column Address Set (2Ah)
  int x_adr_start;
  int x_adr_end;

  // RASET: Row Address Set (2Bh)
  int y_adr_start;
  int y_adr_end;

  // MADCTR
  unsigned char madctr_y; // 0 = regular row increments, 1 = mirror row increments
  unsigned char madctr_x; // 0 = regular column increments, 1 = mirror column increments
  unsigned char madctr_rowcol; // 0 = regular rows/columns order, 1 = invert rows/columns

  // Area set 0x33
  unsigned char tfa;  // top fixed area
  unsigned char vsa;  // verical scrolling area
  unsigned char bfa;  // bottom fixed area

  
  unsigned char scroll_offset;
  
  // color interface mode 0x3a
  unsigned char color_interface;

  void nextWritePixel();
  void map8bitRGB( unsigned char byte, unsigned char* _lo, unsigned char* _hi);
  void map12bitRGB( unsigned short rgbword, unsigned char* _lo, unsigned char* _hi);
  void getXYAfterMadctr( int& x, int& y );
  unsigned char colorlookup[48];
  unsigned char pixwritebuffer[3];
  
};


#endif
