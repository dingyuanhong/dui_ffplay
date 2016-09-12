#ifndef SDLRENDERPARAM_H
#define SDLRENDERPARAM_H

#include "MSDL.h"
#include "ffmpeg.h"
#include "AudioDef.h"

typedef struct FFTSRenderInfor{
	RDFTContext *rdft;
	int rdft_bits;
    FFTSample *rdft_data;
	int xpos;
}FFTSRenderInfor;

typedef struct AudioSampleSpikes
{
	int i_start;
	int h;
}AudioSampleSpikes;

//音频样本渲染
typedef struct AudioSampleParams
{
	//音频相关
	struct AudioParams audio_tgt;
	FFTSRenderInfor fftsri;//音频ffts渲染模块
	AudioSampleSpikes ass; //音频渲染参数
}AudioSampleParams;


typedef struct ImageRenderInfor
{
	int flush_screen;
	int width, height, xleft, ytop;
	SDL_Surface *screen;
	SDL_Rect last_display_rect;
}ImageRenderInfor;

typedef struct UserScreenRender
{
	int fs_screen_width;
	int fs_screen_height;
	int default_width;
	int default_height;
	int screen_width;
	int screen_height;
	int is_full_screen;

	ImageRenderInfor iri;//界面渲染模块

#if !CONFIG_AVFILTER
    struct SwsContext *img_convert_ctx;
#endif

	AudioSampleParams asr;  //音频样本绘制参数
}UserScreenRender;


#endif