#ifndef STREAMCOMPONENT_H
#define STREAMCOMPONENT_H

#include "ffmpeg.h"


typedef struct Audio_Param
{
	int sample_rate;
	int nb_channels;
	int64_t channel_layout;
}Audio_Param;

/* open a given stream. Return 0 if OK */
static int audio_stream_component_open(AVFormatContext *ic,AVStream **st, int stream_index,Audio_Param &param)
{
    AVCodecContext *avctx = NULL;
    AVCodec *codec = NULL;
    const char *forced_codec_name = NULL;
    AVDictionary *opts = NULL;
    AVDictionaryEntry *t = NULL;
    //int sample_rate, nb_channels;
    //int64_t channel_layout;
    int ret;

    if (stream_index < 0 || stream_index >= ic->nb_streams)
        return -1;
    avctx = ic->streams[stream_index]->codec;

    codec = avcodec_find_decoder(avctx->codec_id);
	
    if (forced_codec_name)
        codec = avcodec_find_decoder_by_name(forced_codec_name);
    if (!codec) {
        if (forced_codec_name) av_log(NULL, AV_LOG_WARNING,
                                      "No codec could be found with name '%s'\n", forced_codec_name);
        else                   av_log(NULL, AV_LOG_WARNING,
                                      "No codec could be found with id %d\n", avctx->codec_id);
        return -1;
    }

    avctx->codec_id = codec->id;
    //avctx->workaround_bugs   = workaround_bugs;
    //avctx->lowres            = lowres;
    if(avctx->lowres > codec->max_lowres){
        av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",
                codec->max_lowres);
        avctx->lowres= codec->max_lowres;
    }
    //avctx->error_concealment = error_concealment;

    if(avctx->lowres) avctx->flags |= CODEC_FLAG_EMU_EDGE;
    //if (fast)   avctx->flags2 |= CODEC_FLAG2_FAST;
    if(codec->capabilities & CODEC_CAP_DR1)
        avctx->flags |= CODEC_FLAG_EMU_EDGE;

    if (avcodec_open2(avctx, codec, &opts) < 0)
        return -1;
    if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
        av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
        return AVERROR_OPTION_NOT_FOUND;
    }
	if(avctx->codec_type != AVMEDIA_TYPE_AUDIO){
		return -1;
	}
    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;

#if CONFIG_AVFILTER
    {
        AVFilterLink *link;

        is->audio_filter_src.freq           = avctx->sample_rate;
        is->audio_filter_src.channels       = avctx->channels;
        is->audio_filter_src.channel_layout = get_valid_channel_layout(avctx->channel_layout, avctx->channels);
        is->audio_filter_src.fmt            = avctx->sample_fmt;
        if ((ret = configure_audio_filters(is, afilters, 0)) < 0)
            return ret;
        link = is->out_audio_filter->inputs[0];
        sample_rate    = link->sample_rate;
        nb_channels    = link->channels;
        channel_layout = link->channel_layout;
    }
#else
    param.sample_rate    = avctx->sample_rate;
    param.nb_channels    = avctx->channels;
    param.channel_layout = avctx->channel_layout;
#endif
    //is->audio_stream = stream_index;
    //is->audio_st = ic->streams[stream_index];
	*st = ic->streams[stream_index];
    return stream_index;
}
/* open a given stream. Return 0 if OK */
static int stream_component_open(AVFormatContext *ic,AVStream **st, int stream_index)
{
    AVCodecContext *avctx = NULL;
    AVCodec *codec = NULL;
    const char *forced_codec_name = NULL;
    AVDictionary *opts = NULL;
    AVDictionaryEntry *t = NULL;

    int ret;

    if (stream_index < 0 || stream_index >= ic->nb_streams)
        return -1;
    avctx = ic->streams[stream_index]->codec;

    codec = avcodec_find_decoder(avctx->codec_id);

    if (forced_codec_name)
        codec = avcodec_find_decoder_by_name(forced_codec_name);
    if (!codec) {
        if (forced_codec_name) av_log(NULL, AV_LOG_WARNING,
                                      "No codec could be found with name '%s'\n", forced_codec_name);
        else                   av_log(NULL, AV_LOG_WARNING,
                                      "No codec could be found with id %d\n", avctx->codec_id);
        return -1;
    }

    avctx->codec_id = codec->id;
    //avctx->workaround_bugs   = workaround_bugs;
    //avctx->lowres            = lowres;
    if(avctx->lowres > codec->max_lowres){
        av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",
                codec->max_lowres);
        avctx->lowres= codec->max_lowres;
    }
    //avctx->error_concealment = error_concealment;

    if(avctx->lowres) avctx->flags |= CODEC_FLAG_EMU_EDGE;
    //if (fast)   avctx->flags2 |= CODEC_FLAG2_FAST;
    if(codec->capabilities & CODEC_CAP_DR1)
        avctx->flags |= CODEC_FLAG_EMU_EDGE;

    //opts = filter_codec_opts(codec_opts, avctx->codec_id, ic, ic->streams[stream_index], codec);
    /*if (!av_dict_get(opts, "threads", NULL, 0))
        av_dict_set(&opts, "threads", "auto", 0);
    if (avctx->lowres)
        av_dict_set(&opts, "lowres", av_asprintf("%d", avctx->lowres), AV_DICT_DONT_STRDUP_VAL);
    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
        av_dict_set(&opts, "refcounted_frames", "1", 0);*/
    if (avcodec_open2(avctx, codec, &opts) < 0)
        return -1;
    if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
        av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
        return AVERROR_OPTION_NOT_FOUND;
    }

    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
	*st = ic->streams[stream_index];
    return stream_index;
}

#include <Windows.h>
static void TimeWait(int ms)
{
	::Sleep(ms);
}

#endif