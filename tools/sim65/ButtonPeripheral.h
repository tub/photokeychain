#ifndef _BUTTONPERIPHERAL
#define _BUTTONPERIPHERAL

#include "AddressPeripheral.h"

class ButtonPeripheral: public AddressPeripheral
{
public:
  ButtonPeripheral();
  unsigned int NumReaders () { return 1; }
  unsigned int NumWriters () { return 1; }

  ap_reader_t GetReader (unsigned int);
  ap_writer_t GetWriter (unsigned int);

  unsigned char Read (unsigned short);
  void Write (unsigned short, unsigned char);

  void keyEvent( unsigned char button, unsigned char pressed );
 private:
  unsigned char buttonstate;
};


#endif
