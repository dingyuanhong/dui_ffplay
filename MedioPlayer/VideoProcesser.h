#ifndef VIDEOPROCESSER_H
#define VIDEOPROCESSER_H

#include "ffmpeg.h"
#include "MSDL.h"
#include "VideoState.h"
#include "ScreenRender.h"
#include "SynchronizeClockDef.h"

/*
功能:处理并解码视频包至显示数据缓冲区
*/

typedef struct VideoProcesser
{
	VideoState *is;
	SynchronizeClockState *scs;
	VideoPictureBuffer *vpb;
	VideoSynchronize *vs;
}VideoProcesser;

VideoProcesser *CreateVideoProcesser(double max_frame_duration);
void InitVideoProcesser(VideoProcesser *vp,VideoState *is,SynchronizeClockState *scs);
void UninitVideoProcesser(VideoProcesser * vp);
void FreeVideoProcesser(VideoProcesser ** pvp);

/*
/功能:处理视频流缓冲区
vp：VideoProcesser. 文件流参数
sr：渲染器具，用于内部
备注：内部会锁住一直处理文件流读出的数据，因此需要外部开线程处理
*/
int video_process_packet(VideoProcesser *vp,ScreenRender *sr);

#endif