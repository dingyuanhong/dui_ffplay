#include "VideoProcesser.h"
#include "MClock.h"
#include "SynchronizeClock.h"
#include "VideoPictureRender.h"
#include "SDLVideoRender.h"
//转换AVFrame数据为屏幕数据
int queue_picture(VideoState *is,VideoPictureBuffer *vpr,ScreenRender *sr, AVFrame *src_frame, double pts, int64_t pos, int serial)
{
#if defined(DEBUG_SYNC) && 0
    printf("frame_type=%c pts=%0.3f\n",
           av_get_picture_type_char(src_frame->pict_type), pts);
#endif

    /* wait until we have space to put a new picture */
    SDL_LockMutex(vpr->pictq_mutex);

    /* keep the last already displayed picture in the queue */
    while (vpr->pictq_size >= VIDEO_PICTURE_QUEUE_SIZE - 1 &&
           !is->videoq.abort_request) {
        SDL_CondWait(vpr->pictq_cond, vpr->pictq_mutex);
    }
    SDL_UnlockMutex(vpr->pictq_mutex);

    if (is->videoq.abort_request)
        return -1;
	int nRet = -1;
	if(sr != NULL){
		if(sr->fill_picture != NULL){
			nRet = sr->fill_picture(sr,vpr,src_frame,pts,pos,serial);
		}
	}
	
	if (is->videoq.abort_request)
		return -1;
    return nRet;
}

int Isflushpkt(AVPacket *pkt)
{
	if (pkt->data == flush_pkt.data) return 1;
	return 0;
}

//获取一帧解码数据
int get_video_frame(VideoState *is,VideoPictureBuffer *vrp,VideoSynchronize *vs, AVFrame *frame, AVPacket *pkt, int *serial)
{
    int got_picture;

    if (packet_queue_get(&is->videoq, pkt, 1, serial) < 0)
        return -1;

    if (pkt->data == flush_pkt.data) {
        avcodec_flush_buffers(is->video_st->codec);

        SDL_LockMutex(vrp->pictq_mutex);
        // Make sure there are no long delay timers (ideally we should just flush the queue but that's harder)
        while (vrp->pictq_size && !is->videoq.abort_request) {
            SDL_CondWait(vrp->pictq_cond, vrp->pictq_mutex);
        }
        vs->video_current_pos = -1;
        vs->frame_last_pts = AV_NOPTS_VALUE;
        vs->frame_last_duration = 0;

        vs->frame_last_dropped_pts = AV_NOPTS_VALUE;

        SDL_UnlockMutex(vrp->pictq_mutex);
        return 0;
    }

    if(avcodec_decode_video2(is->video_st->codec, frame, &got_picture, pkt) < 0)
        return 0;

    if (got_picture) {
        int ret = 1;
        double dpts = NAN;

        if (vs->decoder_reorder_pts == -1) {
            frame->pts = av_frame_get_best_effort_timestamp(frame);
        } else if (vs->decoder_reorder_pts) {
            frame->pts = frame->pkt_pts;
        } else {
            frame->pts = frame->pkt_dts;
        }

        if (frame->pts != AV_NOPTS_VALUE)
            dpts = av_q2d(is->video_st->time_base) * frame->pts;

        frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);
		//可进行丢帧处理,ret = 0;
        return ret;
    }
    return 0;
}

