#ifndef SDLDRAWVIDEO_H
#define SDLDRAWVIDEO_H 

#include "ScreenRender.h"
#include "VideoState.h"
#include "SynchronizeClockDef.h"

//将src_frame数据保存至VideoPictureBuffer缓冲区中
int fill_picture(ScreenRender *sr,VideoPictureBuffer *vpr, AVFrame *src_frame, double pts, int64_t pos, int serial);

//绘制图像
/* display the current picture, if any */
void video_display(VideoState *is,
	ScreenRender *sr,
	SubtitleBuffer *sb,		//字幕缓冲区
	VideoPictureBuffer *vpb, //视频图像缓冲区
	SynchronizeClockState *scs);

//音频频谱绘制
void audio_sample_display(VideoState *is,
	ScreenRender *sr,
	SynchronizeClockState *scs);


void toggle_audio_display(VideoState *is,ScreenRender *sr);
//全屏切换
void toggle_full_screen(ScreenRender *is);


static RenderFunc g_sdlRender = 
{
	fill_picture,
	video_display,
	audio_sample_display
};

#include "SDLRenderParam.h"

UserScreenRender *CreateUserScreenRender(int width,int height);
void InitUserScreenRender(UserScreenRender * usr,int width,int height);
void FreeUserScreenRender(UserScreenRender** usr);
#endif