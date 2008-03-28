#
#
#

CC = g++
CXX = g++

CFLAGS = -O2 -fomit-frame-pointer -g3 -ggdb
CXXFLAGS = -O2 -fomit-frame-pointer   -g3 -ggdb

LIB = -lSDL

CXXSRC = sim65_instructions.cpp disasm.cpp tables.cpp srecord.cpp ByteRegister.cpp WordRegister.cpp Memory.cpp  \
	 Core65c02.cpp Breakpoint.cpp BreakpointManager.cpp LCDPeripheral.cpp ButtonPeripheral.cpp  TrapPeripheral.cpp TimerPeripheral.cpp \
	 AddressPeripheral.cpp Uart.cpp Acia6551.cpp fotoframe.cpp SDLDisplay.cpp BankedMemory.cpp FlashMemory.cpp  DMAPeripheral.cpp

COBJ =  sim65_instructions.o disasm.o tables.o srecord.o
CXXOBJ = ByteRegister.o WordRegister.o Memory.o \
	 Core65c02.o Breakpoint.o BreakpointManager.o LCDPeripheral.o ButtonPeripheral.o TrapPeripheral.o \
	 AddressPeripheral.o Uart.o Acia6551.o fotoframe.o SDLDisplay.o BankedMemory.o FlashMemory.o TimerPeripheral.o  DMAPeripheral.o

default: sim65

objs: $(COBJ) $(CXXOBJ)

sim65: $(COBJ) $(CXXOBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(COBJ) $(CXXOBJ) -o sim65 $(LIB)

clean:
	rm -f *.o
	rm -f sim65

distclean: clean
	rm -f *~
	rm -f sim65.depend

sim65.depend:
	$(CXX) -MM -MG  $(CXXSRC) >> sim65.depend

include sim65.depend
