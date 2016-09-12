#include "SDLVideoRender.h"

#include "ImageRender.h"
#include "SDLDraw.h"
#include "AudioSampleDraw.h"

//return -1 then exit
int video_open(UserScreenRender *vr,int force_set_video_mode, VideoPicture *vp)
{
    int flags = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL;
    int w,h;
    SDL_Rect rect;

    if (vr->is_full_screen) flags |= SDL_FULLSCREEN;
    else                flags |= SDL_RESIZABLE;

    if (vp && vp->width) {
        calculate_display_rect(&rect, 0, 0, INT_MAX, vp->height, vp);
        vr->default_width  = rect.w;
        vr->default_height = rect.h;
    }

    if (vr->is_full_screen && vr->fs_screen_width) {
        w = vr->fs_screen_width;
        h = vr->fs_screen_height;
    } else if (!vr->is_full_screen && vr->screen_width) {
        w = vr->screen_width;
        h = vr->screen_height;
    } else {
        w = vr->default_width;
        h = vr->default_height;
    }
    w = FFMIN(16383, w);

	return init_image_render(&vr->iri,w,h,flags,force_set_video_mode);
}

int alloc_picture(VideoPictureBuffer *is,ScreenRender *sr)
{
    VideoPicture *vp = &is->pictq[is->pictq_windex];
	if(vp == NULL) return -1;
    if (vp && vp->bmp){
        SDL_FreeYUVOverlay(vp->bmp);
		vp->bmp = NULL;
	}
	UserScreenRender *vr = (UserScreenRender*)sr->Render;
	video_open(vr, 0, vp);

	alloc_videopicture(&vr->iri,vp);

    SDL_LockMutex(is->pictq_mutex);
    vp->allocated = 1;
    SDL_CondSignal(is->pictq_cond);
    SDL_UnlockMutex(is->pictq_mutex);
	return 1;
}

int fill_picture(ScreenRender *sr,VideoPictureBuffer *vpr, AVFrame *src_frame, double pts, int64_t pos, int serial)
{
	VideoPicture *vp = &vpr->pictq[vpr->pictq_windex];

    vp->sar = src_frame->sample_aspect_ratio;

    /* alloc or resize hardware picture buffer */
    if (!vp->bmp || vp->reallocate || !vp->allocated ||
        vp->width  != src_frame->width ||
        vp->height != src_frame->height) {
        SDL_Event event;

        vp->allocated  = 0;
        vp->reallocate = 0;
        vp->width = src_frame->width;
        vp->height = src_frame->height;

		alloc_picture(vpr,sr);
    }
    /* if the frame is not skipped, then display it */
    if (vp->bmp) {
		UserScreenRender *vr = (UserScreenRender*)sr->Render;
#if CONFIG_AVFILTER
		video_copy_picture(NULL,src_frame,vp);
#else
		video_copy_picture(&vr->img_convert_ctx,src_frame,vp);
#endif
        vp->pts = pts;
        vp->pos = pos;
        vp->serial = serial;

        /* now we can update the picture count */
        if (++vpr->pictq_windex == VIDEO_PICTURE_QUEUE_SIZE)
            vpr->pictq_windex = 0;
        SDL_LockMutex(vpr->pictq_mutex);
        vpr->pictq_size++;
        SDL_UnlockMutex(vpr->pictq_mutex);
    }
	return 0;
}
//ÏÔÊ¾ÒôÆµÆµÆ×
static void video_audio_display(VideoState *is,ImageRenderInfor *iri,AudioSampleParams *asr,AudioSampleArray *s,int audio_write_buf_size)
{
    int channels = FFMAX(asr->audio_tgt.channels,1);

    if (!is->paused) {
		Get_I_Start(iri,&asr->ass,asr->audio_tgt,channels,is->audio_callback_time,s,audio_write_buf_size,is->show_mode == SHOW_MODE_WAVES);
    }
    if (is->show_mode == SHOW_MODE_WAVES) {
		AudioSampleWAVE(iri,&asr->ass,channels,s,audio_write_buf_size);
    } else {
		AudioSampleFFTS(iri,&asr->ass,&asr->fftsri,channels,s,audio_write_buf_size);
		//µ÷ÕûÏÔÊ¾·½Î»
        if (!is->paused)
			asr->fftsri.xpos++;
		if (asr->fftsri.xpos >= iri->width)
            asr->fftsri.xpos= iri->xleft;
    }
}

