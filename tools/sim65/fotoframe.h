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

#ifndef _FOTOFRAME_H
#define _FOTOFRAME_H

#include "Core65c02.h"
#include "disasm.h"
#include "Memory.h"
#include "AddressPeripheral.h"
#include "LCDPeripheral.h"
#include "ButtonPeripheral.h"
#include "TrapPeripheral.h"
#include "BreakpointManager.h"
#include "TimerPeripheral.h"
#include "DMAPeripheral.h"
#include "instructions.h"
#include "SDLDisplay.h"

#include "BankedMemory.h"

class FotoFrame
{
 public:
  const unsigned long long clockspeed;

  FotoFrame();

  void loadAdditionalRoms( char * ramfile, char * romfile );

private:
  void initDeviceRam();

  BreakpointManager* bpm;

  TrapPeripheral* trap;
  NullPeripheral* nul;
  Memory* mem;
  InterruptBankedMemory* bank0x4000;
  BankedMemory* bank0x4000i;
  BankedMemory* bank0x4000n;
  BankedMemory* bank0x8000;


 public:
  DMAPeripheral* dma;
  LCDPeripheral* lcd;
  ButtonPeripheral* buttons;
  Core65c02 cpu;
  SDLDisplay display;
  FlashMemory* flash;
  TimerPeripheral* timer;
};

#endif
