#ifndef AUDIORESAMPLE_H
#define AUDIORESAMPLE_H

#include "AudioDef.h"

typedef struct AudioDataBuffer
{
	uint8_t *audio_buf;
	unsigned int audio_buf_size;
}AudioDataBuffer;

typedef struct AudioSampleParam
{
	struct AudioParams audio_src;//当前重采样器的音频参数,
	struct SwrContext *swr_ctx;//重采样器
	uint8_t *audio_buf1;
    unsigned int audio_buf1_size;
}AudioSampleParam;

//音频重采样
//参数:音频播放器所使用的参数
struct AudioSampleParam * AR_InitAudioSample(struct AudioParams audio_src);
void AR_UninitAudioSample(struct AudioSampleParam **pasp);

struct AudioParams AR_GetAudioParams(struct AudioSampleParam *pasp);
//检查是否需要重采样
//audio_tgt:需要重采样达到的目标音频参数
//-1:出错，0 无需，1 需要冲采样
int AR_ChechAudioSamle(struct AudioSampleParam *pasp,AVFrame *frame,struct AudioParams audio_tgt,int wanted_nb_samples);
//音频重采样
int AR_AudioResample(struct AudioSampleParam *pasp,AVFrame *frame,struct AudioParams audio_tgt,int wanted_nb_samples,AudioDataBuffer *pout);

#endif