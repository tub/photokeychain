#include "SDLDisplay.h"

#include <cstdlib> // For some useful functions such as atexit :)
#include <SDL/SDL.h> // main SDL header
 
const int SCREEN_WIDTH = 128*3+64;
const int  SCREEN_HEIGHT = 128*3+64;

const int LCD_WIDTH = 128;
const int LCD_HEIGHT = 128;

SDLDisplay::SDLDisplay()
{ 

  InitVideo( SDL_DOUBLEBUF );
}

unsigned short bswap( unsigned short v )
{  

  return (v<<8) | (v>>8);
}

void SDLDisplay::updateImage( unsigned short * data )
{

  unsigned short *scaleddata = (unsigned short*)malloc( LCD_WIDTH*LCD_HEIGHT*3*3*2 );
  const unsigned short subpixelmask[3] = { 0xf800, 0x07e0, 0x001f };
  for( int y=0;y<LCD_HEIGHT;y++)
    for( int x=0;x<LCD_WIDTH;x++)
    {
      for( int py=0;py<3;py++)
        for( int px=0;px<3;px++)
          {
            unsigned short col = bswap(data[x+y*LCD_WIDTH]);
//             if (py!=1)
//               col &= subpixelmask[px];
            scaleddata[ (x*3+px) + LCD_WIDTH*3*(y*3+py) ] = col;
          }
    }

  SDL_Surface * surf = SDL_CreateRGBSurfaceFrom( (void*) scaleddata, LCD_WIDTH*3, LCD_HEIGHT*3, 16, LCD_WIDTH * 3 * 2, 0xf800, 0x07e0, 0x001f, 0x0 );

  
  SDL_Rect border;
  border.x = 0;
  border.y = 0;
  border.w = SCREEN_WIDTH;
  border.h = SCREEN_HEIGHT;
  SDL_FillRect( screen,  &border, 0xffff );

  SDL_Rect lcdrect;
  lcdrect.x = 32;
  lcdrect.y = 32;
  lcdrect.w = LCD_WIDTH*3;
  lcdrect.h = LCD_HEIGHT*3;
  SDL_BlitSurface( surf, NULL, screen, &lcdrect );
  SDL_FreeSurface( surf );
  SDL_Flip(screen); 

  free(scaleddata);
}

int SDLDisplay::InitVideo(Uint32 flags) 
{
  // Load SDL
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
    return false;
  }
  atexit(SDL_Quit); // Clean it up nicely :)
  
  // fullscreen can be toggled at run time :) any you might want to change the flags with params?
  //set the main screen to SCREEN_WIDTHxSCREEN_HEIGHT with a colour depth of 16:
  screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16, flags);
  if (!screen) {
    fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
    return false;
  }
  return true;
}
