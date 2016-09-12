#include "AudioRender.h"
#include "MSDL.h"
#include "SDLAudioOpen.h"

void SDLCALL sdl_audio_callback(void *opaque, Uint8 *stream, int len)
{
	AudioRender *pthis = (AudioRender*)opaque;
	pthis->AudioCallback(stream,len);
}

AudioRender::AudioRender()
{
	m_is = NULL;
	m_scs = NULL;
	m_ap = NULL;
	m_as = NULL;

	memset(&audio_tgt,0,sizeof(struct AudioParams));
	audio_hw_buf_size = 0;
	m_nPause = 0;

	m_nVolume = 128;
	m_bMute = 0;
}
AudioRender::~AudioRender()
{
	m_nPause = 1;
	SDL_PauseAudio(1);
	if(m_ap != NULL){
		FreeAudioProcesser(&m_ap);
		m_ap = NULL;
	}
	if(m_as != NULL){
		FreeAudioSynchronize(&m_as);
	}
}
int AudioRender::Open(VideoState *is,SynchronizeClockState *scs)
{
	AVCodecContext *avctx = is->audio_st->codec;
	int sample_rate    = avctx->sample_rate;
    int nb_channels    = avctx->channels;
    int64_t channel_layout = avctx->channel_layout;
	

	SDL_PauseAudio(1);
	m_nPause = 1;
	int nRet = SDL_Audio_Open(sdl_audio_callback,this,channel_layout,nb_channels,sample_rate,&audio_tgt);
	if(nRet != -1){
		SDL_PauseAudio(0);
	}
	audio_hw_buf_size = nRet;

	double audio_diff_threshold = 2.0 * audio_hw_buf_size / av_samples_get_buffer_size(NULL, audio_tgt.channels, audio_tgt.freq, audio_tgt.fmt, 1);
	
	
	if(m_is != is){
		if(m_ap != NULL){
			FreeAudioProcesser(&m_ap);
			m_ap = NULL;
		}
		if(m_as == NULL)
		{
			FreeAudioSynchronize(&m_as);
			m_as = NULL;
		}
	}
	{
		m_is = is;
		m_scs = scs;

		//用于处理非使用音频同步方式音频自适应
		if(m_as == NULL) m_as = CreateAudioSynchronize(m_scs,audio_diff_threshold);
		//初始化音频处理器具
		if(m_ap == NULL) m_ap = CreateAudioProcesser(m_is->audio_st,audio_tgt);
		if(m_ap != NULL){
			m_ap->vs = m_is;
			m_ap->scs = m_scs;
		}
	}
	m_nPause = 0;
	return nRet;
}
void AudioRender::Play()
{
	SDL_PauseAudio(1);

}
void AudioRender::Pause(int nPause)
{
	m_nPause = nPause;
}
void AudioRender::Stop()
{
	SDL_PauseAudio(1);
	if(m_is != NULL) packet_queue_signal(&m_is->audioq);
	SDL_CloseAudio();
}
void AudioRender::AudioCallback(Uint8 *stream, int len)
{
	if(m_nPause) return;
	if(m_ap != NULL){
		if(m_bMute){
			FillAudioDeviceBuffer(m_ap,audio_tgt,audio_hw_buf_size,stream,len,0);
		}else{
			FillAudioDeviceBuffer(m_ap,audio_tgt,audio_hw_buf_size,stream,len,m_nVolume);
		}
	}
}
void AudioRender::SetVolume(int nVolume)
{
	if(nVolume < 0) nVolume = 0;
	if(nVolume > 128) nVolume = 128;
	m_nVolume = nVolume;
}
int AudioRender::GetVolume()
{
	return m_nVolume;
}
int AudioRender::GetMaxVolume()
{
	return 128;
}
void AudioRender::Mute(int bMute)
{
	m_bMute = bMute;
}