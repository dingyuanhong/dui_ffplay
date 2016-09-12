#include "ImageRender.h"
#include "SDLDraw.h"
int init_image_render(ImageRenderInfor *iri,int w,int h,int flags,int force_set_video_mode)
{
	if(iri == NULL) return -1;

	w = FFMIN(16383, w);

	if (iri->screen && iri->width == iri->screen->w && iri->screen->w == w
       && iri->height== iri->screen->h && iri->screen->h == h && !force_set_video_mode)
        return 0;
    iri->screen = SDL_SetVideoMode(w, h, 0, flags);
    if (!iri->screen) {
        av_log(NULL, AV_LOG_FATAL, "SDL: could not set video mode - exiting\n");
		return -1;
    }

    iri->width  = iri->screen->w;
    iri->height = iri->screen->h;
	iri->flush_screen = 0;
	return 1;
}

int alloc_videopicture(ImageRenderInfor *iri,VideoPicture *vp)
{
    int64_t bufferdiff;

	if(vp == NULL) return -1;

    if (vp && vp->bmp)
        SDL_FreeYUVOverlay(vp->bmp);

	vp->bmp = SDL_CreateYUVOverlay(vp->width, vp->height,
                                   SDL_YV12_OVERLAY,
                                   iri->screen);
    bufferdiff = vp->bmp ? FFMAX(vp->bmp->pixels[0], vp->bmp->pixels[1]) - FFMIN(vp->bmp->pixels[0], vp->bmp->pixels[1]) : 0;
    if (!vp->bmp || vp->bmp->pitches[0] < vp->width || bufferdiff < (int64_t)vp->height * vp->bmp->pitches[0]) {
        /* SDL allocates a buffer smaller than requested if the video
         * overlay hardware is unable to support the requested size. */
        av_log(NULL, AV_LOG_FATAL,
               "Error: the video system does not support an image\n"
                        "size of %dx%d pixels. Try using -lowres or -vf \"scale=w:h\"\n"
                        "to reduce the image size.\n", vp->width, vp->height );
        //do_exit(is);
		return -1;
    }
	return 1;
}

void video_image_draw(ImageRenderInfor *iri,VideoPicture *vp,SubPicture *sp,int force_refresh)
{
    AVPicture pict;
	SDL_Rect rect = {0};
    int i;

    if (vp != NULL && vp->bmp) {
		if(sp != NULL){
			if (vp->pts >= sp->pts + ((float) sp->sub.start_display_time / 1000)) {
				SDL_LockYUVOverlay (vp->bmp);

				pict.data[0] = vp->bmp->pixels[0];
				pict.data[1] = vp->bmp->pixels[2];
				pict.data[2] = vp->bmp->pixels[1];

				pict.linesize[0] = vp->bmp->pitches[0];
				pict.linesize[1] = vp->bmp->pitches[2];
				pict.linesize[2] = vp->bmp->pitches[1];

				for (i = 0; i < sp->sub.num_rects; i++)
					blend_subrect(&pict, sp->sub.rects[i],
									vp->bmp->w, vp->bmp->h);

				SDL_UnlockYUVOverlay (vp->bmp);
			}
		}
        calculate_display_rect(&rect, iri->xleft, iri->ytop, iri->width, iri->height, vp);

        SDL_DisplayYUVOverlay(vp->bmp, &rect);

        if (rect.x != iri->last_display_rect.x || rect.y != iri->last_display_rect.y || rect.w != iri->last_display_rect.w || rect.h != iri->last_display_rect.h || force_refresh) 
		{
            int bgcolor = SDL_MapRGB(iri->screen->format, 0x00, 0x00, 0x00);
            fill_border(iri->screen,iri->xleft, iri->ytop, iri->width, iri->height, rect.x, rect.y, rect.w, rect.h, bgcolor, 1);
            iri->last_display_rect = rect;
        }
    }
}
int video_copy_picture(struct SwsContext **pimg_convert_ctx,AVFrame *src_frame,VideoPicture *vp)
{
#if !CONFIG_AVFILTER
	if(pimg_convert_ctx == NULL) return -1;
#endif
	AVPicture pict = { { 0 } };

    /* get a pointer on the bitmap */
    SDL_LockYUVOverlay (vp->bmp);

    pict.data[0] = vp->bmp->pixels[0];
    pict.data[1] = vp->bmp->pixels[2];
    pict.data[2] = vp->bmp->pixels[1];

    pict.linesize[0] = vp->bmp->pitches[0];
    pict.linesize[1] = vp->bmp->pitches[2];
    pict.linesize[2] = vp->bmp->pitches[1];

#if CONFIG_AVFILTER
    // FIXME use direct rendering
    av_picture_copy(&pict, (AVPicture *)src_frame,
                    src_frame->format, vp->width, vp->height);
#else
	int64_t sws_flags = SWS_BICUBIC;
    //av_opt_get_int(sws_opts, "sws_flags", 0, &sws_flags);
    *pimg_convert_ctx = sws_getCachedContext(*pimg_convert_ctx,
        vp->width, vp->height, (AVPixelFormat)src_frame->format, vp->width, vp->height,
        AV_PIX_FMT_YUV420P, sws_flags, NULL, NULL, NULL);
    if (*pimg_convert_ctx == NULL) {
        av_log(NULL, AV_LOG_FATAL, "Cannot initialize the conversion context\n");
        return -1;
    }
    sws_scale(*pimg_convert_ctx, src_frame->data, src_frame->linesize,
                0, vp->height, pict.data, pict.linesize);
#endif
    /* workaround SDL PITCH_WORKAROUND */
    duplicate_right_border_pixels(vp->bmp);
    /* update the bitmap content */
    SDL_UnlockYUVOverlay(vp->bmp);

	return 1;
}