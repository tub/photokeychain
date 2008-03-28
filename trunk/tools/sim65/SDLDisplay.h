#ifndef SDLDISPLAY_H
#define SDLDISPLAY_H

#include <SDL/SDL.h>

class SDLDisplay
{
 public:
  SDLDisplay();
  void updateImage( unsigned short * data ); // 128x128 image fixed.

 private:
    SDL_Surface *screen; //This pointer will reference the backbuffer
    int  InitVideo(Uint32 flags) ;
};

#endif
