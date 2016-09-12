#ifndef AUDIODEF_H
#define AUDIODEF_H

#include "ffmpeg.h"

typedef struct AudioParams {
    int freq;				//采样率
    int channels;			//通道数
    int64_t channel_layout;
    enum AVSampleFormat fmt;
} AudioParams;

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)

typedef struct AudioSampleArray{
	int16_t sample_array[SAMPLE_ARRAY_SIZE];
    int sample_array_index;
}AudioSampleArray;

#endif