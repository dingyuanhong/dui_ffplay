#include "MedioPlay.h"
#include "ffmpeg.h"
#include "packet_queue.h"
#include "SynchronizeClock.h"

void MedioPosCallback(void *usr,int64_t pos,int64_t max)
{
	MedioPlay *pthis = (MedioPlay*)usr;
	pthis->PosCall(pos);
}

DWORD WINAPI Thread_ReadFile(
    LPVOID lpThreadParameter
    )
{
	MedioPlay *pthis = (MedioPlay*)lpThreadParameter;
	return pthis->ThreadReadFile();
}

MedioPlay::MedioPlay()
{
	m_isPlaying = false;
	m_isPause = false;
	m_isStop = false;

	m_is = NULL;
	m_scs = NULL;
	
	memset(&m_sc,0,sizeof(StreamConfigure));

	InitStreamConfigure(&m_sc);

	m_pRender = NULL;
	m_pVideo = NULL;

	m_hThread = NULL;
	m_hwnd = NULL;
	m_pcb = NULL;
	m_usr = NULL;
	m_nCurPos = -1;
}
MedioPlay::~MedioPlay()
{
	Stop();
}
void MedioPlay::SetPosCallback(PPosCallback cb,void *usr)
{
	m_pcb = cb;
	m_usr = usr;
}
void MedioPlay::AdjustPost(int64_t nPos)
{
	if(m_is == NULL) return;
	seek_pos(m_is,m_scs,nPos);
}
void MedioPlay::SetPos(int64_t nPos)
{
	if(m_is != NULL){
		seek_stream_pos(m_is,nPos);
		if(!m_is->abort_request){
			if(m_is->eof){
				RePlay();
			}
		}
	}
}
int MedioPlay::Open(const char* FileName)
{
	if(m_strName == FileName) return 1;
	if(m_strName != ""){
		Stop();
		if(m_pRender != NULL) delete m_pRender;
		m_pRender = NULL;
		if(m_pVideo != NULL) delete m_pVideo;
		m_pVideo = NULL;
		Free_Syschronize_Clock(&m_scs);
		FreeVideoState(&m_is);
		m_isStop = 0;
		m_isPause = 0;
	}
	m_strName = FileName;

	SDL_INIT();

	VideoState *is = CreateVideoState();

	int nRet = OpenStream(is,m_strName.c_str(),&m_sc);
	if(nRet != -1)
	{
		m_is = is;
		m_scs = Create_Syschronize_Clock(AV_SYNC_AUDIO_MASTER,is,MedioPosCallback,this);
	}else{
		av_free(is);
	}
	if(m_pcb != NULL) m_pcb(m_usr,0,GetTimes());
	return nRet;
}
int MedioPlay::ThreadReadFile()
{
	m_isPlaying = true;
	int nRet =  LoadStreams(m_is,m_scs,&m_sc);

	if(m_is->eof){
		if(m_is->videoq.nb_packets <= 0){
			//如果视频缓冲为0，则停止视频播放
			if(m_pVideo != NULL){
				//循环等待视频播放完毕
				while(true){
					if(m_is->abort_request) break;
					if(m_is->videoq.abort_request) break;
					int nSize = m_pVideo->GetVideoPQSize();
					if(nSize <= 0) break;
					::Sleep(1);
				}
			}
		}
		if(m_is->audioq.nb_packets > 0){
			if(m_pRender != NULL){
				while(true){
					if(m_is->abort_request) break;
					if(m_is->audioq.abort_request) break;
					if(m_is->audioq.nb_packets <= 0) break;
					::Sleep(1);
				}
			}
		}
		PosCall(GetTimes());
	}
	m_is->audioq.abort_request = m_is->videoq.abort_request = m_is->subtitleq.abort_request = true;

	if(m_pRender != NULL)
	{
		m_pRender->Stop();
	}
	if(m_pVideo != NULL)
	{
		m_pVideo->Stop();
	}
	m_isPlaying = false;
	return nRet;
}
void MedioPlay::RePlay()
{
	bool eof = m_is->eof;
	if(eof == 1){
		if(m_hThread != NULL){
			WaitForSingleObject(m_hThread,INFINITE);
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}
	}
	m_is->audioq.abort_request = m_is->videoq.abort_request = m_is->subtitleq.abort_request = false;

	if(m_pRender == NULL){
		m_pRender = new AudioRender();
		m_pRender->Open(m_is,m_scs);
	}
	if(m_pVideo == NULL){
		m_pVideo = new VideoRender(m_hwnd);
		m_pVideo->Play(m_is,m_scs);
	}
	if(m_hThread == NULL){
		m_hThread = CreateThread(NULL,0,Thread_ReadFile,this,0,NULL);
	}

	if(IsStop() || eof){
		m_isStop = false;
		if(m_pRender != NULL){
			m_pRender->Open(m_is,m_scs);
			m_pRender->Pause(m_isPause);
		}
		if(m_pVideo != NULL){
			m_pVideo->Play(m_is,m_scs);
			m_pVideo->Pause(m_isPause);
		}
	}else{
		if(m_pRender != NULL){
			m_pRender->Pause(m_isPause);
		}
		if(m_pVideo != NULL){
			m_pVideo->Pause(m_isPause);
		}
	}
}
void MedioPlay::Play()
{
	m_isPause = false;

	bool eof = m_is->eof;
	if(eof == 1){
		if(m_hThread != NULL){
			WaitForSingleObject(m_hThread,INFINITE);
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}
	}
	
	m_is->audioq.abort_request = m_is->videoq.abort_request = m_is->subtitleq.abort_request = m_is->abort_request = 0;
	
	if(m_is->paused == true) m_is->paused = false;

	if(m_pRender == NULL){
		m_pRender = new AudioRender();
		m_pRender->Open(m_is,m_scs);
	}
	if(m_pVideo == NULL){
		m_pVideo = new VideoRender(m_hwnd);
		m_pVideo->Play(m_is,m_scs);
	}
	if(m_hThread == NULL){
		m_hThread = CreateThread(NULL,0,Thread_ReadFile,this,0,NULL);
	}

	if(IsStop() || eof){
		m_isStop = false;
		if(m_pRender != NULL){
			m_pRender->Open(m_is,m_scs);
			m_pRender->Pause(m_isPause);
		}
		if(m_pVideo != NULL){
			m_pVideo->Play(m_is,m_scs);
			m_pVideo->Pause(m_isPause);
		}
	}else{
		if(m_pRender != NULL){
			m_pRender->Pause(m_isPause);
		}
		if(m_pVideo != NULL){
			m_pVideo->Pause(m_isPause);
		}
	}
}
void MedioPlay::Pause()
{
	m_isPause = true;
	m_is->paused = true;
	if(m_pRender != NULL) m_pRender->Pause(m_isPause);
	if(m_pVideo != NULL) m_pVideo->Pause(m_isPause);
}
void MedioPlay::Stop()
{
	m_isStop = true;
	m_is->abort_request = true;

	if(m_hThread != NULL)
	{
		WaitForSingleObject(m_hThread,INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	m_nCurPos = -1;
}
bool MedioPlay::IsPlaying()
{
	return m_isPlaying;
}
bool MedioPlay::IsPause()
{
	return m_isPause;
}
bool MedioPlay::IsStop()
{
	return m_isStop;
}
//更换屏幕大小
void MedioPlay::Resize(int width,int height)
{
	if(m_pVideo != NULL)
	{
		m_pVideo->Resize(width,height);
	}
}
int MedioPlay::PosCall(int64_t pos)
{
	if(pos != m_nCurPos){
		m_nCurPos = pos;

		if(m_pcb != NULL){
			m_pcb(m_usr,m_nCurPos,0);
		}
	}
	return 0;
}
int64_t MedioPlay::GetTimes()//单位秒
{	
	if(m_is == NULL) return 0;
	if(m_is->ic == NULL) return 0;
	return m_is->ic->duration/1000000LL;
}

void MedioPlay::BindWindow(HWND id)
{
	m_hwnd = id;
	SDL_BindWindow(m_hwnd);
};

	