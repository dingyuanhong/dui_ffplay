#ifndef AUDIODECODE_H
#define AUDIODECODE_H

#include "ffmpeg.h"
#include "packet_queue.h"
#include "VideoState.h"
#include "MClock.h"

enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

typedef struct AudioParams {
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
} AudioParams;

/* SDL audio buffer size, in samples. Should be small to have precise
   A/V sync as SDL does not have hardware buffer fullness info. */
#define SDL_AUDIO_BUFFER_SIZE 1024

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20
/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

typedef struct AudioDecodeState
{
	AVPacket audio_pkt_temp;
	int audio_pkt_temp_serial;//帧序号
	int64_t audio_frame_next_pts;//下一包PTS

	AVPacket audio_pkt;
	AVFrame *frame;
	int audio_buf_frames_pending;//音频缓冲区阻塞

	struct AudioParams audio_src;

	struct SwrContext *swr_ctx;//重采样器
	AVStream *audio_st;//解码器具

	uint8_t *audio_buf1;
    unsigned int audio_buf1_size;

	uint8_t *audio_buf;
	unsigned int audio_buf_index;
	unsigned int audio_buf_size;

	uint8_t silence_buf[SDL_AUDIO_BUFFER_SIZE];

	int16_t sample_array[SAMPLE_ARRAY_SIZE];
	unsigned int sample_array_index;

	int paused;


	Clock audclk;

	double audio_diff_cum; /* used for AV difference average computation */
	double audio_diff_avg_coef;
    double audio_diff_threshold;
    int audio_diff_avg_count;
}AudioDecodeState;



/**
 * Decode one audio frame and return its uncompressed size.
 *
 * The processed audio frame is decoded, converted if required, and
 * stored in is->audio_buf, with size in bytes given by the return
 * value.
 */
int audio_decode_frame(AudioDecodeState *is,PacketQueue *audioq,struct AudioParams audio_tgt);

#endif