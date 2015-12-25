#pragma once

// �����д���� ...

#include <cc++/thread.h>
#include <string>
#include <deque>
#include <assert.h>
#include "image_source.h"

extern "C" {
#	include <libavcodec/avcodec.h>
}

class VideoSourceBase : ost::Thread
{
	bool quit_;
	ost::Semaphore sem_pics_;
	ost::Event quit_evt_;
	typedef std::deque<zifImage*> IMAGES;
	IMAGES fifo_, cached_;
	ost::Mutex cs_fifo_, cs_cached_;
	bool paused_;

public:
	VideoSourceBase(const imgsrc_format *fmt, const char *url)
	{
		fmt_ = *fmt;
		url_ = url;

		quit_ = true;
	}

	~VideoSourceBase()
	{
	}

	/** ����ͼ���ȡ */
	int start()
	{
		if (quit_) {
			quit_ = false;
			paused_ = false;
			ost::Thread::start();
			return 0;
		}

		return 1;
	}

	/** ���� .. */
	int stop()
	{
		if (!quit_) {
			quit_ = true;

			join();

			free_all();
			return 0;
		}

		return 1;
	}

	/** ��ȡ��һ֡ͼ���п����������� 100ms����ʱ�򷵻� 0 */
	zifImage *next_img()
	{
		return next_image();
	}

	/** ʹ����ɣ��ͷ� */
	void free_img(zifImage *img)
	{
		free_image(img);
	}

	/** pause */
	void pause()
	{
		paused_ = true;
		pause_src();
	}

	/** resume */
	void resume()
	{
		resume_src();
		paused_ = false;
	}

protected:
	std::string url_;
	imgsrc_format fmt_;

	// ��Դ������ < 0 ʧ��
	virtual int open_src() = 0;

	// �رս��� ..
	virtual void close_src() = 0;

	virtual void pause_src() {}
	virtual void resume_src() {}

	// ��ȡ��һ֡ͼ��img Ϊ�洢�ռ䣬���� < 0 ˵��ʧ�� ...
	virtual int get_src(zifImage *img) = 0;

	zifImage *next_image()
	{
		if (paused_) {
			return 0;
		}

		// ��fifo�еõ� ..
		if (sem_pics_.wait(300)) {
			assert(!fifo_.empty());

			ost::MutexLock al(cs_fifo_);
			zifImage *img = fifo_.front();
			fifo_.pop_front();
			return img;
		}
		else {
			return 0;
		}
	}

	void free_image(zifImage *img)
	{
		// �ŵ������б��� ..
		ost::MutexLock al(cs_cached_);
		while (cached_.size() > 10) {
			zifImage *image = cached_.front();
			cached_.pop_front();
			del_zifImage(image);
		}

		cached_.push_back(img);
	}

private:
	void del_zifImage(zifImage *img)
	{
		avpicture_free((AVPicture*)img->internal_ptr);
		delete (AVPicture*)img->internal_ptr;
		delete img;
	}

	zifImage *new_zifImage()
	{
		zifImage *img = new zifImage;
		img->fmt_type = fmt_.fmt;
		img->width = fmt_.width;
		img->height = fmt_.height;
		img->internal_ptr = new AVPicture;
		avpicture_alloc((AVPicture*)img->internal_ptr, fmt_.fmt, fmt_.width, fmt_.height);

		for (int i = 0; i < 4; i++) {
			img->data[i] = ((AVPicture*)img->internal_ptr)->data[i];
			img->stride[i] = ((AVPicture*)img->internal_ptr)->linesize[i];
		}

		return img;
	}

	void free_all()
	{
		{
			ost::MutexLock al(cs_cached_);
			while (!cached_.empty()) {
				zifImage *img = cached_.front();
				del_zifImage(img);
				cached_.pop_front();
			}
		}

		{
			ost::MutexLock al(cs_fifo_);
			while (!fifo_.empty()) {
				zifImage *img = fifo_.front();
				del_zifImage(img);
				fifo_.pop_front();
			}
		}
	}

	size_t fifo_size()
	{
		ost::MutexLock al(cs_fifo_);
		return fifo_.size();
	}

	void save_image(zifImage *img)
	{
		ost::MutexLock al(cs_fifo_);

		fifo_.push_back(img);
		sem_pics_.post();
	}

	zifImage *get_empty()
	{
		ost::MutexLock al(cs_cached_);
		if (cached_.empty()) {
			return new_zifImage();
		}
		else {
			zifImage *img = cached_.front();
			cached_.pop_front();
			return img;
		}
	}

	virtual void run()
	{
		size_t cnt = 0;	// FIXME: ������������֡�����ڸ�����˵��ÿ�� 30/25 ̫֡���ˣ����Կ����������� ..

		while (!quit_) {
			if (open_src() < 0) {
				quit_evt_.wait(5000);	// 5������ ..
				continue;
			}

			while (!quit_) {
				if (paused_) {
					sleep(50);
					continue;
				}
				
				if (fifo_size() > 10) {
					sleep(50);
					continue;
				}

				zifImage *img = get_empty();
				assert(img);

				int rc = get_src(img);
				if (rc == 0) {
					// ����������TODO: ...
				}
				else if (rc < 0) {
					// ʧ�� ...
					break;
				}
				else {
					if (cnt++ % 2 == 0) {
						save_image(img);
					}
					else {
						free_image(img);
					}
				}
			}

			close_src();
		}
	}
};
