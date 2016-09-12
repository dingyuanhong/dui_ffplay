#ifndef IMAGERENDER_H
#define IMAGERENDER_H

/*
功能:绘制VideoPicture->bmp图像至SDL屏幕涵涵素
*/


#include "ffmpeg.h"
#include "SDLRenderParam.h"
#include "VideoDef.h"

//初始化绘制模块
int init_image_render(ImageRenderInfor *iri,int w,int h,int flags,int force_set_video_mode);
//申请图片内存
int alloc_videopicture(ImageRenderInfor *iri,VideoPicture *vp);
//绘制视频，字幕
void video_image_draw(ImageRenderInfor *iri,VideoPicture *vp,SubPicture *sp,int force_refresh);
//通过转换模块将AVFrame图像数据复制到VideoPicture中
int video_copy_picture(struct SwsContext **pimg_convert_ctx,AVFrame *src_frame,VideoPicture *vp);

#endif