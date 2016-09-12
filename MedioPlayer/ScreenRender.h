#ifndef SCREENRENDER_H
#define SCREENRENDER_H

#include "VideoDef.h"
#include "VideoState.h"

typedef struct ScreenRender;

//填充缓冲区
typedef int (*PFill_Picture)(ScreenRender *sr,VideoPictureBuffer *vpr, AVFrame *src_frame, double pts, int64_t pos, int serial);

//视频绘制函数
typedef void (*PVideo_Display)(VideoState *is,
	ScreenRender *sr,
	SubtitleBuffer *sb,		//字幕缓冲区
	VideoPictureBuffer *vpb, //视频图像缓冲区
	SynchronizeClockState *scs);

//音频频谱绘制
typedef void (*PAudio_Sample_Display)(VideoState *is,
	ScreenRender *sr,
	SynchronizeClockState *scs);

typedef struct RenderFunc
{
	PFill_Picture fill_picture;
	PVideo_Display video_display;
	PAudio_Sample_Display audio_sample_display;
}RenderFunc;

//屏幕渲染
typedef struct ScreenRender
{
	PFill_Picture fill_picture;
	PVideo_Display video_display;
	PAudio_Sample_Display audio_sample_display;
	void *Render;			//用户渲染参数
	
	double last_vis_time;	//最后的音频样本绘制时间,0.0
	int force_refresh;		//强制刷新,0
	int display_disable;	//关闭显示,0
}ScreenRender;

#endif