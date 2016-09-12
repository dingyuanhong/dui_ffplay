#ifndef SYNCHRONIZECLOCK_H
#define SYNCHRONIZECLOCK_H

#include "ffmpeg.h"
#include "SynchronizeClockDef.h"
#include "AudioDef.h"
#include "VideoState.h"

//初始化同步器
SynchronizeClockState *Create_Syschronize_Clock(int av_sync_type,VideoState *vs,PPosCallback cb,void *usr);
void Free_Syschronize_Clock(SynchronizeClockState** is);

//初始化音频同步器,用于音频同步至其他时钟
AudioSynchronize *CreateAudioSynchronize(SynchronizeClockState *sc,double audio_diff_threshold);
void InitAudioSynchronize(AudioSynchronize * is,SynchronizeClockState *sc,double audio_diff_threshold);
//取消绑定
void UninitAudioSynchronize(AudioSynchronize *is);
void FreeAudioSynchronize(AudioSynchronize **pis);
//初始化视频同步器
VideoSynchronize *CreateVideoSynchronize(SynchronizeClockState *sc,double max_frame_duration);
void InitVideoSynchronize(VideoSynchronize * vs,SynchronizeClockState *sc,double max_frame_duration);
void UninitVideoSynchronize(VideoSynchronize *is);
void FreeVideoSynchronize(VideoSynchronize **pis);

//获取同步方式
int get_master_sync_type(SynchronizeClockState *is);

//获取主时钟时间
/* get the current master clock value */
double get_master_clock(SynchronizeClockState *is);

/* return the wanted number of samples to get better sync if sync_type is video
 * or external master clock */
//同步音频至其他时钟
int synchronize_audio(SynchronizeClockState *scs,int nb_samples,struct AudioParams audio_src);
//更新视频PTS步
void update_video_pts(SynchronizeClockState *scs, double pts, int serial);
//暂停或启动视频
//返回值为是否暂停
int sc_stream_toggle_pause(SynchronizeClockState *is,int paused,int read_pause_return);

//匹配视频延迟,用以计算视频显示需等待时长
double compute_target_delay(double delay,double max_frame_duration, SynchronizeClockState *scs);
//更新音频同步时钟
/*
更新音频帧时钟
tb:样本时长
audio_serial:是否连续的音频数据
*/
void updata_audio_frame_clock(SynchronizeClockState * scs,AVFrame *frame,AVRational tb,int audio_serial);
//更新同步音频时钟
void update_audio_clock(SynchronizeClockState *scs,int audio_write_buf_size,int audio_hw_buf_size,int bytes_per_sec,int64_t audio_callback_time);
//检查并调整主时钟速度
void check_external_clock_speed(VideoState *is,SynchronizeClockState *scs);
//设置外部时钟数据
void set_extclk_clock(int seek_flags,int64_t seek_target,SynchronizeClockState *scs);

#endif