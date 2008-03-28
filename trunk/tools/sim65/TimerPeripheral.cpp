#include "TimerPeripheral.h"

#include "instructions.h"

TimerPeripheral::TimerPeripheral()
{
}

ap_reader_t TimerPeripheral::GetReader (unsigned int idx)
{
  assert(idx < 1);

  ap_reader_t reader;
  reader.ap = this;
  reader.read = (ap_rfunc_t)&TimerPeripheral::Read;

  return reader;
}

ap_writer_t TimerPeripheral::GetWriter (unsigned int idx)
{
  assert(idx < 1);

  ap_writer_t writer;
  writer.ap = this;
  writer.write = (ap_wfunc_t)&TimerPeripheral::Write;

  return writer;
}

unsigned char TimerPeripheral::Read (unsigned short addr)
{
  return cycle_clock;
}

void TimerPeripheral::Write (unsigned short addr, unsigned char byte)
{
  return;  // we do not support gpio output lines on this pad
}

