#ifndef VIDEORENDER_H
#define VIDEORENDER_H

#include "VideoPictureRender.h"
#include "SDLRenderParam.h"

class VideoRender
{
public:
	VideoRender(HWND hwnd);
	~VideoRender();
	int Pause(int nPause);		//暂停播放
	void Stop();				//停止播放
	void WaitStop();
	int Play(VideoState *is,SynchronizeClockState *scs);//播放视频
	void Resize(int width,int height);//调整播放大小
	//获取播放缓冲区数据大小
	int GetVideoPQSize();
	//获取字幕缓冲区数据大小
	int GetSubtitlePQSize();
public:
	//线程回调函数
	int SubtitleCallback();
	int VideoCallback();
	int VideoRenderCallback();
private:
	VideoState *m_is;
	SynchronizeClockState *m_scs;

	SubtitleBuffer* m_sb;
	ScreenRender *m_sr;
	VideoProcesser *m_vp;
	UserScreenRender * m_usr;

	SDL_Thread *m_subtitle_tid;
	SDL_Thread *m_video_tid;
	SDL_Thread *m_render_tid;
	HWND m_hwnd;
};

void RegisterWindow(HWND id);

#endif