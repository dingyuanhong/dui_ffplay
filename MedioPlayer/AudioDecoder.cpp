#include "AudioDecoder.h"

static inline
int cmp_audio_fmts(enum AVSampleFormat fmt1, int64_t channel_count1,
                   enum AVSampleFormat fmt2, int64_t channel_count2)
{
    /* If channel count == 1, planar and non-planar formats are the same */
    if (channel_count1 == 1 && channel_count2 == 1)
        return av_get_packed_sample_fmt(fmt1) != av_get_packed_sample_fmt(fmt2);
    else
        return channel_count1 != channel_count2 || fmt1 != fmt2;
}
static inline
int64_t get_valid_channel_layout(int64_t channel_layout, int channels)
{
    if (channel_layout && av_get_channel_layout_nb_channels(channel_layout) == channels)
        return channel_layout;
    else
        return 0;
}

//<0 退出,==0继续，>0正确结果
int DecodePacket(AudioPacketDecode *is,AVPacket *pkt_temp,AVFrame *frame,struct AudioParams audio_src)
{
    AVCodecContext *dec = is->codec;
    int len1;
    int got_frame;
    int wanted_nb_samples = 0;
	AVRational tb = is->tb;
	AVRational AVR_tmp ={0};

	if (frame == NULL) {
		return AVERROR(ENOMEM);//外层整体退出
    }
    if (!is->audio_buf_frames_pending) {
        len1 = avcodec_decode_audio4(dec, frame, &got_frame, pkt_temp);
        if (len1 < 0) {
            /* if error, we skip the frame */
            pkt_temp->size = 0;
            return 0;
        }

        pkt_temp->dts =
        pkt_temp->pts = AV_NOPTS_VALUE;
        pkt_temp->data += len1;
        pkt_temp->size -= len1;
        if (pkt_temp->data && pkt_temp->size <= 0 || !pkt_temp->data && !got_frame)
            pkt_temp->stream_index = -1;

        if (!got_frame) return 0;//继续循环
	
		AVR_tmp.num = 1;
		AVR_tmp.den = frame->sample_rate;
		
		tb = AVR_tmp;
        if (frame->pts != AV_NOPTS_VALUE)
            frame->pts = av_rescale_q(frame->pts, dec->time_base, tb);
        else if (frame->pkt_pts != AV_NOPTS_VALUE)
            frame->pts = av_rescale_q(frame->pkt_pts, is->time_base, tb);
        else if (is->audio_frame_next_pts != AV_NOPTS_VALUE)
#if CONFIG_AVFILTER
		{
			AVR_tmp.den = 1;
			AVR_tmp.num = is->audio_filter_src.freq;
			is->frame->pts = av_rescale_q(is->audio_frame_next_pts, AVR_tmp, tb);
            //is->frame->pts = av_rescale_q(is->audio_frame_next_pts, (AVRational){1, is->audio_filter_src.freq}, tb);
		}
#else
		{
			AVR_tmp.num = 1;
			AVR_tmp.den = audio_src.freq;;
			
			frame->pts = av_rescale_q(is->audio_frame_next_pts, AVR_tmp, tb);
            //is->frame->pts = av_rescale_q(is->audio_frame_next_pts, (AVRational){1, is->audio_src.freq}, tb);
		}
#endif

        if (frame->pts != AV_NOPTS_VALUE)
            is->audio_frame_next_pts = frame->pts + frame->nb_samples;

#if CONFIG_AVFILTER
        dec_channel_layout = get_valid_channel_layout(is->frame->channel_layout, av_frame_get_channels(is->frame));

        reconfigure =
            cmp_audio_fmts(is->audio_filter_src.fmt, is->audio_filter_src.channels,
                            is->frame->format, av_frame_get_channels(is->frame))    ||
            is->audio_filter_src.channel_layout != dec_channel_layout ||
            is->audio_filter_src.freq           != is->frame->sample_rate ||
            is->audio_pkt_temp_serial           != is->audio_last_serial;

        if (reconfigure) {
            char buf1[1024], buf2[1024];
            av_get_channel_layout_string(buf1, sizeof(buf1), -1, is->audio_filter_src.channel_layout);
            av_get_channel_layout_string(buf2, sizeof(buf2), -1, dec_channel_layout);
            av_log(NULL, AV_LOG_DEBUG,
                    "Audio frame changed from rate:%d ch:%d fmt:%s layout:%s serial:%d to rate:%d ch:%d fmt:%s layout:%s serial:%d\n",
                    is->audio_filter_src.freq, is->audio_filter_src.channels, av_get_sample_fmt_name(is->audio_filter_src.fmt), buf1, is->audio_last_serial,
                    is->frame->sample_rate, av_frame_get_channels(is->frame), av_get_sample_fmt_name(is->frame->format), buf2, is->audio_pkt_temp_serial);

            is->audio_filter_src.fmt            = is->frame->format;
            is->audio_filter_src.channels       = av_frame_get_channels(is->frame);
            is->audio_filter_src.channel_layout = dec_channel_layout;
            is->audio_filter_src.freq           = is->frame->sample_rate;
            is->audio_last_serial               = is->audio_pkt_temp_serial;

            if ((ret = configure_audio_filters(is, afilters, 1)) < 0)
                return ret;
        }

        if ((ret = av_buffersrc_add_frame(is->in_audio_filter, is->frame)) < 0)
            return ret;
        av_frame_unref(is->frame);
#endif
    }
#if CONFIG_AVFILTER
    if ((ret = av_buffersink_get_frame_flags(is->out_audio_filter, is->frame, 0)) < 0) {
        if (ret == AVERROR(EAGAIN)) {
            is->audio_buf_frames_pending = 0;
            continue;
        }
        return ret;
    }
    is->audio_buf_frames_pending = 1;
    tb = is->out_audio_filter->inputs[0]->time_base;
#endif

	is->tb = tb;
    return 1;
}

struct AudioPacketDecode *InitDecoder(AVStream *audio_st)
{
	struct AudioPacketDecode *ret  = (struct AudioPacketDecode *)av_malloc(sizeof(struct AudioPacketDecode));
	memset(ret,0,sizeof(struct AudioPacketDecode));

	ret->codec = audio_st->codec;
	ret->time_base = audio_st->time_base;
	ret->audio_buf_frames_pending = 1;
	return ret;
}
void UninitDecoder(struct AudioPacketDecode **pdec)
{
	if(pdec != NULL && *pdec != NULL){
		av_free(*pdec);
		*pdec = NULL;
	}
}
int BeginDecoder(struct AudioPacketDecode *dec,int64_t pts)
{
	if(dec != NULL){
		avcodec_flush_buffers(dec->codec);
		dec->audio_buf_frames_pending = 0;
		dec->audio_frame_next_pts = pts;
	}
	return 0;
}
int SetNextPts(struct AudioPacketDecode *dec,int64_t pts)
{
	dec->audio_frame_next_pts = pts;
	return 0;
}