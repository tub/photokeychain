#ifndef _DMAPERIPHERAL_H
#define _DMAPERIPHERAL_H

#include "AddressPeripheral.h"

// firmware disassembly seems to suggest these dma addresses work
// differently from what we've seen in the st2205u docs

  const unsigned char AD_SPTRL = 0x58;
  const unsigned char AD_SPTRH = 0x59;
  const unsigned char AD_DPTRL = 0x5a;
  const unsigned char AD_DPTRH = 0x5b;
  const unsigned char AD_DCNTL = 0x5c;
  const unsigned char AD_DCNTH = 0x5d;
  const unsigned char AD_DCTR = 0x5e;
  const unsigned char AD_DMOD = 0x5f;


// a peripheral that will warn/dma when it's bytes are read/written

class DMAPeripheral: public AddressPeripheral
{
public:
  DMAPeripheral()
    {
      sptr = 0;
      dptr = 0;
      dctr = 0;
      dmod = 0;
      dcnt = 0;
      debug = 0;
    };


  unsigned int NumReaders () { return 1; }
  unsigned int NumWriters () { return 1; }

  ap_reader_t GetReader (unsigned int);
  ap_writer_t GetWriter (unsigned int);

  unsigned char Read (unsigned short);
  void Write (unsigned short, unsigned char);
 private:

  void dmaCopy( unsigned short saddr, unsigned short daddr, unsigned short len );

  unsigned short sptr, dptr, dcnt;
  unsigned char dctr, dmod;

  unsigned char debug;

};


#endif