void video_image_display(ScreenRender *sr,VideoPictureBuffer *vpb,SubtitleBuffer *sb,bool bShowSubtitle)
{
    VideoPicture *vp = NULL;
    SubPicture *sp = NULL;

    vp = &vpb->pictq[vpb->pictq_rindex];
    if (vp->bmp) {
        if (bShowSubtitle) {
            if (sb->subpq_size > 0) {
                sp = &sb->subpq[sb->subpq_rindex];
            }
		}
		UserScreenRender *vr = (UserScreenRender*)sr->Render;
		video_image_draw(&vr->iri,vp,sp,sr->force_refresh);
    }
}

/* display the current picture, if any */
void video_display(VideoState *is,
	ScreenRender *sr,
	SubtitleBuffer *sb,		//×ÖÄ»»º³åÇø
	VideoPictureBuffer *vpb, //ÊÓÆµÍ¼Ïñ»º³åÇø
	SynchronizeClockState *scs)
{
	UserScreenRender *vr = (UserScreenRender*)sr->Render;
	if (!vr->iri.screen || vr->iri.flush_screen != 0)
		video_open(vr, 0, NULL);
	if (is->audio_st && is->show_mode != SHOW_MODE_VIDEO)
	{
		video_audio_display(is,&vr->iri,&vr->asr,&scs->sample_array,scs->audio_write_buf_size);
	}
	else
	if (is->video_st)
		video_image_display(sr,vpb,sb,(is->subtitle_st!=NULL));
}
void audio_sample_display(VideoState *is,
	ScreenRender *sr,
	SynchronizeClockState *scs)
{
	UserScreenRender *vr = (UserScreenRender*)sr->Render;

	if (!vr->iri.screen || vr->iri.flush_screen != 0)
		video_open(vr, 0, NULL);
	if (is->audio_st && is->show_mode != SHOW_MODE_VIDEO)
	{
		video_audio_display(is,&vr->iri,&vr->asr,&scs->sample_array,scs->audio_write_buf_size);
	}
}


void toggle_audio_display(VideoState *is,ScreenRender *sr)
{
	UserScreenRender *vrs = (UserScreenRender*)sr->Render;
	int bgcolor = SDL_MapRGB(vrs->iri.screen->format, 0x00, 0x00, 0x00);
    int next = is->show_mode;
    do {
        next = (next + 1) % SHOW_MODE_NB;
    } while (next != is->show_mode && (next == SHOW_MODE_VIDEO && !is->video_st || next != SHOW_MODE_VIDEO && !is->audio_st));
    if (is->show_mode != next) {
        fill_rectangle(vrs->iri.screen,
                    vrs->iri.xleft, vrs->iri.ytop, vrs->iri.width, vrs->iri.height,
                    bgcolor, 1);
        sr->force_refresh = 1;
        is->show_mode = (ShowMode)next;
    }
}
void toggle_full_screen(ScreenRender *is)
{
#if defined(__APPLE__) && SDL_VERSION_ATLEAST(1, 2, 14)
    /* OS X needs to reallocate the SDL overlays */
    int i;
    for (i = 0; i < VIDEO_PICTURE_QUEUE_SIZE; i++)
        is->pictq[i].reallocate = 1;
#endif
	UserScreenRender *vrs = (UserScreenRender*)is->Render;
	vrs->is_full_screen = !vrs->is_full_screen;
    video_open(vrs, 1, NULL);
}


UserScreenRender *CreateUserScreenRender(int width,int height)
{
	UserScreenRender *is = (UserScreenRender*)av_malloc(sizeof(UserScreenRender));
	memset(is,0,sizeof(UserScreenRender));
	is->screen_height = height;
	is->screen_width = width;
	return is;
}
void InitUserScreenRender(UserScreenRender * usr,int width,int height)
{
	if(usr == NULL) return;
	if(!(usr->screen_height == height && usr->screen_width == width))
	{
		usr->screen_width == width;
		usr->screen_height == height;
		usr->iri.flush_screen = 1;
	}
}
void FreeUserScreenRender(UserScreenRender** usr)
{
	if(usr == NULL) return;
	if(*usr != NULL){
		av_free(*usr);
	}
	*usr = NULL;
}