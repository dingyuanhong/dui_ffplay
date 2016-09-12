#ifndef MSDL_H
#define MSDL_H

#ifdef __cplusplus
extern "C" {
#endif

#define _SDL_main_h
#include "sdl/sdl.h"
#include "SDL/SDL_thread.h"

#ifdef __cplusplus
}
#endif

#pragma comment(lib,"SDL.lib")
//#pragma comment(lib,"SDLmain.lib")
#include <Windows.h>

int SDL_INIT();
void SDL_BindWindow(HWND h);
int SDLGetMaxVolume();

#endif