#include "SynchronizeClock.h"
#include "MClock.h"

#define AVLOG printf

void UpdataClockValue(SynchronizeClockState* is,int show_mode)
{
	
	switch (get_master_sync_type(is)) {
        case AV_SYNC_VIDEO_MASTER:
        case AV_SYNC_AUDIO_MASTER:
			if(show_mode == SHOW_MODE_VIDEO){
				if(is->video_stream >= 0){
					if(is->cb !=NULL) is->cb(is->usr,is->vidclk.pts,0);
				}
				return;
			}
			{
				if(is->audio_stream >= 0){
					if(is->cb !=NULL) is->cb(is->usr,is->audclk.pts,0);
				}else{
					if(is->cb !=NULL) is->cb(is->usr,is->extclk.pts,0);
				}
			}
            break;
        default:
			if(is->cb !=NULL) is->cb(is->usr,is->extclk.pts,0);
            break;
    }
}

SynchronizeClockState *Create_Syschronize_Clock(int av_sync_type,VideoState *vs,PPosCallback cb,void *usr)
{
	SynchronizeClockState *is = (SynchronizeClockState*)av_malloc(sizeof(SynchronizeClockState));
	memset(is,0,sizeof(SynchronizeClockState));

	init_clock(&is->vidclk, &vs->videoq.serial);
    init_clock(&is->audclk, &vs->audioq.serial);
    init_clock(&is->extclk, &is->extclk.serial);

	is->show_mode = vs->show_mode;
    is->av_sync_type = av_sync_type;
	is->audio_clock_serial = -1;

	is->audio_stream = vs->audio_stream;
	is->video_stream = vs->video_stream;
	is->subtitle_stream = vs->subtitle_stream;

	is->cb = cb;
	is->usr = usr;
	return is;
}
void Free_Syschronize_Clock(SynchronizeClockState**is)
{
	if(is != NULL && *is != NULL){
		av_free(*is);
		*is = NULL;
	}
}

AudioSynchronize *CreateAudioSynchronize(SynchronizeClockState *sc,double audio_diff_threshold)
{
	AudioSynchronize *is = (AudioSynchronize*)av_malloc(sizeof(AudioSynchronize));
	memset(is,0,sizeof(AudioSynchronize));
	InitAudioSynchronize(is,sc,audio_diff_threshold);
	return is;
}
void InitAudioSynchronize(AudioSynchronize * is,SynchronizeClockState *sc,double audio_diff_threshold)
{
	if(is == NULL) return;
	memset(is,0,sizeof(AudioSynchronize));
	is->audio_diff_avg_coef  = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
    is->audio_diff_avg_count = 0;
    /* since we do not have a precise anough audio fifo fullness,
        we correct audio sync only if larger than this threshold */
    is->audio_diff_threshold = audio_diff_threshold;
	is->sc = sc;
	sc->as = is;
}

void UninitAudioSynchronize(AudioSynchronize *is)
{
	if(is != NULL){
		if(is->sc != NULL){
			is->sc->as = NULL;
		}
		is->sc = NULL;
	}
}
void FreeAudioSynchronize(AudioSynchronize **pis)
{
	if(pis == NULL) return;
	AudioSynchronize *is = *pis;
	UninitAudioSynchronize(is);
	if(is != NULL) av_free(is);
	*pis = NULL;
}
//初始化视频同步器
VideoSynchronize *CreateVideoSynchronize(SynchronizeClockState *sc,double max_frame_duration)
{
	VideoSynchronize *is = (VideoSynchronize*)av_malloc(sizeof(VideoSynchronize));
	memset(is,0,sizeof(VideoSynchronize));

	is->max_frame_duration = max_frame_duration;
	is->decoder_reorder_pts = -1;
	is->frame_last_dropped_pts = AV_NOPTS_VALUE;

	if(sc != NULL) sc->vs = is;
	is->sc = sc;
	return is;
}
void InitVideoSynchronize(VideoSynchronize * vs,SynchronizeClockState *sc,double max_frame_duration)
{
	if(vs == NULL) return;
	vs->max_frame_duration = max_frame_duration;
	vs->decoder_reorder_pts = -1;
	vs->frame_last_dropped_pts = AV_NOPTS_VALUE;

	if(sc != NULL) sc->vs = vs;
	vs->sc = sc;
}
void UninitVideoSynchronize(VideoSynchronize *is)
{
	if(is != NULL){
		if(is->sc != NULL){
			is->sc->as = NULL;
		}
		is->sc = NULL;
	}
}
void FreeVideoSynchronize(VideoSynchronize **pis)
{
	if(pis == NULL) return;
	UninitVideoSynchronize(*pis);
	if(*pis != NULL){
		av_free(*pis);
	}
	*pis = NULL;
}

