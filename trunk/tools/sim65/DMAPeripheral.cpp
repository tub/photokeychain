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

#include "DMAPeripheral.h"


ap_reader_t DMAPeripheral::GetReader (unsigned int idx)
{
  assert(idx < 1);

  ap_reader_t reader;
  reader.ap = this;
  reader.read = (ap_rfunc_t)&DMAPeripheral::Read;

  return reader;
}

ap_writer_t DMAPeripheral::GetWriter (unsigned int idx)
{
  assert(idx < 1);

  ap_writer_t writer;
  writer.ap = this;
  writer.write = (ap_wfunc_t)&DMAPeripheral::Write;

  return writer;
}

unsigned char DMAPeripheral::Read (unsigned short addr)
{
  printf("Invalid dma controller read from address 0x%04x\n", addr );
  return 0xff;
}

void DMAPeripheral::dmaCopy( unsigned short saddr, unsigned short daddr, unsigned short len )
{
  printf("DMA copy %d bytes from %04x to %04x\n", len,saddr, daddr );
  // this is instantaneous for now.
  while(len--)
    {
      unsigned char b;
      b = READ( saddr );
      saddr++;
      WRITE( daddr, b );
      daddr++;
    }
}

void DMAPeripheral::Write (unsigned short addr, unsigned char byte)
{
  if (debug)
    printf("DMA controller write to address 0x%04x (value 0x%02x)\n", addr, byte );

  switch(addr)
    {
    case AD_DPTRL:
      dptr = (dptr & 0xff00) | byte;
      if (debug)
        printf("[DMA] DestPtr low update to 0x%04x\n", dptr );
      break;
    case AD_DPTRH:
      dptr = (dptr & 0xff) | (byte<<8);
      if (debug)
        printf("[DMA]  DestPtr high update to 0x%04x\n", dptr );
      break;

    case AD_SPTRL:
      sptr = (sptr & 0xff00) | byte;
      if (debug)
        printf("[DMA] SrcPtr low update to 0x%04x\n", sptr );
      break;
    case AD_SPTRH:
      sptr = (sptr & 0xff) | (byte<<8);
      if (debug)
        printf("[DMA] SrcPtr high update to 0x%04x\n", sptr );
      break;

    case AD_DCNTL:
      dcnt = (dcnt &  0xff00) | byte;
      if (debug)
        printf("[DMA] DmaCnt low update to 0x%04x\n", dcnt );
      // as soon as dcntl has been written, copy dcnt+1 bytes from sptr to dptr
      dmaCopy( sptr, dptr, dcnt+1 );
      break;
    case AD_DCNTH:
      dcnt = (dcnt & 0xff) | (byte<<8);
      if (debug)
        printf("[DMA] DmaCnt high update to 0x%04x\n", dcnt );
      break;
    default:
      printf("[DMA] unsupported dma controller write addr=0x%x, value=0x%x\n", addr, byte );
      break;
    }
}
