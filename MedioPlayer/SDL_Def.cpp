#include "MSDL.h"

static int g_bInited = 0;

int SDL_INIT()
{
	if(g_bInited == 1) return 1;

	int flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
    //flags &= ~SDL_INIT_VIDEO;
#if !defined(__MINGW32__) && !defined(__APPLE__)
    //flags |= SDL_INIT_EVENTTHREAD; /* Not supported on Windows or Mac OS X */
#endif

	if(!SDL_WasInit(flags)){
		if (SDL_Init (flags)) {
			//av_log(NULL, AV_LOG_FATAL, "Could not initialize SDL - %s\n", SDL_GetError());
			//av_log(NULL, AV_LOG_FATAL, "(Did you set the DISPLAY variable?)\n");
			//exit(1);
			return -1;
		}
	}

	SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
    SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
    SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

	g_bInited = 1;

	return 1;
}
void SDL_BindWindow(HWND h)
{
	char tmpBuf[24] = {0};
	sprintf(tmpBuf,"SDL_WINDOWID=%d",h);
	SDL_putenv(tmpBuf);

	SDL_INIT();
}

int SDLGetMaxVolume()
{
	return 128;
}