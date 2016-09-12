#ifndef SUBTITLEPROCESSER_H
#define SUBTITLEPROCESSER_H

#include "VideoState.h"
#include "VideoDef.h"

void free_subpicture(SubPicture *sp);

SubtitleBuffer *CreateSubtile();
void FreeSubtitle(SubtitleBuffer** psb);

int subtitle_queue_processer(VideoState *is,SubtitleBuffer* sb);

#endif