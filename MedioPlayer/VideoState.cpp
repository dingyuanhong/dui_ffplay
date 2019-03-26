#include "VideoState.h"
#include "SynchronizeClock.h"
#include "VideoPictureRender.h"

int is_realtime(AVFormatContext *s)
{
    if(   !strcmp(s->iformat->name, "rtp")
       || !strcmp(s->iformat->name, "rtsp")
       || !strcmp(s->iformat->name, "sdp")
    )
        return 1;

    if(s->pb && (   !strncmp(s->filename, "rtp:", 4)
                 || !strncmp(s->filename, "udp:", 4)
                )
    )
        return 1;
    return 0;
}
int decode_interrupt_cb(void *ctx)
{
    VideoState *is = (VideoState*)ctx;
    return is->abort_request;
}
/* seek in the stream */
static void stream_seek(VideoState *is, int64_t pos, int64_t rel, int seek_by_bytes)
{
    if (!is->seek_req) {
        is->seek_pos = pos;
        is->seek_rel = rel;
        is->seek_flags &= ~AVSEEK_FLAG_BYTE;
        if (seek_by_bytes)
            is->seek_flags |= AVSEEK_FLAG_BYTE;
        is->seek_req = 1;
        //SDL_CondSignal(is->continue_read_thread);
    }
}

void WaitTimeout(VideoState *is,SDL_mutex *wait_mutex,int ms)
{
	SDL_LockMutex(wait_mutex);
    //SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, ms);
    SDL_UnlockMutex(wait_mutex);
	TimeWait(ms);
}
int CheckPaused(VideoState *is)
{
	AVFormatContext *ic = is->ic;
	if (is->paused != is->last_paused) {
        is->last_paused = is->paused;
        if (is->paused)
            is->read_pause_return = av_read_pause(ic);
        else
            av_read_play(ic);
    }
#if CONFIG_RTSP_DEMUXER || CONFIG_MMSH_PROTOCOL
    if (is->paused &&
            (!strcmp(is->ic->iformat->name, "rtsp") ||
                (is->ic->pb && !strncmp(is->ic->filename, "mmsh:", 5)))) {
        return 1;
    }
#endif
	return 0;
}

