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

#include "TrapPeripheral.h"

ap_reader_t TrapPeripheral::GetReader (unsigned int idx)
{
  assert(idx < 1);

  ap_reader_t reader;
  reader.ap = this;
  reader.read = (ap_rfunc_t)&TrapPeripheral::Read;

  return reader;
}

ap_writer_t TrapPeripheral::GetWriter (unsigned int idx)
{
  assert(idx < 1);

  ap_writer_t writer;
  writer.ap = this;
  writer.write = (ap_wfunc_t)&TrapPeripheral::Write;

  return writer;
}

char * TrapPeripheral::getAddressName( unsigned short addr )
{
  switch (addr)
    {
    case 0:
      return "PADA";
    case 1:
      return "PADB";
    case 2:
      return "PADC";
    case 3:
      return "PADD";
    case 0x6:
      return "PSC";
    case 0x8:
      return "PCA";
    case 0x9:
      return "PCB";
    case 0xa:
      return "PCC";
    case 0xb:
      return "PCD";
    case 0xc:
      return "PFA";
    case 0xd:
      return "PFB";
    case 0xe:
      return "PFC";
    case 0xf:
      return "PFD";
    case 0x22:
      return "T1CL";
    case 0x23:
      return "T1CH";
    case 0x24:
      return "T2CL";
    case 0x25:
      return "T2CH";
    case 0x26:
      return "T3CL";
    case 0x27:
      return "T3CH";
    case 0x28:
      return "TIEN";
    case 0x29:
      return "PRS";
    case 0x2a:
      return "BTEN";
    case 0x2b:
      return "BTREQ";
    case 0x2e:
      return "RCTR";
    case 0x2f:
      return "RTC";

    case 0x30:
      return "IRRL";
    case 0x31:
      return "IRRH";
    case 0x38:
      return "MISC";
    case 0x39:
      return "SYS";
    case 0x3a:
      return "PMCR";
    case 0x3b:
      return "XREQ";
    case 0x3e:
      return "IENAL";
    case 0x3f:
      return "IENAH";
    case 0x3c:
      return "IREQL";
    case 0x3d:
      return "IREQH";

    case 0x4e:
      return "PCL";
    case 0x4f:
      return "PCL";

    case 0x6e:
      return "MULL?";
    case 0x6f:
      return "MULH?";

      // these i changed after looking at firmware bootup logs
    case 0x58:
      return "SPTRL";
    case 0x59:
      return "SPTRH";
    case 0x5a:
      return "DPTRL";
    case 0x5b:
      return "DPTRH";
    case 0x5c:
      return "DCNTL";
    case 0x5d:
      return "DCNTH";
    case 0x5e:
      return "DCTR";
    case 0x5f:
      return "DMOD";
    }

  return NULL;

}

unsigned char TrapPeripheral::Read (unsigned short addr)
{
  char* name = getAddressName( addr );
  if (name)
    printf("Invalid memory read from address 0x%04x [%s]\n", addr, name );
  else
    printf("Invalid memory read from address 0x%04x\n", addr );
  return 0x00;
}

void TrapPeripheral::Write (unsigned short addr, unsigned char byte)
{
  char* name = getAddressName( addr );
  if (name)
    printf("Invalid memory write to address 0x%04x [%s] (value 0x%02x)\n", addr, name, byte );
  else
    printf("Invalid memory write to address 0x%04x (value 0x%02x)\n", addr, byte );

}
