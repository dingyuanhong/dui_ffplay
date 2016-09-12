#include "AudioDecode.h"

int get_master_sync_type() {
	return AV_SYNC_VIDEO_MASTER;
    /*if (is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
        if (is->video_st)
            return AV_SYNC_VIDEO_MASTER;
        else
            return AV_SYNC_AUDIO_MASTER;
    } else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
        if (is->audio_st)
            return AV_SYNC_AUDIO_MASTER;
        else
            return AV_SYNC_EXTERNAL_CLOCK;
    } else {
        return AV_SYNC_EXTERNAL_CLOCK;
    }*/
}
/* get the current master clock value */
double get_master_clock(AudioDecodeState *is)
{
    double val;
    val = get_clock(&is->audclk);
    return val;
}
/* return the wanted number of samples to get better sync if sync_type is video
 * or external master clock */
static int synchronize_audio(AudioDecodeState *is, int nb_samples,struct AudioParams audio_src)
{
	return nb_samples;
    //int wanted_nb_samples = nb_samples;

    ///*if not master, then we try to remove or add samples to correct the clock */
    //if (get_master_sync_type() != AV_SYNC_AUDIO_MASTER) {
    //    double diff, avg_diff;
    //    int min_nb_samples, max_nb_samples;

    //    diff = get_clock(&is->audclk) - get_master_clock(is);

    //    if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD) {
    //        is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
    //        if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
    //            /* not enough measures to have a correct estimate */
    //            is->audio_diff_avg_count++;
    //        } else {
    //            /* estimate the A-V difference */
    //            avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);

    //            if (fabs(avg_diff) >= is->audio_diff_threshold) {
    //                wanted_nb_samples = nb_samples + (int)(diff * audio_src.freq);
    //                min_nb_samples = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
    //                max_nb_samples = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
    //                wanted_nb_samples = FFMIN(FFMAX(wanted_nb_samples, min_nb_samples), max_nb_samples);
    //            }
    //            //av_dlog(NULL, "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n",
    //            //        diff, avg_diff, wanted_nb_samples - nb_samples,
    //            //        is->audio_clock, is->audio_diff_threshold);
    //        }
    //    } else {
    //        /* too big difference : may be initial PTS errors, so
    //           reset A-V filter */
    //        is->audio_diff_avg_count = 0;
    //        is->audio_diff_cum       = 0;
    //    }
    //}

    //return wanted_nb_samples;
}

/**
 * Decode one audio frame and return its uncompressed size.
 *
 * The processed audio frame is decoded, converted if required, and
 * stored in is->audio_buf, with size in bytes given by the return
 * value.
 */
