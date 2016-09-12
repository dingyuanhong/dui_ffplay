#ifndef VIDEOPICTUREREFRESH_H
#define VIDEOPICTUREREFRESH_H

/*
功能:屏幕区域刷新函数
*/

#include "ffmpeg.h"
#include "MSDL.h"
#include "VideoState.h"
#include "ScreenRender.h"
#include "VideoProcesser.h"

ScreenRender *CreateScreenRender(void *Render,RenderFunc func);
void InitScreenRender(ScreenRender * is,void *Render,RenderFunc func);
void FreeScreenRender(ScreenRender **pis);

//刷新显示区域
void video_refresh(VideoProcesser *vp,ScreenRender *sr,
	SubtitleBuffer *sb,	//字幕缓冲区
	double *remaining_time);

void toggle_pause(VideoState *is,VideoSynchronize *vs,SynchronizeClockState *scs);
void step_to_next_frame(VideoState *is,VideoSynchronize *vs,SynchronizeClockState *scs);

#endif
