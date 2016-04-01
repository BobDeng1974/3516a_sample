#pragma once

#include <zonekey/zqpsource.h>
#include <cc++/thread.h>
#include <deque>
extern "C" {
#	include <libavformat/avformat.h>
#	include <libavcodec/avcodec.h>
#	include <libswscale/swscale.h>
}
#include "image_source.h"

/** ��Ӧͼ��Դ */
class VideoSource : ost::Thread
{
	// �����̴߳� zqpkt �������ݣ������浽 images_ �У�next_img() ������ȡ
	typedef std::deque<zifImage*> IMAGES;
	IMAGES fifo_, cached_;
	ost::Mutex cs_images_;
	void *src_;
	bool quit_;
	AVCodecContext *dec_;
	AVFrame *frame_;
	HANDLE evt_;
	int ow_, oh_;	// Ҫ�������Ĵ�С
	unsigned int cnt_;
	bool opened_;
	std::string url_;
	imgsrc_format fmt_;

	enum Mode
	{
		M_TCP,		// ���� tcp://... zqpkt Դ
		M_LOCAL,	// ���� jp100hd ��Ԥ��
	};

	Mode mode_;

public:
	VideoSource() : opened_(false)
	{
	}

	/** �� tcp://... ͼ��Դ��ʹ�� zqpkt ��ʽ */
	int open(const imgsrc_format *fmt, const char *url);
	void close();

	/** ��ȡ��һ֡ͼ��Ӧ���죬����������ʧ */
	zifImage *next_img();
	void free_img(zifImage *img);

	/** ��ջ��� */
	void flush();

	/** ���� M_LOCAL��
	 */
	int write_yuv420p(int width, int height, unsigned char *data[], int stride[]);

private:
	void run();
	void save_frame(SwsContext *sws, AVFrame *frame, double stamp);
	zifImage *get_cached(int width, int height, int fmt);
};
