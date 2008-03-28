
#ifndef _FLASHMEMORY_H
#define _FLASHMEMORY_H

// this is internaly used by the bankedmemory as a backend for storing the
// full flash rom

class FlashMemory
{
 public:
  FlashMemory();

  void loadFirmware( char* fwimage, char* flashimage );
  void loadROM( char * filename, unsigned int offset, unsigned int size );

  unsigned char Read( int laddr );
  unsigned char storage[2*1024*1024];

};

#endif
