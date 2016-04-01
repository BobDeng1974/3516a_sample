#pragma once
#include "videosourcebase.h"

extern "C" {
#	include <libavcodec/avcodec.h>
#	include <libavformat/avformat.h>
#	include <libswscale/swscale.h>
}

class VideoSourceFFmpeg : public VideoSourceBase
{
	AVFormatContext *fc_;
	SwsContext *sw_;
	AVFrame *frame_;
	int last_w_, last_h_;

public:
	VideoSourceFFmpeg(const imgsrc_format *fmt, const char *url);
	~VideoSourceFFmpeg(void);

private:
	virtual int open_src();
	virtual void close_src();
	virtual int get_src(zifImage *img);
	virtual void pause_src();
	virtual void resume_src();

	bool chk_sws(AVCodecContext *cc);	// ��� sws �Ƿ����� ..
	bool save(zifImage *img); // ����һ֡ͼ�� ..
};