int get_master_sync_type(SynchronizeClockState *is) 
{
    if (is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
		if (is->video_stream >= 0)
            return AV_SYNC_VIDEO_MASTER;
		else if(is->audio_stream >= 0){
            return AV_SYNC_AUDIO_MASTER;
		}else{
			return AV_SYNC_EXTERNAL_CLOCK;
		}
    } else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
		if (is->audio_stream >=0)
            return AV_SYNC_AUDIO_MASTER;
        else
            return AV_SYNC_EXTERNAL_CLOCK;
    } else {
        return AV_SYNC_EXTERNAL_CLOCK;
    }
}
/* get the current master clock value */
double get_master_clock(SynchronizeClockState *is)
{
    double val;

    switch (get_master_sync_type(is)) {
        case AV_SYNC_VIDEO_MASTER:
            val = get_clock(&is->vidclk);
            break;
        case AV_SYNC_AUDIO_MASTER:
            val = get_clock(&is->audclk);
            break;
        default:
            val = get_clock(&is->extclk);
            break;
    }
    return val;
}

double get_audio_diff(SynchronizeClockState *scs)
{
	double diff = get_clock(&scs->audclk) - get_master_clock(scs);
	return diff;
}

void set_audio_clock(SynchronizeClockState *scs,double audio_clock,int audio_clock_serial)
{
	/* update the audio clock with the pts */
	scs->audio_clock = audio_clock;
	scs->audio_clock_serial = audio_clock_serial;
}

double get_av_diff(SynchronizeClockState *scs)
{
	double av_diff = 0.0;
	if (scs->audio_stream >= 0 && scs->video_stream >= 0)
        av_diff = get_clock(&scs->audclk) - get_clock(&scs->vidclk);
    else if (scs->video_stream >= 0)
        av_diff = get_master_clock(scs) - get_clock(&scs->vidclk);
    else if (scs->audio_stream >= 0)
        av_diff = get_master_clock(scs) - get_clock(&scs->audclk);
	return av_diff;
}

double get_video_diff(SynchronizeClockState *scs)
{
	double clockdiff = get_clock(&scs->vidclk) - get_master_clock(scs);
	return clockdiff;
}

void set_extclk_clock(int seek_flags,int64_t seek_target,SynchronizeClockState *scs)
{
	if (seek_flags & AVSEEK_FLAG_BYTE) {
        set_clock(&scs->extclk, NAN, 0);
		if(scs->cb != NULL) scs->cb(scs->usr,0,0);
    } else {
        set_clock(&scs->extclk, seek_target / (double)AV_TIME_BASE, 0);
		if(scs->cb != NULL) scs->cb(scs->usr,scs->extclk.pts,0);
    }
}

void check_external_clock_speed(VideoState *is,SynchronizeClockState *scs) 
{
   if (is->video_stream >= 0 && is->videoq.nb_packets <= MIN_FRAMES / 2 ||
       is->audio_stream >= 0 && is->audioq.nb_packets <= MIN_FRAMES / 2) {
       set_clock_speed(&scs->extclk, FFMAX(EXTERNAL_CLOCK_SPEED_MIN, scs->extclk.speed - EXTERNAL_CLOCK_SPEED_STEP));
   } else if ((is->video_stream < 0 || is->videoq.nb_packets > MIN_FRAMES * 2) &&
              (is->audio_stream < 0 || is->audioq.nb_packets > MIN_FRAMES * 2)) {
       set_clock_speed(&scs->extclk, FFMIN(EXTERNAL_CLOCK_SPEED_MAX, scs->extclk.speed + EXTERNAL_CLOCK_SPEED_STEP));
   } else {
       double speed = scs->extclk.speed;
       if (speed != 1.0)
           set_clock_speed(&scs->extclk, speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
   }
}
/* return the wanted number of samples to get better sync if sync_type is video
 * or external master clock */
int synchronize_audio(SynchronizeClockState *scs, int nb_samples,struct AudioParams audio_src)
{
    int wanted_nb_samples = nb_samples;
	
    /* if not master, then we try to remove or add samples to correct the clock */
    if (scs != NULL && get_master_sync_type(scs) != AV_SYNC_AUDIO_MASTER ) {
		AudioSynchronize *as = scs->as;
		if(as == NULL) return wanted_nb_samples;

        double diff, avg_diff;
        int min_nb_samples, max_nb_samples;

        diff = get_audio_diff(scs);

        if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD) {
            as->audio_diff_cum = diff + as->audio_diff_avg_coef * as->audio_diff_cum;
            if (as->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
                /* not enough measures to have a correct estimate */
                as->audio_diff_avg_count++;
            } else {
                /* estimate the A-V difference */
                avg_diff = as->audio_diff_cum * (1.0 - as->audio_diff_avg_coef);

                if (fabs(avg_diff) >= as->audio_diff_threshold) {
                    wanted_nb_samples = nb_samples + (int)(diff * audio_src.freq);
                    min_nb_samples = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
                    max_nb_samples = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
                    wanted_nb_samples = FFMIN(FFMAX(wanted_nb_samples, min_nb_samples), max_nb_samples);
                }
                //av_dlog(NULL, "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n",
                //        diff, avg_diff, wanted_nb_samples - nb_samples,
                //        is->audio_clock, is->audio_diff_threshold);
            }
        } else {
            /* too big difference : may be initial PTS errors, so
               reset A-V filter */
            as->audio_diff_avg_count = 0;
            as->audio_diff_cum       = 0;
        }
    }

    return wanted_nb_samples;
}

