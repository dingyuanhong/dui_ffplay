#include "AudioProcesser.h"

#include "SynchronizeClock.h"
#include "AudioSampleDraw.h"

int resampleframe(struct AudioSampleParam *asp,SynchronizeClockState * scs,AVFrame *frame,struct AudioParams audio_tgt,DecodeBuffer &outbuffer)
{
	int len1, data_size, resampled_data_size;
    int64_t dec_channel_layout;
    int got_frame;
    int wanted_nb_samples = 0;

	struct AudioParams audio_src = AR_GetAudioParams(asp);

	data_size = av_samples_get_buffer_size(NULL, av_frame_get_channels(frame),
                                            frame->nb_samples,
                                            (AVSampleFormat)frame->format, 1);

	dec_channel_layout =
		(frame->channel_layout && av_frame_get_channels(frame) == av_get_channel_layout_nb_channels(frame->channel_layout)) ?
		frame->channel_layout : av_get_default_channel_layout(av_frame_get_channels(frame));

	wanted_nb_samples = synchronize_audio(scs, frame->nb_samples,audio_src);

	int nRet = AR_ChechAudioSamle(asp,frame,audio_tgt,wanted_nb_samples);
	if(nRet < 0) return nRet;outbuffer.pts = frame->pts;
	if(nRet == 1){
		AudioDataBuffer out = {0};
		resampled_data_size = AR_AudioResample(asp,frame,audio_tgt,wanted_nb_samples,&out);
		outbuffer.audio_buf = out.audio_buf;
		outbuffer.audio_buf_size = resampled_data_size;
	}else
	{
		resampled_data_size = data_size;
		outbuffer.audio_buf = frame->data[0];
		outbuffer.audio_buf_size = data_size;
	}
	outbuffer.pts = frame->pts;
	return resampled_data_size;
}

int audio_decode_queue(VideoState *vs,SynchronizeClockState * scs,
	AudioQueueDecode *aqd,struct AudioPacketDecode *apd,
	struct AudioSampleParam *pasp,struct AudioParams audio_tgt,
	PacketQueue *audioq,DecodeBuffer &outbuffer)
{
    AVPacket *pkt_temp = &aqd->audio_pkt_temp;
    AVPacket *pkt = &aqd->audio_pkt;
    int ret;
	
    for (;;) {
		while (pkt_temp->stream_index != -1 || apd->audio_buf_frames_pending){
			if (vs->paused){
				return -1;
			}
			//初始化音频内存块
			if (!aqd->frame) {
                if (!(aqd->frame = avcodec_alloc_frame()))
                    return AVERROR(ENOMEM);
            } else {
                av_frame_unref(aqd->frame);
                avcodec_get_frame_defaults(aqd->frame);
            }
			if (audioq->serial != aqd->audio_pkt_temp_serial) break;//退出循环

			struct AudioParams audio_src = AR_GetAudioParams(pasp);
			//解码数据
			ret = DecodePacket(apd,pkt_temp,aqd->frame,audio_src);
			//重采样数据
			if(ret > 0){
				ret = resampleframe(pasp,scs,aqd->frame,audio_tgt,outbuffer);
			}
			if(ret > 0){
				//更新音频时钟数据
				updata_audio_frame_clock(scs,aqd->frame,apd->tb,aqd->audio_pkt_temp_serial);
				return ret;
			}
			if(ret == 0) break;
			return ret;//出错
		}
        /* free the current packet */
        if (pkt->data) av_free_packet(pkt);
        memset(pkt_temp, 0, sizeof(*pkt_temp));
        pkt_temp->stream_index = -1;

        if (audioq->abort_request) {
            return -1;
        }

        /* read next packet */
        if ((packet_queue_get(audioq, pkt, 1, &aqd->audio_pkt_temp_serial)) < 0)
            return -1;

        if (pkt->data == flush_pkt.data) {
			BeginDecoder(apd,AV_NOPTS_VALUE);
            if ((vs->ic->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) && !vs->ic->iformat->read_seek)
				SetNextPts(apd,vs->audio_st->start_time);
		}else{
			if(scs != NULL && scs->as != NULL ) scs->as->audio_current_pos = pkt->pos;
		}

        *pkt_temp = *pkt;
    }
}