int CheckSeekPos(VideoState *is)
{
    int64_t seek_target = is->seek_pos;
    int64_t seek_min    = is->seek_rel > 0 ? seek_target - is->seek_rel + 2: INT64_MIN;
    int64_t seek_max    = is->seek_rel < 0 ? seek_target - is->seek_rel - 2: INT64_MAX;
// FIXME the +-2 is due to rounding being not done in the correct direction in generation
//      of the seek_pos/seek_rel variables

    int ret = avformat_seek_file(is->ic, -1, seek_min, seek_target, seek_max, is->seek_flags);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR,
                "%s: error while seeking\n", is->ic->filename);
    } else {
        if (is->audio_stream >= 0) {
			packet_queue_empty(&is->audioq);
        }
        if (is->subtitle_stream >= 0) {
			packet_queue_empty(&is->subtitleq);
        }
        if (is->video_stream >= 0) {
			packet_queue_empty(&is->videoq);
        }
    }
	return ret;
}
int Queue_Attachments_Req(VideoState *is)
{
	int ret = 1;
	if (is->video_st && is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
		AVPacket copy;
		if (ret = (av_copy_packet(&copy, &is->video_st->attached_pic)) < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "Queue_Attachments_Req:av_copy_packet attached_pic FAILED!\n");
			return ret;
		}
		packet_queue_put(&is->videoq, &copy);
	}
	return ret;
}
int CheckQueueStatus(VideoState *is,int infinite_buffer)
{
	/* if the queue are full, no need to read more */
    if (infinite_buffer<1 &&
            (is->audioq.size + is->videoq.size + is->subtitleq.size > MAX_QUEUE_SIZE
        || (   (is->audioq   .nb_packets > MIN_FRAMES || is->audio_stream < 0 || is->audioq.abort_request)
            && (is->videoq   .nb_packets > MIN_FRAMES || is->video_stream < 0 || is->videoq.abort_request
                /*|| (is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)*/)
            && (is->subtitleq.nb_packets > MIN_FRAMES || is->subtitle_stream < 0 || is->subtitleq.abort_request)))) {
		return 1;
    }
	return -1;
}
int IsQueueEmpty(VideoState *is)
{
	return is->audioq.size + is->videoq.size + is->subtitleq.size == 0;
}
void ClearPacketQueue(VideoState *is)
{
	if (is->video_stream >= 0) {
		AVPacket pkt1, *pkt = &pkt1;
        av_init_packet(pkt);
        pkt->data = NULL;
        pkt->size = 0;
        pkt->stream_index = is->video_stream;
        packet_queue_put(&is->videoq, pkt);
    }
    if (is->audio_stream >= 0) {
		AVPacket pkt1, *pkt = &pkt1;
        av_init_packet(pkt);
        pkt->data = NULL;
        pkt->size = 0;
        pkt->stream_index = is->audio_stream;
        packet_queue_put(&is->audioq, pkt);
    }
}
void AddPacket(VideoState *is,AVPacket *pkt,int64_t start_time,int64_t &duration)
{
	AVFormatContext *ic = is->ic;
	/* check if packet is in play range specified by user, then queue, otherwise discard */
    int64_t stream_start_time = ic->streams[pkt->stream_index]->start_time;
    int pkt_in_play_range = duration == AV_NOPTS_VALUE ||
            (pkt->pts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
            av_q2d(ic->streams[pkt->stream_index]->time_base) -
            (double)(start_time != AV_NOPTS_VALUE ? start_time : 0) / 1000000
            <= ((double)duration / 1000000);
    if (pkt->stream_index == is->audio_stream && pkt_in_play_range) {
        packet_queue_put(&is->audioq, pkt);
    } else if (pkt->stream_index == is->video_stream && pkt_in_play_range
                && !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
        packet_queue_put(&is->videoq, pkt);
    } else if (pkt->stream_index == is->subtitle_stream && pkt_in_play_range) {
        packet_queue_put(&is->subtitleq, pkt);
    } else {
        av_free_packet(pkt);
    }
}
int IsAboutRequest(VideoState *is)
{
	return is->abort_request;
}

VideoState *CreateVideoState()
{
	VideoState *is = (VideoState*)av_mallocz(sizeof(VideoState));
	memset(is,0,sizeof(VideoState));
    packet_queue_init(&is->videoq);
    packet_queue_init(&is->audioq);
    packet_queue_init(&is->subtitleq);

	is->seek_flags = AVSEEK_FLAG_BYTE;

	is->audio_stream = -1;
	is->subtitle_stream = -1;
	is->subtitle_stream = -1;
	is->show_mode = SHOW_MODE_NONE;
	return is;
}
void FreeVideoState(VideoState **is)
{
	if(is == NULL) return;
	if(*is != NULL)
	{
		packet_queue_abort(&(*is)->videoq);
		packet_queue_abort(&(*is)->audioq);
		packet_queue_abort(&(*is)->subtitleq);

		packet_queue_destroy(&(*is)->videoq);
		packet_queue_destroy(&(*is)->audioq);
		packet_queue_destroy(&(*is)->subtitleq);

		if ((*is)->video_st != NULL) {
			avcodec_close((*is)->video_st->codec);
		}

		if ((*is)->audio_st != NULL) {
			avcodec_close((*is)->audio_st->codec);
		}

		if ((*is)->subtitle_st != NULL) {
			avcodec_close((*is)->subtitle_st->codec);
		}

		if ((*is)->ic) {
			avformat_close_input(&(*is)->ic);
		}
		av_free(*is);
	}
	*is = NULL;
}

void InitStream(VideoState *is,StreamConfigure *cf)
{
	is->video_stream = cf->st_index[AVMEDIA_TYPE_VIDEO];
	is->audio_stream = cf->st_index[AVMEDIA_TYPE_AUDIO];;
	is->subtitle_stream=cf->st_index[AVMEDIA_TYPE_SUBTITLE];
}

int OpenStream(VideoState *is,std::string strName,StreamConfigure *cf)
{
    AVFormatContext *ic = NULL;

	ic = avformat_alloc_context();
    ic->interrupt_callback.callback = decode_interrupt_cb;
    ic->interrupt_callback.opaque = is;

    int err = avformat_open_input(&ic, strName.c_str(), NULL, NULL);
    if (err < 0) {
        return -1;
    }
	is->ic = ic;

	//自动添加PTS，即使没有PTS
    /*if (genpts)
        ic->flags |= AVFMT_FLAG_GENPTS;*/

    int orig_nb_streams = ic->nb_streams;

	AVDictionary **opts = NULL;
    err = avformat_find_stream_info(ic, opts);
    if (err < 0) {
		return -1;
    }
	if(opts != NULL){
		for (int i = 0; i < orig_nb_streams; i++)
			av_dict_free(&opts[i]);
		av_freep(&opts);
	}
    if (ic->pb)
        ic->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use url_feof() to test for the end

    if (cf->seek_by_bytes < 0)
        cf->seek_by_bytes = !!(ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", ic->iformat->name);

    //is->max_frame_duration = (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;
	//切换流至指定位置
	int ret = -1;
    /* if seeking requested, we execute it */
    if (cf->start_time != AV_NOPTS_VALUE) {
        int64_t timestamp;

        timestamp = cf->start_time;
        /* add the stream start time */
        if (ic->start_time != AV_NOPTS_VALUE)
            timestamp += ic->start_time;
        ret = avformat_seek_file(ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);
        if (ret < 0) {
            /*av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
                    is->filename, (double)timestamp / AV_TIME_BASE);*/
        }
    }

    is->realtime = is_realtime(ic);

	int wanted_stream[AVMEDIA_TYPE_NB] = {-1,-1,0,-1};
	//int st_index[AVMEDIA_TYPE_NB] = {0};
	memset(cf->st_index,-1,sizeof(int)*AVMEDIA_TYPE_NB);

    for (int i = 0; i < ic->nb_streams; i++)
        ic->streams[i]->discard = AVDISCARD_ALL;
    if (!cf->video_disable)
        cf->st_index[AVMEDIA_TYPE_VIDEO] =
            av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO,
                                wanted_stream[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
    if (!cf->audio_disable)
        cf->st_index[AVMEDIA_TYPE_AUDIO] =
            av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO,
                                wanted_stream[AVMEDIA_TYPE_AUDIO],
                                cf->st_index[AVMEDIA_TYPE_VIDEO],
                                NULL, 0);
    if (!cf->video_disable && !cf->subtitle_disable)
        cf->st_index[AVMEDIA_TYPE_SUBTITLE] =
            av_find_best_stream(ic, AVMEDIA_TYPE_SUBTITLE,
                                wanted_stream[AVMEDIA_TYPE_SUBTITLE],
                                (cf->st_index[AVMEDIA_TYPE_AUDIO] >= 0 ?
                                 cf->st_index[AVMEDIA_TYPE_AUDIO] :
                                 cf->st_index[AVMEDIA_TYPE_VIDEO]),
                                NULL, 0);
    /*if (show_status) {
        av_dump_format(ic, 0, is->filename, 0);
    }*/

    //is->show_mode = show_mode;

	int video_stream = -1;
	int audio_stream = -1;
    /* open the streams */
	if (cf->st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
		int nRet = audio_stream_component_open(ic,&is->audio_st, cf->st_index[AVMEDIA_TYPE_AUDIO],cf->audiopm);
		if(nRet != -1)
		{
			audio_stream = nRet;
			packet_queue_start(&is->audioq);
		}
	}
	if (cf->st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
		video_stream = stream_component_open(ic,&is->video_st, cf->st_index[AVMEDIA_TYPE_VIDEO]);
		packet_queue_start(&is->videoq);
	}
	//用于匹配打开模式，当无视频数据时使用SHOW_MODE_RDFT方式处理
	if (is->show_mode == SHOW_MODE_NONE)
		is->show_mode = video_stream >= 0 ? SHOW_MODE_VIDEO : SHOW_MODE_WAVES;

	if (cf->st_index[AVMEDIA_TYPE_SUBTITLE] >= 0) {
		stream_component_open(ic,&is->subtitle_st, cf->st_index[AVMEDIA_TYPE_SUBTITLE]);
		packet_queue_start(&is->subtitleq);
	}

    if (video_stream < 0 && audio_stream < 0) {
        av_log(NULL, AV_LOG_FATAL, "Failed to open file '%s' or configure filtergraph\n",
               "");

        return -1;
    }
	InitStream(is,cf);
	return 1;
}

int LoadStreams(VideoState *is,SynchronizeClockState *scs,StreamConfigure *cf)
{
    AVFormatContext *ic = is->ic;
    int ret;
    AVPacket pkt1, *pkt = &pkt1;
    int eof = 0;
    int pkt_in_play_range = 0;
    int orig_nb_streams = ic->nb_streams;;

	SDL_mutex *wait_mutex = SDL_CreateMutex();

	//当数据为实时模式时不做缓存处理

	if (cf->infinite_buffer < 0 && is->realtime)
        cf->infinite_buffer = 1;

	if(is->eof == 1){
		//切换流至指定位置
		int ret = -1;
		/* if seeking requested, we execute it */
		{
			int64_t timestamp = 0;
			if (cf->start_time != AV_NOPTS_VALUE)
				timestamp = cf->start_time;
			/* add the stream start time */
			if (ic->start_time != AV_NOPTS_VALUE)
				timestamp += ic->start_time;
			ret = avformat_seek_file(ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);
			if (ret < 0) {
				/*av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
						is->filename, (double)timestamp / AV_TIME_BASE);*/
			}
		}
		is->eof = 0;
	}

    for (;;) {
		//检查是否终止
        if (IsAboutRequest(is)) break;
		//检查是否暂停
        if(CheckPaused(is))
		{
			/* wait 10 ms to avoid trying to get another packet */
			/* XXX: horrible */
			TimeWait(10);
			continue;
		}
		//检查是否设置切换读取位置
		int queue_attachments_req = 0;
		if (is->seek_req){
			ret = CheckSeekPos(is);

			if(ret >= 0){
				if(scs != NULL) set_extclk_clock(is->seek_flags,is->seek_pos,scs);
			}
			is->seek_req = 0;
			queue_attachments_req = 1;
			eof = 0;

			if (is->paused) step_to_next_frame(is,scs->vs,scs);
		}
        if (queue_attachments_req) {
			if(ret = Queue_Attachments_Req(is) < 0)
			{
				SDL_DestroyMutex(wait_mutex);
				return ret;
			}
		}
		//检查队列数据大小状态，是否为无限内存
		if(CheckQueueStatus(is,cf->infinite_buffer) != -1)
		{
			/* wait 10 ms */
			WaitTimeout(is,wait_mutex,10);
			continue;
		}
        //检查是否读取到文件结尾
        if (eof) {
			//追加NULL包标记读取文件结束
			ClearPacketQueue(is);
			TimeWait(10);
            if (IsQueueEmpty(is)) {
                if (cf->loop != 1 && (!cf->loop || --cf->loop)) {
                    stream_seek(is, cf->start_time != AV_NOPTS_VALUE ? cf->start_time : 0, 0, 0);
                } else if (cf->autoexit) {
                    ret = AVERROR_EOF;
					is->eof = 1;
					SDL_DestroyMutex(wait_mutex);
                    return ret;
                }
            }
            eof=0;
            continue;
        }
		//读取包
        ret = av_read_frame(ic, pkt);
        if (ret < 0) {
            if (ret == AVERROR_EOF || url_feof(ic->pb))
                eof = 1;
            if (ic->pb && ic->pb->error)
                break;
			WaitTimeout(is,wait_mutex,10);
            continue;
        }
		//讲读取的数据包添加到文件中
		AddPacket(is,pkt,cf->start_time,cf->duration);
    }
	SDL_DestroyMutex(wait_mutex);
	return ret;
}

int AbortStream(VideoState *is)
{
	if(is == NULL) return -1;
	is->abort_request = 1;
	packet_queue_abort(&is->audioq);
	packet_queue_abort(&is->videoq);
	packet_queue_abort(&is->subtitleq);

	return 0;
}

void InitStreamConfigure(StreamConfigure *is)
{
	if(is == NULL) return;
	is->st_index[AVMEDIA_TYPE_VIDEO] = -1;
	is->st_index[AVMEDIA_TYPE_AUDIO] = -1;
	is->st_index[AVMEDIA_TYPE_DATA] = -1;
	is->st_index[AVMEDIA_TYPE_SUBTITLE] = -1;

	is->start_time = AV_NOPTS_VALUE;
	is->loop = 1;
	is->autoexit = 1;
	is->duration = AV_NOPTS_VALUE;
	is->infinite_buffer = -1;
}
void FreeStreamConfigure(StreamConfigure **pis)
{
	if(pis == NULL) return;
	if(*pis != NULL)
	{
		free(*pis);
	}
	*pis = NULL;
}

void seek_stream_pos(VideoState *is,int64_t nPos)
{
	int x = nPos;
	double frac;
	int seek_by_bytes = 0;
	if (seek_by_bytes || is->ic->duration <= 0) {
        uint64_t size =  avio_size(is->ic->pb);
        stream_seek(is, size*x , 0, 1);
    } else {
        int64_t ts;
        int ns, hh, mm, ss;
        int tns, thh, tmm, tss;
        tns  = is->ic->duration / 1000000LL;
        thh  = tns / 3600;
        tmm  = (tns % 3600) / 60;
        tss  = (tns % 60);
		frac = x;
        ns   = x;
        hh   = ns / 3600;
        mm   = (ns % 3600) / 60;
        ss   = (ns % 60);
        av_log(NULL, AV_LOG_INFO,
                "Seek to %2.0f%% (%2d:%02d:%02d) of total duration (%2d:%02d:%02d)       \n", frac*100,
                hh, mm, ss, thh, tmm, tss);
        ts = x*1000000LL;
        if (is->ic->start_time != AV_NOPTS_VALUE)
            ts += is->ic->start_time;
        stream_seek(is, ts, 0, 0);
    }
}
#include "AudioProcesser.h"
#include "VideoDef.h"
void seek_pos(VideoState *is,SynchronizeClockState *scs,AudioSynchronize *as,VideoSynchronize *vrs,double incr)
{
	double pos = 0.0;
	int seek_by_bytes = 0;
	if (seek_by_bytes) {
        if (is->video_stream >= 0 && vrs != NULL && vrs->video_current_pos >= 0) {
            pos = vrs->video_current_pos;
		} else if (is->audio_stream >= 0 && as != NULL && as->audio_current_pos >= 0) {
            pos = as->audio_current_pos;
        } else
            pos = avio_tell(is->ic->pb);
        if (is->ic->bit_rate)
            incr *= is->ic->bit_rate / 8.0;
        else
            incr *= 180000.0;
        pos += incr;
        stream_seek(is, pos, incr, 1);
    } else {
        pos = get_master_clock(scs);
        if (isnan(pos))
            pos = (double)is->seek_pos / AV_TIME_BASE;
        pos += incr;
        if (is->ic->start_time != AV_NOPTS_VALUE && pos < is->ic->start_time / (double)AV_TIME_BASE)
            pos = is->ic->start_time / (double)AV_TIME_BASE;
        stream_seek(is, (int64_t)(pos * AV_TIME_BASE), (int64_t)(incr * AV_TIME_BASE), 0);
    }
}
void seek_pos(VideoState *is,SynchronizeClockState *scs,double incr)
{
	if(scs == NULL){
		seek_pos(is,NULL,NULL,NULL,incr);
	}else{
		seek_pos(is,scs,scs->as,scs->vs,incr);
	}
}