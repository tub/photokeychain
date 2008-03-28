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

#include "instructions.h"
#include "BankedMemory.h"

BankedMemory::BankedMemory(unsigned char memtype, LCDPeripheral* lcdcontroller, FlashMemory * flashmemory) : memType( memtype ) , lcdcontroller(lcdcontroller), flashMemory( flashmemory )
{
  //  initFlash();

  bankSelectHi = 0x00;
  bankSelectLo = 0x00;
  if (memType == 0)  // interrupt bank
    {
      controlAddressHi = 0x31; // msb of bank select comes from here
      controlAddressLo = 0x30; // lsb of bank select are here
      startAddress  = 0x4000;
      endAddress    = 0x7fff;
    }
  if (memType == 1) 
    {
      controlAddressHi = 0x35;
      controlAddressLo = 0x34;
      startAddress  = 0x8000;
      endAddress    = 0xffff;
    }
  if (memType == 2) // prr bank
    {
      controlAddressHi = 0x33;
      controlAddressLo = 0x32;
      startAddress  = 0x4000;
      endAddress    = 0x7fff;
    }
}

unsigned char BankedMemory::Read( unsigned short addr )
{
  if (addr == controlAddressLo)
    return bankSelectLo;
  if (addr == controlAddressHi)
    return bankSelectHi;
  
  if (addr<startAddress || addr >endAddress)
    {
      printf("illegal read request for bankedmemory (addr %x is out of range %x..%x)\n", addr, startAddress, endAddress );
      return 0xff;
    }

  if (memType==1)
    {
      if ((bankSelectHi % 4) == 0) // flash memory
        {
          return flashRead( addr - startAddress + (bankSelectLo % 0x40) * 0x8000 );
        }
      
      if ((bankSelectHi % 4) == 3) // lcd controller
        {
          return lcdcontroller->Read( addr - startAddress ); 
        }
    } 
  else if (memType==0 || memType==2)
    {
      if ((bankSelectHi % 8) == 0) // flash memory
        {
          return flashRead( addr - startAddress + (bankSelectLo % 0x80) * 0x4000 );
        }
      
      if ((bankSelectHi % 8) == 6) // lcd controller
        {
          return lcdcontroller->Read( addr - startAddress + (bankSelectLo % 2) * 0x4000 ); 
        }
    }
  return 0xff;
};

unsigned char BankedMemory::flashRead( int Laddr )
{
  if (Laddr < 0 || Laddr > 2*1024*1024 )
    {
      printf("internal error in flashRead, addr=%x\n", Laddr );
      return 0xff;
    }
  return flashMemory->Read(Laddr);
};

void BankedMemory::Write( unsigned short addr, unsigned char byte )
{
  if (addr == controlAddressLo)
    {
      printf("memory at %04x--%04x changed bankSelectLo to %02x\n", startAddress, endAddress, byte );
      bankSelectLo = byte;
    }
  else if (addr == controlAddressHi)
    {
      printf("memory at %04x--%04x changed bankSelectHi to %02x\n", startAddress, endAddress, byte );
      bankSelectHi = byte;
    }
  else 
    {
      if (addr<startAddress || addr >endAddress)
        {
          printf("illegal write request for bankedmemory (addr %x is out of range %x..%x)\n", addr, startAddress, endAddress );
          return;
        }
      
      if (memType==1)
        {
          if ((bankSelectHi % 4) == 3)
            {
              // write to lcd controller;
              lcdcontroller->Write( addr-startAddress, byte );
            }
          else
            {
              printf("Illegal writing to flash memory at address %04x, value %02x, bankSelectHi=%x bankSelectLo=%x\n", 
                     addr, byte, bankSelectHi, bankSelectLo );
            }
        }
      else if(memType==0 || memType==2)
        {
          if ((bankSelectHi % 8) == 6)
            {
              // write to lcd controller;
              lcdcontroller->Write( addr - startAddress + (bankSelectLo % 2) * 0x4000, byte );
            }
          else
            {
              printf("Illegal writing to flash memory at address %04x, value %02x, bankSelectHi=%x bankSelectLo=%x\n", 
                     addr, byte, bankSelectHi, bankSelectLo );
            }
        }
    }
};



InterruptBankedMemory::InterruptBankedMemory( BankedMemory* nbank, BankedMemory* ibank ) : nbank(nbank), ibank(ibank)
{
  
}


unsigned char InterruptBankedMemory::Read( unsigned short addr )
{
  if (addr == nbank->controlAddressLo ||
      addr == nbank->controlAddressHi )
    return nbank->Read( addr );
  else 
    if (addr == ibank->controlAddressLo ||
        addr == ibank->controlAddressHi )
      return ibank->Read( addr );

  if (inside_interrupt)
    return ibank->Read(addr);
  else
    return nbank->Read(addr);
};

void InterruptBankedMemory::Write( unsigned short addr, unsigned char byte )
{
  if (addr == nbank->controlAddressLo ||
      addr == nbank->controlAddressHi )
    return nbank->Write( addr, byte );
  else 
    if (addr == ibank->controlAddressLo ||
        addr == ibank->controlAddressHi )
      return ibank->Write( addr, byte );

  if (inside_interrupt)
    return ibank->Write(addr, byte);
  else
    return nbank->Write(addr, byte);
  
  printf("Illegal writing to interrupt banked flash memory at address %04x, value %02x\n", 
         addr, byte );
  
};