/* pause or resume the video */
int sc_stream_toggle_pause(SynchronizeClockState *is,int paused,int read_pause_return)
{
    if (paused) {
        if (read_pause_return != AVERROR(ENOSYS)) {
            is->vidclk.paused = 0;
        }
        set_clock(&is->vidclk, get_clock(&is->vidclk), is->vidclk.serial);
    }
    set_clock(&is->extclk, get_clock(&is->extclk), is->extclk.serial);
    is->audclk.paused = is->vidclk.paused = is->extclk.paused = !paused;
	return !paused;
}

void update_video_pts(SynchronizeClockState *scs, double pts, int serial)
{
	/* update current video pts */
	set_clock(&scs->vidclk, pts, serial);
	sync_clock_to_slave(&scs->extclk, &scs->vidclk);
	UpdataClockValue(scs,scs->show_mode);
}

double compute_target_delay(double delay,double max_frame_duration, SynchronizeClockState *scs)
{
    double sync_threshold, diff;

    /* update delay to follow master synchronisation source */
    if (get_master_sync_type(scs) != AV_SYNC_VIDEO_MASTER) {
        /* if video is slave, we try to correct big delays by
           duplicating or deleting a frame */
        diff = get_clock(&scs->vidclk) - get_master_clock(scs);

        /* skip or repeat frame. We take into account the
           delay to compute the threshold. I still don't know
           if it is the best guess */
        sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        if (!isnan(diff) && fabs(diff) < max_frame_duration) {
            if (diff <= -sync_threshold)	//视频慢
                delay = FFMAX(0, delay + diff);
            else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)//视频太快了
                delay = delay + diff;
            else if (diff >= sync_threshold)//视频快
                delay = 2 * delay;
        }
    }

    av_dlog(NULL, "video: delay=%0.3f A-V=%f\n",
            delay, -diff);

    return delay;
}

void updata_audio_frame_clock(SynchronizeClockState * scs,AVFrame *frame,AVRational tb,int audio_serial)
{
	double audio_clock = NAN;
	/* update the audio clock with the pts */
	if (frame->pts != AV_NOPTS_VALUE)
		audio_clock = frame->pts * av_q2d(tb) + (double) frame->nb_samples / frame->sample_rate;
	set_audio_clock(scs,audio_clock,audio_serial);
}

void update_audio_clock(SynchronizeClockState *scs,int audio_write_buf_size,int audio_hw_buf_size,int bytes_per_sec,int64_t audio_callback_time)
{
	scs->audio_write_buf_size = audio_write_buf_size;
    /* Let's assume the audio driver that is used by SDL has two periods. */
    if (!isnan(scs->audio_clock)) {
		double audio_clock = scs->audio_clock - (double)(2 * audio_hw_buf_size + scs->audio_write_buf_size) / bytes_per_sec;
		set_clock_at(&scs->audclk,audio_clock , scs->audio_clock_serial, audio_callback_time / 1000000.0);
        sync_clock_to_slave(&scs->extclk, &scs->audclk);
		UpdataClockValue(scs,scs->show_mode);
    }
}