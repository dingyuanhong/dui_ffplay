#ifndef AUDIOSAMPLEDRAW_H
#define AUDIOSAMPLEDRAW_H

/*
功能:绘制音频样本频谱曲线图
*/

#include "ffmpeg.h"
#include "MSDL.h"
#include "AudioDef.h"
#include "SDLRenderParam.h"

//
//audio_tgt:音频播放器参数
//audio_callback_time:音频数据回调时长
//s:数据
//channels:通道数
//audio_write_buf_size:音频写入的长度
//bShowWAVE：是否按照WAVE方式显示
int Get_I_Start(ImageRenderInfor *iri,AudioSampleSpikes *pSampleSpikes,
	struct AudioParams audio_tgt,int channels,int64_t audio_callback_time,
	AudioSampleArray *s,int audio_write_buf_size,bool bShowWAVE);

int AudioSampleWAVE(ImageRenderInfor *iri,AudioSampleSpikes *pSampleSpikes,int channels,AudioSampleArray *s,int audio_write_buf_size);

int AudioSampleFFTS(ImageRenderInfor *iri,AudioSampleSpikes *pSampleSpikes,FFTSRenderInfor *fri,int channels,AudioSampleArray *s,int audio_write_buf_size);

//更新音频样本数据用于绘制频谱
/* copy samples for viewing in editor window */
void update_sample_display(AudioSampleArray *asa, short *samples, int samples_size);

#endif