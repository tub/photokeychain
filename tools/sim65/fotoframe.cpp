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

#include <stdio.h>
#include <stdlib.h>
#include "fotoframe.h"

double gettimeofdayf() // get current time in seconds (floating point)
{
  int millisecs = SDL_GetTicks();
  return millisecs / 1000.0;
}

FotoFrame::FotoFrame() :  clockspeed(  6000000 )
{

  cpu.SetFrequency( clockspeed );

  mem = new Memory();
  nul = new NullPeripheral();
  trap = new TrapPeripheral();
  lcd = new LCDPeripheral();
  buttons = new ButtonPeripheral();
  bpm = new BreakpointManager();
  flash = new FlashMemory();
  timer = new TimerPeripheral();
  dma = new DMAPeripheral();

  bank0x4000i = new BankedMemory( 0, lcd, flash );
  bank0x8000 = new BankedMemory( 1, lcd, flash );
  bank0x4000n = new BankedMemory( 2, lcd, flash );

  bank0x4000 = new InterruptBankedMemory( bank0x4000n, bank0x4000i );

  mem->init_ram_memory_read( 0x0, 0xffff, 0x0 );
  mem->init_ram_memory_write( 0x0, 0xffff, 0x0 );

  initDeviceRam();

  cpu.SetBreakpointManager(bpm);
  //  bpm->AddBreakpoint( Breakpoint( (unsigned short)0x7f, 0 ) );
  mem->SetRunning(true);

}

void FotoFrame::loadAdditionalRoms( char * ramfile, char * romfile )
{
  printf("loading bootcode...\n");
  
  if (ramfile)
    {
      printf("loading ramfile %s\n", ramfile );
      FILE * fp = fopen( ramfile, "rb");
      char buf[65536];
      const int baseADDR = 0x0400;
      int br = fread( buf, 1, sizeof buf, fp );
      for(int x=0;x<br;x++)
        {
          mem->Write( x+baseADDR, buf[x] );
        }

      emPC = baseADDR;
    }


  if (romfile)
    {
      flash->loadROM( romfile, 0x10000, 2*1024*1024-0x10000 ); // load at 0x10000
//       printf("loading romfile %s\n", romfile );
//       FILE * fp = fopen( romfile, "rb");
//       char buf[65536];
//       const int baseADDR = 0x4000;
//       int br = fread( buf, 1, sizeof buf, fp );
//       for(int x=0;x<br;x++)
//         {
//           flash->Write( x+baseADDR, buf[x] );
//         }
    }

}


