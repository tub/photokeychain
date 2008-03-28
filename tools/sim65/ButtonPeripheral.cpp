#include "ButtonPeripheral.h"

#include "instructions.h"

ButtonPeripheral::ButtonPeripheral()
{
  buttonstate=0;
}

ap_reader_t ButtonPeripheral::GetReader (unsigned int idx)
{
  assert(idx < 1);

  ap_reader_t reader;
  reader.ap = this;
  reader.read = (ap_rfunc_t)&ButtonPeripheral::Read;

  return reader;
}

ap_writer_t ButtonPeripheral::GetWriter (unsigned int idx)
{
  assert(idx < 1);

  ap_writer_t writer;
  writer.ap = this;
  writer.write = (ap_wfunc_t)&ButtonPeripheral::Write;

  return writer;
}

unsigned char ButtonPeripheral::Read (unsigned short addr)
{
  // the other pins are default 7f, don't know for sure what these encode
  return 0x7f ^ buttonstate;
}

void ButtonPeripheral::Write (unsigned short addr, unsigned char byte)
{
  return;  // we do not support gpio output lines on this pad
}

void ButtonPeripheral::keyEvent( unsigned char button, unsigned char pressed )
{
  // pressed = 1 means pressed
  // pressed = 0 means depressed
  // button is in the range [0,2]
  
  buttonstate &= ~(1<<button);
  if (pressed)
    buttonstate |= (1<<button);

  interrupt_flags |= F_PAD_A_IRQ;
}




