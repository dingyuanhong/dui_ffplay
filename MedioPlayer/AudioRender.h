#ifndef AUDIORENDER_H
#define AUDIORENDER_H

#include "VideoState.h"
#include "AudioDecoder.h"
#include "AudioProcesser.h"
#include "SynchronizeClock.h"

class AudioRender
{
public:
	AudioRender();
	~AudioRender();
	int Open(VideoState *is,SynchronizeClockState *scs);
	void Play();
	void Pause(int nPause);
	void Stop();

public:
	void SetVolume(int nVolume);
	int GetVolume();
	int GetMaxVolume();
	void Mute(int bMute);

	struct AudioParams GetAudiotgt(){return audio_tgt;}
	void AudioCallback(Uint8 *stream, int len);
private:
	VideoState *m_is;
	SynchronizeClockState *m_scs;
	AudioProcesser *m_ap;
	AudioSynchronize *m_as;

	int m_nPause;
	struct AudioParams audio_tgt;
	int audio_hw_buf_size;		//“Ù∆µ‰÷»æª∫≥Â«¯¥Û–°
	int m_nVolume;
	int m_bMute;

};

#endif