int audio_decode_frame(AudioDecodeState *is,PacketQueue *audioq,struct AudioParams audio_tgt)
{
    AVPacket *pkt_temp = &is->audio_pkt_temp;
    AVPacket *pkt = &is->audio_pkt;
    AVCodecContext *dec = is->audio_st->codec;
    int len1, data_size, resampled_data_size;
    int64_t dec_channel_layout;
    int got_frame;
    av_unused double audio_clock0;
    int wanted_nb_samples = 0;
    AVRational tb;
	AVRational AVR_tmp ={0};
    int ret;
    int reconfigure;

    for (;;) {
        /* NOTE: the audio packet can contain several frames */
        while (pkt_temp->stream_index != -1 || is->audio_buf_frames_pending) {
            if (!is->frame) {
                if (!(is->frame = avcodec_alloc_frame()))
                    return AVERROR(ENOMEM);
            } else {
                av_frame_unref(is->frame);
                avcodec_get_frame_defaults(is->frame);
            }

            if (audioq->serial != is->audio_pkt_temp_serial)
                break;

            if (is->paused)
                return -1;

            if (!is->audio_buf_frames_pending) {
                len1 = avcodec_decode_audio4(dec, is->frame, &got_frame, pkt_temp);
                if (len1 < 0) {
                    /* if error, we skip the frame */
                    pkt_temp->size = 0;
                    break;
                }

                pkt_temp->dts =
                pkt_temp->pts = AV_NOPTS_VALUE;
                pkt_temp->data += len1;
                pkt_temp->size -= len1;
                if (pkt_temp->data && pkt_temp->size <= 0 || !pkt_temp->data && !got_frame)
                    pkt_temp->stream_index = -1;

                if (!got_frame)
                    continue;

                //tb = (AVRational){1, is->frame->sample_rate};
				
				AVR_tmp.den = 1;
				AVR_tmp.num = is->frame->sample_rate;
				tb = AVR_tmp;
                if (is->frame->pts != AV_NOPTS_VALUE)
                    is->frame->pts = av_rescale_q(is->frame->pts, dec->time_base, tb);
                else if (is->frame->pkt_pts != AV_NOPTS_VALUE)
                    is->frame->pts = av_rescale_q(is->frame->pkt_pts, is->audio_st->time_base, tb);
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
					AVR_tmp.den = 1;
					AVR_tmp.num = is->audio_src.freq;
					is->frame->pts = av_rescale_q(is->audio_frame_next_pts, AVR_tmp, tb);
                    //is->frame->pts = av_rescale_q(is->audio_frame_next_pts, (AVRational){1, is->audio_src.freq}, tb);
				}
#endif

                if (is->frame->pts != AV_NOPTS_VALUE)
                    is->audio_frame_next_pts = is->frame->pts + is->frame->nb_samples;

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

            data_size = av_samples_get_buffer_size(NULL, av_frame_get_channels(is->frame),
                                                   is->frame->nb_samples,
                                                   (AVSampleFormat)is->frame->format, 1);

            dec_channel_layout = /*len1;*/
                (is->frame->channel_layout && av_frame_get_channels(is->frame) == av_get_channel_layout_nb_channels(is->frame->channel_layout)) ?
                is->frame->channel_layout : av_get_default_channel_layout(av_frame_get_channels(is->frame));

            wanted_nb_samples = synchronize_audio(is, is->frame->nb_samples,is->audio_src);
			//wanted_nb_samples = len1;
            if (is->frame->format        != is->audio_src.fmt            ||
                dec_channel_layout       != is->audio_src.channel_layout ||
                is->frame->sample_rate   != is->audio_src.freq           ||
                (wanted_nb_samples       != is->frame->nb_samples && !is->swr_ctx)) {
                swr_free(&is->swr_ctx);
                is->swr_ctx = swr_alloc_set_opts(NULL,
                                                 audio_tgt.channel_layout, audio_tgt.fmt, audio_tgt.freq,
                                                 dec_channel_layout,(AVSampleFormat)is->frame->format, is->frame->sample_rate,
                                                 0, NULL);
                if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
                    av_log(NULL, AV_LOG_ERROR,
                           "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
                            is->frame->sample_rate, av_get_sample_fmt_name((AVSampleFormat)is->frame->format), av_frame_get_channels(is->frame),
                            audio_tgt.freq, av_get_sample_fmt_name(audio_tgt.fmt), audio_tgt.channels);
                    break;
                }
                is->audio_src.channel_layout = dec_channel_layout;
                is->audio_src.channels       = av_frame_get_channels(is->frame);
                is->audio_src.freq = is->frame->sample_rate;
                is->audio_src.fmt = (AVSampleFormat)is->frame->format;
            }

            if (is->swr_ctx) {
                const uint8_t **in = (const uint8_t **)is->frame->extended_data;
                uint8_t **out = &is->audio_buf1;
                int out_count = (int64_t)wanted_nb_samples * audio_tgt.freq / is->frame->sample_rate + 256;
                int out_size  = av_samples_get_buffer_size(NULL, audio_tgt.channels, out_count, audio_tgt.fmt, 0);
                int len2;
                if (out_size < 0) {
                    av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
                    break;
                }
                if (wanted_nb_samples != is->frame->nb_samples) {
                    if (swr_set_compensation(is->swr_ctx, (wanted_nb_samples - is->frame->nb_samples) * audio_tgt.freq / is->frame->sample_rate,
                                                wanted_nb_samples * audio_tgt.freq / is->frame->sample_rate) < 0) {
                        av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
                        break;
                    }
                }
                av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
                if (!is->audio_buf1)
                    return AVERROR(ENOMEM);
                len2 = swr_convert(is->swr_ctx, out, out_count, in, is->frame->nb_samples);
                if (len2 < 0) {
                    av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
                    break;
                }
                if (len2 == out_count) {
                    av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too small\n");
                    swr_init(is->swr_ctx);
                }
                is->audio_buf = is->audio_buf1;
                resampled_data_size = len2 * audio_tgt.channels * av_get_bytes_per_sample(audio_tgt.fmt);
            } else {
                is->audio_buf = is->frame->data[0];
                resampled_data_size = data_size;
            }

            //audio_clock0 = is->audio_clock;
            ///* update the audio clock with the pts */
            //if (is->frame->pts != AV_NOPTS_VALUE)
            //    is->audio_clock = is->frame->pts * av_q2d(tb) + (double) is->frame->nb_samples / is->frame->sample_rate;
            //else
            //    is->audio_clock = NAN;
            //is->audio_clock_serial = is->audio_pkt_temp_serial;
#ifdef DEBUG
            {
                static double last_clock;
                printf("audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n",
                       is->audio_clock - last_clock,
                       is->audio_clock, audio_clock0);
                last_clock = is->audio_clock;
            }
#endif
            return resampled_data_size;
        }

        /* free the current packet */
        if (pkt->data)
            av_free_packet(pkt);
        memset(pkt_temp, 0, sizeof(*pkt_temp));
        pkt_temp->stream_index = -1;

        if (audioq->abort_request) {
            return -1;
        }

        /*if (is->audioq.nb_packets == 0)
            SDL_CondSignal(is->continue_read_thread);*/

        /* read next packet */
        if ((packet_queue_get(audioq, pkt, 1, &is->audio_pkt_temp_serial)) < 0)
            return -1;

        if (pkt->data == flush_pkt.data) {
            avcodec_flush_buffers(dec);
            is->audio_buf_frames_pending = 0;
            is->audio_frame_next_pts = AV_NOPTS_VALUE;
            /*if ((is->ic->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) && !is->ic->iformat->read_seek)
                is->audio_frame_next_pts = is->audio_st->start_time;*/
        }

        *pkt_temp = *pkt;
    }
}