int checkframeanddrop(VideoState *is,VideoPictureBuffer *vpr,VideoSynchronize *vs,SynchronizeClockState *scs,AVPacket *pkt,AVFrame *frame,int serial)
{
	int framedrop = -1;
	int ret = 0;
	double dpts = NAN;
	if (frame->pts != AV_NOPTS_VALUE)
		dpts = av_q2d(is->video_st->time_base) * frame->pts;
	//丢弃数据帧
	if (framedrop>0 || (framedrop && get_master_sync_type(scs) != AV_SYNC_VIDEO_MASTER)) {
		SDL_LockMutex(vpr->pictq_mutex);
		if (vs->frame_last_pts != AV_NOPTS_VALUE && frame->pts != AV_NOPTS_VALUE) {
			double clockdiff = get_clock(&scs->vidclk) - get_master_clock(scs);
			double ptsdiff = dpts - vs->frame_last_pts;
			if (!isnan(clockdiff) && fabs(clockdiff) < AV_NOSYNC_THRESHOLD &&
				!isnan(ptsdiff) && ptsdiff > 0 && ptsdiff < AV_NOSYNC_THRESHOLD &&
				clockdiff + ptsdiff - vs->frame_last_filter_delay < 0 &&
				is->videoq.nb_packets) {
				vs->frame_last_dropped_pos = pkt->pos;
				vs->frame_last_dropped_pts = dpts;
				vs->frame_last_dropped_serial = serial;
				vs->frame_drops_early++;
				av_frame_unref(frame);
				ret = 1;
			}
		}
		SDL_UnlockMutex(vpr->pictq_mutex);
	}
	return ret;
}
int video_process_packet(VideoState *is,VideoPictureBuffer *vrs,ScreenRender *sr,VideoSynchronize *vs,SynchronizeClockState *scs)
{
    AVPacket pkt = { 0 };
    AVFrame *frame = av_frame_alloc();
    double pts;
    int ret;
    int serial = 0;

#if CONFIG_AVFILTER
    AVFilterGraph *graph = avfilter_graph_alloc();
    AVFilterContext *filt_out = NULL, *filt_in = NULL;
    int last_w = 0;
    int last_h = 0;
    enum AVPixelFormat last_format = -2;
    int last_serial = -1;
#endif

    for (;;) {
        while (is->paused && !is->videoq.abort_request)
            SDL_Delay(10);

        avcodec_get_frame_defaults(frame);
        av_free_packet(&pkt);

        ret = get_video_frame(is,vrs,vs, frame, &pkt, &serial);

		if(ret > 0){
			int nRet = checkframeanddrop(is,vrs,vs,scs,&pkt,frame,serial);
			if(nRet == 1){
				ret = 0;//丢弃数据帧
			}
		}
		if(Isflushpkt(&pkt)){
			vs->frame_timer = (double)av_gettime() / 1000000.0;
		}
        if (ret < 0)
            goto the_end;
        if (!ret)
            continue;

#if CONFIG_AVFILTER
        if (   last_w != frame->width
            || last_h != frame->height
            || last_format != frame->format
            || last_serial != serial) {
            av_log(NULL, AV_LOG_DEBUG,
                   "Video frame changed from size:%dx%d format:%s serial:%d to size:%dx%d format:%s serial:%d\n",
                   last_w, last_h,
                   (const char *)av_x_if_null(av_get_pix_fmt_name(last_format), "none"), last_serial,
                   frame->width, frame->height,
                   (const char *)av_x_if_null(av_get_pix_fmt_name(frame->format), "none"), serial);
            avfilter_graph_free(&graph);
            graph = avfilter_graph_alloc();
            if ((ret = configure_video_filters(graph, is, vfilters, frame)) < 0) {
                SDL_Event event;
                event.type = FF_QUIT_EVENT;
                event.user.data1 = is;
                SDL_PushEvent(&event);
                av_free_packet(&pkt);
                goto the_end;
            }
            filt_in  = is->in_video_filter;
            filt_out = is->out_video_filter;
            last_w = frame->width;
            last_h = frame->height;
            last_format = frame->format;
            last_serial = serial;
        }

        ret = av_buffersrc_add_frame(filt_in, frame);
        if (ret < 0)
            goto the_end;
        av_frame_unref(frame);
        avcodec_get_frame_defaults(frame);
        av_free_packet(&pkt);

        while (ret >= 0) {
            is->frame_last_returned_time = av_gettime() / 1000000.0;

            ret = av_buffersink_get_frame_flags(filt_out, frame, 0);
            if (ret < 0) {
                ret = 0;
                break;
            }

            is->frame_last_filter_delay = av_gettime() / 1000000.0 - is->frame_last_returned_time;
            if (fabs(is->frame_last_filter_delay) > AV_NOSYNC_THRESHOLD / 10.0)
                is->frame_last_filter_delay = 0;

            pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(filt_out->inputs[0]->time_base);
            ret = queue_picture(is, frame, pts, av_frame_get_pkt_pos(frame), serial);
            av_frame_unref(frame);
        }
#else
        double pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(is->video_st->time_base);
        ret = queue_picture(is,vrs,sr, frame,pts, pkt.pos, serial);
        av_frame_unref(frame);
#endif

        if (ret < 0)
            goto the_end;
    }
the_end:
	if(!is->abort_request){
		if(is->video_st != NULL)avcodec_flush_buffers(is->video_st->codec);
	}
#if CONFIG_AVFILTER
    avfilter_graph_free(&graph);
#endif
    av_free_packet(&pkt);
    av_frame_free(&frame);
    return 0;
}

int video_process_packet(VideoProcesser *vp,ScreenRender *sr)
{
	return video_process_packet(vp->is,vp->vpb,sr,vp->vs,vp->scs);
}

VideoProcesser *CreateVideoProcesser(double max_frame_duration)
{
	VideoProcesser *vp = (VideoProcesser*)av_malloc(sizeof(VideoProcesser));

	vp->vs = CreateVideoSynchronize(NULL,max_frame_duration);

	vp->vpb = (VideoPictureBuffer*)av_malloc(sizeof(VideoPictureBuffer));
	memset(vp->vpb,0,sizeof(VideoPictureBuffer));
	vp->vpb->pictq_mutex = SDL_CreateMutex();
	vp->vpb->pictq_cond = SDL_CreateCond();

	return vp;
}
void InitVideoProcesser(VideoProcesser *vp,VideoState *is,SynchronizeClockState *scs)
{
	if(vp == NULL) return ;
	if(vp->vs != NULL) InitVideoSynchronize(vp->vs,scs,vp->vs->max_frame_duration);
	vp->is = is;
	vp->scs = scs;
}
void UninitVideoProcesser(VideoProcesser * vp)
{
	if(vp == NULL) return;
	UninitVideoSynchronize(vp->vs);
	if(vp->vpb != NULL){
		for(int i = 0;i< VIDEO_PICTURE_QUEUE_SIZE;i++)
		{
			VideoPicture &vpic = vp->vpb->pictq[i];
			if(vpic.bmp != NULL)
			{
				SDL_FreeYUVOverlay(vpic.bmp);
			}
			vpic.bmp = NULL;
		}
		SDL_DestroyMutex(vp->vpb->pictq_mutex);
		SDL_DestroyCond(vp->vpb->pictq_cond);
		av_free(vp->vpb);
	}
	vp->vpb = NULL;
	vp->is = NULL;
	vp->scs = NULL;
}
void FreeVideoProcesser(VideoProcesser ** pvp)
{
	if(pvp == NULL) return;
	UninitVideoProcesser(*pvp);
	if(*pvp != NULL) av_free(*pvp);
	*pvp = NULL;
}

