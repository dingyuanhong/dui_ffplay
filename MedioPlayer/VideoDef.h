#ifndef VIDEODEF_H
#define VIDEODEF_H

#include "ffmpeg.h"
#include "MSDL.h"
#include "packet_queue.h"

#define VIDEO_PICTURE_QUEUE_SIZE 3

#define SUBPICTURE_QUEUE_SIZE 4

#define REFRESH_RATE 0.01

typedef struct SubPicture {
    double pts; /* presentation time stamp for this picture */
    AVSubtitle sub;
    int serial;
} SubPicture;

//×ÖÄ»äÖÈ¾
typedef struct SubtitleBuffer
{
	SubPicture subpq[SUBPICTURE_QUEUE_SIZE];//ÎÄ×Ö»º³åÇø£¬Ä¬ÈÏ4Ö¡
    int subpq_size, subpq_rindex, subpq_windex;

	SDL_mutex *subpq_mutex;
    SDL_cond *subpq_cond;
}SubtitleBuffer;

typedef struct VideoPicture {
    double pts;             // presentation timestamp for this picture
    int64_t pos;            // byte position in file
    SDL_Overlay *bmp;
    int width, height; /* source height & width */
    int allocated;
    int reallocate;
    int serial;

    AVRational sar;	//Ëõ·Å±ÈÀý
} VideoPicture;

//ÊÓÆµÍ¼Æ¬äÖÈ¾
typedef struct VideoPictureBuffer
{
	VideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE];//Í¼Æ¬»º³åÇø£¬Ä¬ÈÏ3Ö¡
	int pictq_size,pictq_rindex,pictq_windex;

	SDL_mutex *pictq_mutex;
    SDL_cond *pictq_cond;
}VideoPictureBuffer;

#endif