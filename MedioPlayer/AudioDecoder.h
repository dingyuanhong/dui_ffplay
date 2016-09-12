#ifndef AUDIODECODER_H
#define AUDIODECODER_H

/*
功能:音频解码函数
*/

#include "ffmpeg.h"
#include "AudioDef.h"

typedef struct AudioPacketDecode
{
	AVCodecContext *codec;
	AVRational time_base;//流时间

						//同步计算的参数
	AVRational tb;		//0
	int64_t audio_frame_next_pts;//下一包PTS
	

	int audio_buf_frames_pending;//音频缓冲区阻塞，0
}AudioPacketDecode;

struct AudioPacketDecode *InitDecoder(AVStream *audio_st);
void UninitDecoder(struct AudioPacketDecode **pdec);
/*
功能:启动解码器
*/
int BeginDecoder(struct AudioPacketDecode *dec,int64_t pts);
/*
功能:设置下一帧率的PTS
*/
int SetNextPts(struct AudioPacketDecode *dec,int64_t pts);
/*
功能:解码音频包
返回值:<0 退出,==0继续，>0正确结果
*/
int DecodePacket(AudioPacketDecode *is,AVPacket *pkt_temp,AVFrame *frame,struct AudioParams audio_src);

#endif