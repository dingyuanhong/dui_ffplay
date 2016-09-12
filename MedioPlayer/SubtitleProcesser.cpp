#include "SubtitleProcesser.h"

#include "ffmpeg.h"

#include "YUV.h"

void free_subpicture(SubPicture *sp)
{
    avsubtitle_free(&sp->sub);
}

SubtitleBuffer *CreateSubtile()
{
	SubtitleBuffer* sb = (SubtitleBuffer*)av_malloc(sizeof(SubtitleBuffer));
	memset(sb,0,sizeof(SubtitleBuffer));

	sb->subpq_mutex = SDL_CreateMutex();
	sb->subpq_cond = SDL_CreateCond();

	return sb;
}
void FreeSubtitle(SubtitleBuffer** psb)
{
	if(psb == NULL) return;
	if(*psb == NULL) return;
	SubtitleBuffer* sb = *psb;
	SubPicture subpq[SUBPICTURE_QUEUE_SIZE];
	for(int i = 0 ; i < SUBPICTURE_QUEUE_SIZE;i++){
		free_subpicture(&sb->subpq[i]);
	}
	av_free(sb);
	*psb = NULL;
}

int subtitle_queue_processer(VideoState *is,SubtitleBuffer* sb)
{
    SubPicture *sp;
    AVPacket pkt1, *pkt = &pkt1;
    int got_subtitle;
    int serial;
    double pts;
    int i, j;
    int r, g, b, y, u, v, a;

    for (;;) {
        while (is->paused && !is->subtitleq.abort_request) {
            SDL_Delay(10);
        }
        if (packet_queue_get(&is->subtitleq, pkt, 1, &serial) < 0)
            break;

        if (pkt->data == flush_pkt.data) {
            avcodec_flush_buffers(is->subtitle_st->codec);
            continue;
        }
        SDL_LockMutex(sb->subpq_mutex);
        while (sb->subpq_size >= SUBPICTURE_QUEUE_SIZE &&
               !is->subtitleq.abort_request) {
            SDL_CondWait(sb->subpq_cond, sb->subpq_mutex);
        }
        SDL_UnlockMutex(sb->subpq_mutex);

        if (is->subtitleq.abort_request)
            return 0;

        sp = &sb->subpq[sb->subpq_windex];

       /* NOTE: ipts is the PTS of the _first_ picture beginning in
           this packet, if any */
        pts = 0;
        if (pkt->pts != AV_NOPTS_VALUE)
            pts = av_q2d(is->subtitle_st->time_base) * pkt->pts;

        avcodec_decode_subtitle2(is->subtitle_st->codec, &sp->sub,
                                 &got_subtitle, pkt);
        if (got_subtitle && sp->sub.format == 0) {
            if (sp->sub.pts != AV_NOPTS_VALUE)
                pts = sp->sub.pts / (double)AV_TIME_BASE;
            sp->pts = pts;
            sp->serial = serial;

            for (i = 0; i < sp->sub.num_rects; i++)
            {
                for (j = 0; j < sp->sub.rects[i]->nb_colors; j++)
                {
                    RGBA_IN(r, g, b, a, (uint32_t*)sp->sub.rects[i]->pict.data[1] + j);
                    y = RGB_TO_Y_CCIR(r, g, b);
                    u = RGB_TO_U_CCIR(r, g, b, 0);
                    v = RGB_TO_V_CCIR(r, g, b, 0);
                    YUVA_OUT((uint32_t*)sp->sub.rects[i]->pict.data[1] + j, y, u, v, a);
                }
            }

            /* now we can update the picture count */
            if (++sb->subpq_windex == SUBPICTURE_QUEUE_SIZE)
                sb->subpq_windex = 0;
            SDL_LockMutex(sb->subpq_mutex);
            sb->subpq_size++;
            SDL_UnlockMutex(sb->subpq_mutex);
        }
        av_free_packet(pkt);
    }
    return 0;
}