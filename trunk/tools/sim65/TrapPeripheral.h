#ifndef _TRAPPERIPHERAL_H
#define _TRAPPERIPHERAL_H

#include "AddressPeripheral.h"

// a peripheral that will warn/trap when it's bytes are read/written

class TrapPeripheral: public AddressPeripheral
{
public:
  unsigned int NumReaders () { return 1; }
  unsigned int NumWriters () { return 1; }

  ap_reader_t GetReader (unsigned int);
  ap_writer_t GetWriter (unsigned int);

  unsigned char Read (unsigned short);
  void Write (unsigned short, unsigned char);

  char* getAddressName( unsigned short addr );
};


#endif
