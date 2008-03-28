#ifndef _MEMORY_H
#define _MEMORY_H

#include "AddressPeripheral.h"
#include "LCDPeripheral.h"
#include "FlashMemory.h"

// This emulates a banked memory class, it has a single control register
// which determines which bank is currently accessible.

class BankedMemory : AddressPeripheral
{
 public:
  BankedMemory(unsigned char memtype, LCDPeripheral* lcdcontroler, FlashMemory * flashmemory);

  virtual unsigned char Read( unsigned short addr );
  virtual void Write( unsigned short addr, unsigned char byte );

  unsigned int NumReaders () { return 1; }
  unsigned int NumWriters () { return 1; }

  ap_reader_t GetReader (unsigned int) 
  { 
    ap_reader_t reader;
    reader.ap = this;
    reader.read = (ap_rfunc_t)&BankedMemory::Read;
    
    return reader;
  }

  ap_writer_t GetWriter (unsigned int)
  {
    ap_writer_t writer;
    writer.ap = this;
    writer.write = (ap_wfunc_t)&BankedMemory::Write;
    
    return writer;
  }

  unsigned short getControlAddress( unsigned char lo )
  {
    if (lo)
      return controlAddressLo;
    else
      return controlAddressHi;
  }

  unsigned short controlAddressHi, controlAddressLo;

protected:
  void initFlash();
  unsigned char flashRead( int Laddr );

  unsigned char memType; // hardcoded. 0 means this is the 0x4000 bank type memory, 1 means it is the 0x8000 type
  unsigned char bankSelectHi, bankSelectLo;
  unsigned short startAddress, endAddress;
  LCDPeripheral * lcdcontroller;
  FlashMemory* flashMemory;
};

class InterruptBankedMemory : public AddressPeripheral
{
 public:
  
  InterruptBankedMemory( BankedMemory* nbank, BankedMemory* ibank );

  unsigned int NumReaders () { return 1; }
  unsigned int NumWriters () { return 1; }

  ap_reader_t GetReader (unsigned int) 
  { 
    ap_reader_t reader;
    reader.ap = this;
    reader.read = (ap_rfunc_t)&BankedMemory::Read;
    
    return reader;
  }

  ap_writer_t GetWriter (unsigned int)
  {
    ap_writer_t writer;
    writer.ap = this;
    writer.write = (ap_wfunc_t)&BankedMemory::Write;
    
    return writer;
  }


  virtual unsigned char Read( unsigned short addr );
  virtual void Write( unsigned short addr, unsigned char byte );

 private:
  unsigned short contrallAddressHi2, contrallAddressLo2;
  BankedMemory* ibank;
  BankedMemory* nbank;
};

#endif
