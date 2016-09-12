#ifndef SDLAUDIOOPEN_H
#define SDLAUDIOOPEN_H

/*
功能:打开SDL设备函数
*/


#include "MSDL.h"
#include "AudioDef.h"


typedef void (SDLCALL *PSDL_Audio_callback)(void *userdata, Uint8 *stream, int len);

int SDL_Audio_Open(PSDL_Audio_callback cb,void *userdata, 
	int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, 
	struct AudioParams *audio_hw_params);

#endif