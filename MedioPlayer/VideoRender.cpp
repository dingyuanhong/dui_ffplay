#include "VideoRender.h"
#include "VideoProcesser.h"
#include "SubtitleProcesser.h"
#include "SDLVideoRender.h"

int subtitle_m_thread(void *param)
{
	VideoRender *pthis = (VideoRender*)param;
	return pthis->SubtitleCallback();
}
int video_m_thread(void *param)
{
	VideoRender *pthis = (VideoRender*)param;
	return pthis->VideoCallback();
}
int video_render_thread(void *param)
{
	VideoRender *pthis = (VideoRender*)param;
	return pthis->VideoRenderCallback();
}
VideoRender::VideoRender(HWND hwnd)
{
	m_is = NULL;
	m_scs = NULL;

	m_sb = NULL;
	m_sr = NULL;
	m_vp = NULL;
	m_usr = NULL;
	m_hwnd = hwnd;

	m_subtitle_tid = NULL;
	m_video_tid = NULL;
	m_render_tid = NULL;
}
VideoRender::~VideoRender()
{
	
}
int VideoRender::Play(VideoState *is,SynchronizeClockState *scs)
{
	//数据源变更
	if(m_is != is){
		if(m_vp != NULL){
			FreeVideoProcesser(&m_vp);
		}
		if(m_sr != NULL){
			FreeScreenRender(&m_sr);
		}
		if(m_usr != NULL){
			FreeUserScreenRender(&m_usr);
		}
		if(m_sb != NULL){
			FreeSubtitle(&m_sb);
		}
	}
	m_is = is;
	m_scs = scs;

	RECT rt = {0};
	GetWindowRect(m_hwnd,&rt);
	double max_frame_duration = (m_is->ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

	{
		if(m_vp == NULL){
			VideoProcesser *vp = CreateVideoProcesser(max_frame_duration);
			InitVideoProcesser(vp,is,scs);
			m_vp = vp;
		}else{
			InitVideoProcesser(m_vp,is,scs);
		}
		if(m_usr == NULL){
			UserScreenRender * usr = CreateUserScreenRender(rt.right - rt.left,rt.bottom - rt.top);
			m_usr = usr;
		}else{
			InitUserScreenRender(m_usr,rt.right - rt.left,rt.bottom - rt.top);
		}
		if(m_sr == NULL){
			ScreenRender * sr = CreateScreenRender(m_usr,g_sdlRender);
			m_sr = sr;
		}else{
			InitScreenRender(m_sr,m_usr,g_sdlRender);
		}
		if(m_is->subtitle_stream >= 0){
			if(m_sb == NULL) m_sb = CreateSubtile();
		}else{
			if(m_sb != NULL){
				FreeSubtitle(&m_sb);
				m_sb = NULL;
			}
		}
	}
	if(m_subtitle_tid == NULL) m_subtitle_tid = SDL_CreateThread(subtitle_m_thread, this);
	if(m_video_tid == NULL) m_video_tid = SDL_CreateThread(video_m_thread, this);
	if(m_render_tid == NULL) m_render_tid = SDL_CreateThread(video_render_thread, this);
	return -1;
}
int VideoRender::Pause(int nPause)
{
	return 1;
}
void VideoRender::Stop()
{
	if(m_render_tid != NULL) SDL_WaitThread(m_render_tid,NULL);

	if(m_vp != NULL && m_vp->vpb != NULL){
		SDL_CondSignal(m_vp->vpb->pictq_cond);
	}
	if(m_is != NULL) packet_queue_signal(&m_is->videoq);
	if(m_video_tid != NULL){
		SDL_WaitThread(m_video_tid,NULL);
	}
	if(m_sb != NULL){
		SDL_CondSignal(m_sb->subpq_cond);
	}
	if(m_is != NULL) packet_queue_signal(&m_is->subtitleq);
	if(m_subtitle_tid != NULL){
		SDL_WaitThread(m_subtitle_tid,NULL);
	}
	m_subtitle_tid = NULL;
	m_video_tid = NULL;
	m_render_tid = NULL;
}
void VideoRender::WaitStop()
{

}
int VideoRender::SubtitleCallback()
{
	return subtitle_queue_processer(m_is,m_sb);
}
int VideoRender::VideoCallback()
{
	return video_process_packet(m_vp,m_sr);
}
int VideoRender::VideoRenderCallback()
{
	double remaining_time = 0.0;
    for (;;) {
		if(m_is->videoq.abort_request) break;
		if (remaining_time > 0.0)
			av_usleep((int64_t)(remaining_time * 1000000.0));
        remaining_time = REFRESH_RATE;
        if (m_is->show_mode != SHOW_MODE_NONE && (!m_is->paused || (m_sr != NULL && m_sr->force_refresh)))
            video_refresh(m_vp,m_sr,m_sb, &remaining_time);
	}
	return -1;
}
void VideoRender::Resize(int width,int height)
{
	if(m_usr != NULL){
		m_usr->screen_height = height;
		m_usr->screen_width = width;
		m_usr->iri.flush_screen = 1;

		if(m_sr != NULL){
			m_sr->force_refresh = 1;
		}
	}
}
//获取播放缓冲区数据大小
int VideoRender::GetVideoPQSize()
{
	if(m_vp != NULL){
		if(m_vp->vpb != NULL){
			return m_vp->vpb->pictq_size;
		}
	}
	return 0;
}
//获取字幕缓冲区数据大小
int VideoRender::GetSubtitlePQSize()
{
	if(m_sb != NULL){
		return m_sb->subpq_size;
	}
	return 0;
}