void FotoFrame::initDeviceRam()
{
  mem->init_ram_memory_read(0, 0xFFFF, 0);
  mem->init_ram_memory_write(0, 0xFFFF, 0);

  // initialize all of address space to null peripheral initially
  ap_reader_t nul_reader = trap->GetReader(0);
  ap_writer_t nul_writer = trap->GetWriter(0);

  for (int idx = 0x0000; idx <= 0xFFFF; ++idx)
    {
      ap_reader[idx] = nul_reader;
      ap_writer[idx] = nul_writer;
    }

  // memory (ram bank between 0x00 and 0x800)
  ap_reader_t mem_reader = mem->GetReader(0);
  ap_writer_t mem_writer = mem->GetWriter(0);

  // this 0x80 is a broad assumption. We have many unmapped io-style
  // registers in the first 128 bytes of memory, and in this way we can
  // trap any access to them easily.
  for (int idx = 0x80; idx <= 0x900; ++idx) // i'm not entirely sure about this range!
    {
      ap_reader[idx] = mem_reader;
      ap_writer[idx] = mem_writer;
    }
  
  // banked memory
  ap_reader_t flash0_reader = bank0x4000->GetReader(0);
  ap_writer_t flash0_writer = bank0x4000->GetWriter(0);

  for (int idx = 0x4000; idx < 0x8000; ++idx)
    {
      ap_reader[idx] = flash0_reader;
      ap_writer[idx] = flash0_writer;
    }

  // set up banking control registers

  ap_writer[ bank0x4000i->getControlAddress(0) ] = flash0_writer;
  ap_reader[ bank0x4000i->getControlAddress(0) ] = flash0_reader;
  ap_writer[ bank0x4000i->getControlAddress(1) ] = flash0_writer;
  ap_reader[ bank0x4000i->getControlAddress(1) ] = flash0_reader;
  ap_writer[ bank0x4000n->getControlAddress(0) ] = flash0_writer;
  ap_reader[ bank0x4000n->getControlAddress(0) ] = flash0_reader;
  ap_writer[ bank0x4000n->getControlAddress(1) ] = flash0_writer;
  ap_reader[ bank0x4000n->getControlAddress(1) ] = flash0_reader;

  ap_reader_t flash1_reader = bank0x8000->GetReader(0);
  ap_writer_t flash1_writer = bank0x8000->GetWriter(0);

  for (int idx = 0x8000; idx < 0x10000; ++idx)
    {
      ap_reader[idx] = flash1_reader;
      ap_writer[idx] = flash1_writer;
    }

  ap_writer[ bank0x8000->getControlAddress(0) ] = flash1_writer;
  ap_reader[ bank0x8000->getControlAddress(0) ] = flash1_reader;
  ap_writer[ bank0x8000->getControlAddress(1) ] = flash1_writer;
  ap_reader[ bank0x8000->getControlAddress(1) ] = flash1_reader;

  ap_reader[0x0] = buttons->GetReader(0);

  // these are no longer needed, they are handled by the banked flash controler
  //  ap_writer[0xC000] = lcd->GetWriter(0);
  //  ap_writer[0x8000] = lcd->GetWriter(0);

  // add timer port
  ap_reader[0x29] = timer->GetReader(0);

  // add dma ports
  ap_writer[AD_DPTRL] = dma->GetWriter(0);
  ap_writer[AD_DPTRH] = dma->GetWriter(0);
  ap_writer[AD_SPTRL] = dma->GetWriter(0);
  ap_writer[AD_SPTRH] = dma->GetWriter(0);
  ap_writer[AD_DCNTL] = dma->GetWriter(0);
  ap_writer[AD_DCNTH] = dma->GetWriter(0);
  ap_writer[AD_DCTR] = dma->GetWriter(0);
  ap_writer[AD_DMOD] = dma->GetWriter(0);
}

void dumpCpuState()
{
  char buf[1024];
  static long long prev_cycle_clock = 0;
  long long diff_cycle_clock = cycle_clock - prev_cycle_clock;
  prev_cycle_clock = cycle_clock;
  printf("cpu state: Cycle=%Ld (%+Ld) A=%02x X=%02x Y=%02x S=%02x P=%02x PC=%04x ii=%x\n", cycle_clock, diff_cycle_clock, A, X, Y, S, P, emPC, inside_interrupt );
  disasm (buf, emPC, 10, 0);
  printf("%04x %s\n", emPC, buf);
}

