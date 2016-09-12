#include "AudioResample.h"

struct AudioSampleParam * AR_InitAudioSample(struct AudioParams audio_src)
{
	struct AudioSampleParam * ret = (struct AudioSampleParam*)av_malloc(sizeof(struct AudioSampleParam));
	memset(ret,0,sizeof(struct AudioSampleParam));
	ret->audio_src = audio_src;
	return ret;
}
void AR_UninitAudioSample(struct AudioSampleParam **pasp)
{
	if(pasp == NULL) return;
	struct AudioSampleParam * asp = *pasp;
	if(asp != NULL)
	{
		if(asp->swr_ctx != NULL)
		{
			swr_free(&asp->swr_ctx);
			asp->swr_ctx = NULL;
		}
		if(asp->audio_buf1 != NULL)
		{
			av_free(asp->audio_buf1);
			asp->audio_buf1 = NULL;
		}
		asp->audio_buf1_size = 0;

		av_free(asp);
	}
	*pasp = NULL;
}

struct AudioParams AR_GetAudioParams(struct AudioSampleParam *pasp)
{
	struct AudioParams audio_src = {0};
	if(pasp != NULL)
	{
		audio_src = pasp->audio_src;
	}
	return audio_src;
}
//检查是否需要重采样
//-1:出错，0 无需，1 需要冲采样
int AR_ChechAudioSamle(struct AudioSampleParam *pasp,AVFrame *frame,struct AudioParams audio_tgt,int wanted_nb_samples)
{
	int64_t dec_channel_layout = 0;
	if(pasp == NULL) return 0;
	if(frame == NULL) return 0;

	dec_channel_layout =
        (frame->channel_layout && av_frame_get_channels(frame) == av_get_channel_layout_nb_channels(frame->channel_layout)) ?
        frame->channel_layout : av_get_default_channel_layout(av_frame_get_channels(frame));
	if (frame->format        != pasp->audio_src.fmt            ||
        dec_channel_layout       != pasp->audio_src.channel_layout ||
        frame->sample_rate   != pasp->audio_src.freq           ||
        (wanted_nb_samples       != frame->nb_samples && !pasp->swr_ctx)) {
        swr_free(&pasp->swr_ctx);
        pasp->swr_ctx = swr_alloc_set_opts(NULL,
                                            audio_tgt.channel_layout, audio_tgt.fmt, audio_tgt.freq,
                                            dec_channel_layout,(AVSampleFormat)frame->format, frame->sample_rate,
                                            0, NULL);
        if (!pasp->swr_ctx || swr_init(pasp->swr_ctx) < 0) {
            av_log(NULL, AV_LOG_ERROR,
                    "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
                    frame->sample_rate, av_get_sample_fmt_name((AVSampleFormat)frame->format), av_frame_get_channels(frame),
                    audio_tgt.freq, av_get_sample_fmt_name(audio_tgt.fmt), audio_tgt.channels);
            return -1;
        }
        pasp->audio_src.channel_layout = dec_channel_layout;
        pasp->audio_src.channels       = av_frame_get_channels(frame);
        pasp->audio_src.freq = frame->sample_rate;
        pasp->audio_src.fmt = (AVSampleFormat)frame->format;
    }
	if (pasp->swr_ctx) return 1;
	return 0;
}
//音频重采样
int AR_AudioResample(struct AudioSampleParam *pasp,AVFrame *frame,struct AudioParams audio_tgt,int wanted_nb_samples,AudioDataBuffer *pout)
{
	if(pasp == NULL) return 0;
	if(pasp->swr_ctx == NULL) return 0;
	if(frame == NULL) return 0;
	

	const uint8_t **in = (const uint8_t **)frame->extended_data;
    uint8_t **out = &pasp->audio_buf1;
    int out_count = (int64_t)wanted_nb_samples * audio_tgt.freq / frame->sample_rate + 256;
    int out_size  = av_samples_get_buffer_size(NULL, audio_tgt.channels, out_count, audio_tgt.fmt, 0);
    int len2;
    if (out_size < 0) {
        av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
        return 0;
    }
    if (wanted_nb_samples != frame->nb_samples) {
        if (swr_set_compensation(pasp->swr_ctx, (wanted_nb_samples - frame->nb_samples) * audio_tgt.freq / frame->sample_rate,
                                    wanted_nb_samples * audio_tgt.freq / frame->sample_rate) < 0) {
            av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
            return 0;
        }
    }
    av_fast_malloc(&pasp->audio_buf1, &pasp->audio_buf1_size, out_size);
    if (!pasp->audio_buf1)
        return AVERROR(ENOMEM);
    len2 = swr_convert(pasp->swr_ctx, out, out_count, in, frame->nb_samples);
    if (len2 < 0) {
        av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
        return 0;
    }
    if (len2 == out_count) {
        av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too small\n");
        swr_init(pasp->swr_ctx);
    }
    int resampled_data_size = len2 * audio_tgt.channels * av_get_bytes_per_sample(audio_tgt.fmt);

	if(pout != NULL){
		pout->audio_buf = pasp->audio_buf1;
		pout->audio_buf_size = resampled_data_size;
	}
	return resampled_data_size;
}