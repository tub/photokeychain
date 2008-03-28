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

#include "FlashMemory.h"
#include <stdio.h>
#include <string.h>

FlashMemory::FlashMemory()
{
  memset( storage, 0, sizeof(storage) );
};

void FlashMemory::loadFirmware( char* fwimage, char* flashimage )
{
  loadROM( fwimage, 0, 65536 );
  loadROM( flashimage, 65536, 2*1024*1024-65536 );
};

void FlashMemory::loadROM( char * filename, unsigned int offset, unsigned int size )
{
  printf("loading ROM '%s' size %04x at offset %08x\n", filename, size, offset );
  FILE * fp = fopen(filename, "rb");
  if (!fp)
    {
      printf("error opening %s\n", filename );
      perror("failed to find file");
    }

  int br = fread( storage + offset , 1, size, fp );
  if (br != size)
    perror("failed to load firmware.dump");
  fclose(fp);
};

unsigned char FlashMemory::Read( int laddr )
{
  return storage[laddr];
};