void Uninit_AudioQueueDecode(AudioQueueDecode **paqd)
{
	if(paqd == NULL) return;
	AudioQueueDecode * aqd = *paqd;
	if(aqd != NULL)
	{
		if(aqd->frame != NULL)
		{
			avcodec_free_frame(&aqd->frame);
		}
		if(aqd->audio_pkt.data != NULL)
		{
			av_free_packet(&aqd->audio_pkt);
			memset(&aqd->audio_pkt, 0, sizeof(AVPacket));
			memset(&aqd->audio_pkt_temp, 0, sizeof(AVPacket));
		}
		if(aqd->audio_pkt_temp.data != NULL)
		{
			av_free_packet(&aqd->audio_pkt_temp);
			memset(&aqd->audio_pkt, 0, sizeof(AVPacket));
			memset(&aqd->audio_pkt_temp, 0, sizeof(AVPacket));
		}
	}
	*paqd = NULL;
}

AudioProcesser *CreateAudioProcesser(AVStream *audio_st,struct AudioParams audio_params)
{
	AudioProcesser *is = (AudioProcesser*)malloc(sizeof(AudioProcesser));
	memset(is,0,sizeof(AudioProcesser));

	is->dec = InitDecoder(audio_st);
	is->pasp = AR_InitAudioSample(audio_params);
	is->qdec = (AudioQueueDecode*)malloc(sizeof(AudioQueueDecode));
	memset(is->qdec,0,sizeof(AudioQueueDecode));
	return is;
}

void FreeAudioProcesser(AudioProcesser **is)
{
	if(is != NULL){
		AudioProcesser *ap = *is;
		if(ap != NULL){
			Uninit_AudioQueueDecode(&ap->qdec);
			UninitDecoder(&ap->dec);
			AR_UninitAudioSample(&ap->pasp);
			free(ap);
		}
		*is = NULL;
	}
}

void FillAudioDeviceBuffer(AudioProcesser *ap,struct AudioParams ap_render,int audio_buf_size,Uint8 *stream, int len,int volume)
{
	struct AudioParams audio_tgt = ap_render;

	int frame_size = av_samples_get_buffer_size(NULL, audio_tgt.channels, 1, audio_tgt.fmt, 1);

    ap->vs->audio_callback_time = av_gettime();

    while (len > 0) {
		if (ap->vs->paused){
			return;
		}
		if(ap->vs->eof){
			if(ap->vs->audioq.size <= 0) return ;
		}
        if (ap->audio_buf_index >= ap->outbuffer.audio_buf_size) {
			int audio_size = audio_decode_queue(ap->vs,ap->scs,ap->qdec,ap->dec,ap->pasp,audio_tgt,&ap->vs->audioq,ap->outbuffer);
			if (audio_size < 0) {
				/* if error, just output silence */
				ap->outbuffer.audio_buf      = ap->silence_buf;
				ap->outbuffer.audio_buf_size = sizeof(ap->silence_buf) / frame_size * frame_size;
			} else {
				if (ap->vs->show_mode != SHOW_MODE_VIDEO)
					update_sample_display(&ap->scs->sample_array, (int16_t *)ap->outbuffer.audio_buf, audio_size);
			}
			ap->audio_buf_index = 0;
        }
        int len1 = ap->outbuffer.audio_buf_size - ap->audio_buf_index;
        if (len1 > len)
            len1 = len;
		if(ap->outbuffer.audio_buf != NULL){
			//memcpy(stream, (uint8_t *)ap->outbuffer.audio_buf + ap->audio_buf_index, len1);
			SDL_MixAudio(stream, (uint8_t *)ap->outbuffer.audio_buf + ap->audio_buf_index, len1,volume);
		}
        len -= len1;
        stream += len1;
        ap->audio_buf_index += len1;
    }
    int bytes_per_sec = audio_tgt.freq * audio_tgt.channels * av_get_bytes_per_sample(audio_tgt.fmt);
    int audio_write_buf_size = ap->outbuffer.audio_buf_size - ap->audio_buf_index;
	int audio_hw_buf_size = audio_buf_size;
	//更新音频时钟用于同步
	update_audio_clock(ap->scs,audio_write_buf_size,audio_hw_buf_size,bytes_per_sec,ap->vs->audio_callback_time);
}