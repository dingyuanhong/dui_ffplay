#ifndef SYNCHRONIZECLOCKDEF_H
#define SYNCHRONIZECLOCKDEF_H

#include "ffmpeg.h"
#include "AudioDef.h"

typedef struct Clock {
    double pts;           /* clock base，当前帧PTS,单位秒*/
    double pts_drift;     /* clock base minus time at which we updated the clock */
    double last_updated;
    double speed;
    int serial;           /* clock is based on a packet with this serial */
    int paused;
    int *queue_serial;    /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20
/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

typedef struct SynchronizeClockState;

//音频处理时用于计算同步的参数
typedef struct AudioSynchronize
{
	double audio_diff_cum; /* used for AV difference average computation */
	double audio_diff_avg_coef;
    double audio_diff_threshold;
    int audio_diff_avg_count;

	int64_t audio_current_pos;
	SynchronizeClockState *sc;
}AudioSynchronize;

//视频处理时用于计算同步的参数
typedef struct VideoSynchronize
{
	//视频
	double frame_timer;			//当前帧绘制时间点,默认0
	double max_frame_duration;	//帧间最大间距,填充计算方式:(AVFormatContext->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;
	int step;					//默认0,
	int decoder_reorder_pts;	//默认 -1;重排解码pts

	int64_t video_current_pos;
	double frame_last_pts;
	double frame_last_duration;

	double frame_last_filter_delay;
	int64_t frame_last_dropped_pos;
	double frame_last_dropped_pts;//AV_NOPTS_VALUE
	int frame_last_dropped_serial;
	int frame_drops_early;

	SynchronizeClockState *sc;
}VideoSynchronize;

enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

typedef void (*PPosCallback)(void *usr,int64_t pos,int64_t max);

typedef struct SynchronizeClockState
{
	int av_sync_type;			//时钟同步方式,默认0
	int show_mode;

	Clock audclk;
    Clock vidclk;
    Clock extclk;
	//用于标示是否存在这些流，默认:-1
	int audio_stream;
	int subtitle_stream;
	int video_stream;

	//音频
	double audio_clock;			//默认0
	int audio_clock_serial;		//默认-1

	int audio_write_buf_size;	//音频数据大小,默认0
	AudioSampleArray sample_array;//音频数据,用于视频渲染绘制频谱


	AudioSynchronize *as;		//用以音频同步至其他时钟
	VideoSynchronize *vs;		//用以视频自身记录同步参数

	//用于回调当前渲染位置
	PPosCallback cb;
	void *usr;
}SynchronizeClockState;

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001


#endif