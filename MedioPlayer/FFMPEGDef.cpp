#include "FFMPEGDef.h"
#include "ffmpeg.h"
#include "packet_queue.h"

int FFMPEG_Init()
{
	/* register all codecs, demux and protocols */
    avcodec_register_all();
#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif
#if CONFIG_AVFILTER
    avfilter_register_all();
#endif
    av_register_all();
    avformat_network_init();

	//³õÊ¼»¯°ü
	init_flush_pkt();
	return 1;
}