#ifndef _TIMERPERIPHERAL
#define _TIMERPERIPHERAL

#include "AddressPeripheral.h"

class TimerPeripheral: public AddressPeripheral
{
public:
  TimerPeripheral();
  unsigned int NumReaders () { return 1; }
  unsigned int NumWriters () { return 1; }

  ap_reader_t GetReader (unsigned int);
  ap_writer_t GetWriter (unsigned int);

  unsigned char Read (unsigned short);
  void Write (unsigned short, unsigned char);
private:
};


#endif