int main( int argc, char** argv)
{

  char * ramfile = "bootcode.bin";
  char * romfile = 0;
  if (argc==3)
    {
      ramfile = argv[1];
      romfile = argv[2];

    }

  inside_interrupt = 1; // when we use ramfile, assume that we start executing directly from the usb interrupt handler, so inside_interrupt is on
  
  FotoFrame ff;
  ff.flash->loadFirmware("firmware.dump","memory.dump");
  ff.loadAdditionalRoms( ramfile, romfile );

  printf("Initial cpu state: A=%02x X=%02x Y=%02x S=%02x P=%02x PC=%04x ii=%x\n", A, X, Y, S, P, emPC, inside_interrupt );

  int cnt=0; // instruction counter
  bool trace=false;

  if (getenv( "SIM_TRACE" ))
    trace = true;
  bool running=true;
  bool dumpframes=false;
  bool quitting=false;
  long long int fps = 60;

  double startuptime = gettimeofdayf();
  double lastframetime = gettimeofdayf();
  double endtime;

  while (!quitting)
    {
      // this is a bit of delay code, that checks how much time we have
      // used, and wait accordingly to make sure this whole loop is
      // executed 60 times per second.

      double now = gettimeofdayf();
      double frametime = 1.0 / float(fps); // seconds that a single frame should take to emulate
      double sparetime = frametime - (now-lastframetime); // time that we have to spare
      if (sparetime>0.0)
        {
          SDL_Delay( int(1000.0 * sparetime) );
          // usleep( 1000000.0 * sparetime );
        }
      lastframetime = gettimeofdayf();

      // if the cpu is running, we execute a chunk of cycles that is
      // roughly the same as we would expect for a single video frame (at
      // a rate of fps)
      if (running)
        {
          unsigned long long startcycle = cycle_clock;
          unsigned long long cycles_per_frame = ff.clockspeed / fps;
          unsigned long long endcycle = startcycle + cycles_per_frame;
          while (running && cycle_clock < endcycle )
            {
              if (trace)
                {

                  dumpCpuState();
                }
              bool status = ff.cpu.Execute(1);
              if (status)
                {
                  SDL_WM_SetCaption("Stopped - hit breakpoint", NULL );
                  printf("--- Hit Breakpoint! ---\n");
                  dumpCpuState();
                  running = false;
                }
              if (emPC==0x1) // a plain rts
                {
                  SDL_WM_SetCaption("Stopped - hit RTS (end program)", NULL );
                  printf("encountered RTS, terminating\n");
                  running = false;
                }
            }

          // display update every 1/fps frames
          unsigned short * fb = ff.lcd->getRenderedFB();
          ff.display.updateImage( fb );
          free(fb);

          if (dumpframes)
          {
          char buf[100];
          static int framecnt = 0;
          sprintf(buf,"frame-%05d.ppm", framecnt++ );
          ff.lcd->dumpPPM( buf );
          }

          char status[100];
          sprintf( status, "Emulating. %Ld cycles, %Ld instructions executed",       cycle_clock  ,ff.cpu.GetNumExecuted() );
          if (running)
            SDL_WM_SetCaption( status , NULL );
        }


      
      if (!running)
        {
          SDL_WaitEvent( NULL ); // save cpu cycles by waiting for an event instead of polling when we're the emulated cpu isn't running
        }
      
        {

          SDL_Event ev;
          int evcnt = SDL_PollEvent( &ev );
          if (evcnt > 0)
            {
              if (ev.type == SDL_QUIT)
                quitting = true;
              if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP)
                {
                  int sym = ((SDL_KeyboardEvent*)(&ev))->keysym.sym;
                  int pressed = ev.type == SDL_KEYDOWN;
                  int button = -1;
                  switch( sym )
                    {
                    case 'z':
                    case 'h':
                      button = 0;
                      break;
                    case 'x':
                    case 't':
                      button = 1;
                      break;
                    case 'c':
                    case 'n':
                      button = 2;
                      break;
                    }
                  if (button != -1 )
                    ff.buttons->keyEvent( button, pressed );

                }
              if (ev.type == SDL_KEYUP)
                {
                  unsigned int ks =  ((SDL_KeyboardEvent*)(&ev))->keysym.sym;
                  if (ks == SDLK_ESCAPE)
                    quitting = true;
                  else if (ks == SDLK_SPACE)
                    {
                      printf("continue..\n");
                      running = true;
                    }
                  else if (ks == 'm')
                    {
                      if (trace) {
                        printf("disable code monitor\n");
                      } else {
                        printf("enable code monitor\n");
                      }
                      trace = !trace;
                    }
                }
            }
          
          if (quitting)
            running = false;
        }

        if (!running)
          endtime = gettimeofdayf();

    }
  double exectime = endtime-startuptime;
  printf("time spend : %f\n", exectime );
  printf("executed %Ld cycles, %Ld instructions, at %5.5f Mcycles/second\n", cycle_clock, ff.cpu.GetNumExecuted(), (cycle_clock/1000000.0) / exectime );

  {
    // sorry about this code
    unsigned short * fb = ff.lcd->getRenderedFB();
    ff.display.updateImage( fb );
    free(fb);
  }
  ff.lcd->dumpPPM( "lcd.ppm");

  return 0;
}
