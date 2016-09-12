#ifndef VIDEOSTATE_H
#define VIDEOSTATE_H

#include <string>
#include "ffmpeg.h"
#include "packet_queue.h"
#include "SynchronizeClockDef.h"

#include "Streamcomponent.h"

//队列缓冲区字节最大空间
#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
//最小读取包数
#define MIN_FRAMES 5

//视频窗体显示模式
enum ShowMode {
	SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
};

typedef struct VideoState{
	enum ShowMode show_mode;

	int64_t audio_callback_time;//音频数据回调时间
	int realtime;				//是否为实时流

	int abort_request;			//终止
	int paused;					//暂停
	int last_paused;			//最后一次的暂停数值
	int read_pause_return;		//暂停操作结果	

	int seek_req;				//请求切换流位置
	int64_t seek_pos;			//读取位置，PTS
	int64_t seek_rel;			//真实读取的位置，相对于真实事件
	int seek_flags;				//偏移文件使用的模式，AVSEEK_FLAG_BYTE
	int eof;					//文件读取至结尾
	AVFormatContext *ic;		//文件上下文

	//队列
	//数据缓冲区
	PacketQueue videoq;			//视频数据缓冲区
	PacketQueue audioq;			//音频数据缓冲区
	PacketQueue subtitleq;		//字幕数据缓冲区

	AVStream *audio_st;//编解码器
	AVStream *video_st;//编解码器
	AVStream *subtitle_st;//编解码器

	int audio_stream;			//是否有音频流，-1为没有，>= 0为有
	int subtitle_stream;		//是否有字幕流，-1为没有，>= 0为有
	int video_stream;			//是否有视频流，-1为没有，>= 0为有
}VideoState;

typedef struct StreamConfigure
{
	int seek_by_bytes;		//设置读取字节方式
	int64_t start_time;		//启动时间
	int st_index[AVMEDIA_TYPE_NB];
	int video_disable;		//禁止视频
	int audio_disable;		//禁止音频
	int subtitle_disable;	//禁止字幕
	Audio_Param audiopm;	//音频参数

	int64_t duration;	//

	int loop;			//循环次数
	int autoexit;		//读完数据自动终止
	int infinite_buffer;	//无限内存模式
}StreamConfigure;

//读取流配置
void InitStreamConfigure(StreamConfigure *is);
void UnitStreamConfigure(StreamConfigure **pis);

//初始化视频参数
VideoState *CreateVideoState();
//释放结构体
void FreeVideoState(VideoState **is);
//打开文件并初始化VideoState
int OpenStream(VideoState *is,std::string strName,StreamConfigure *cf);
//开始加载文件流
int LoadStreams(VideoState *is,SynchronizeClockState *scs,StreamConfigure *cf);
//终止流
int AbortStream(VideoState *is);
//设置流读取位置，nPos:单位秒
void seek_stream_pos(VideoState *is,int64_t nPos);
//前进POS
void seek_pos(VideoState *is,SynchronizeClockState *scs,double incr);

#endif