#pragma once

#include <opencv2/opencv.hpp>
#include <cc++/thread.h>
#include "../libkvconfig/KVConfig.h"
#include "ObjectDetect.h"

enum Dir
{
	RIGHT,
	DOWN,
	LEFT,
	UP,
	NONE,
};

static const char *DirDesc[] =
{
	"RIGHT",
	"DOWN",
	"LEFT",
	"UP",
	"NONE",
};

/** ά��վ��Ŀ�� ...
 */
class Detect
{
	/// ��Ӧһ����վ����Ŀ�� ...
	struct Standup
	{
		cv::Rect pos;		// 
		double stamp;		// ȷ��ʱ��� ...
		bool waiting;		// �ȴ��Ա������������ ...

		bool enable_od;		// �Ƿ����ö���ʶ�� ...
		cv::Rect face;		// ������ö���ʶ����Ϊ ...
	};
	typedef std::vector<Standup> STANDUPS;
	STANDUPS standups_;

	/// ������ ...
	double max_duration_;	// ��վ����Ŀ��������ʱ�䣬��ʱ����ǿ����Ϊ���� ...
	double waiting_;		// Ŀ�����º󣬵ȴ�һ��ʱ�䣬����Ӧ�˴��Ļ ...
	bool masked_;
	cv::Mat mask_;
	ObjectDetect *od_;

public:
	explicit Detect(KVConfig *cfg);
	virtual ~Detect();

public:
	typedef std::vector<cv::Rect> RCS;

	/// ���� rgb ͼ��������ء�վ�����˵�λ�� ...
	virtual void detect(const cv::Mat &origin, RCS &standups);

	/// ����������ݣ����´������ļ��м��� ...
	virtual void reset();

protected:
	KVConfig *cfg_;
	size_t cnt_;
	double stamp_;
	cv::Mat origin_;
	cv::Mat gray_prev_, gray_curr_;		// ��֡�Ҷ�ͼ������ʹ��֡� ...
	bool debug_;
	void(*log_init)(const char *fname);
	void(*log)(const char *fmt, ...);

	/// �����෵�ز�ͬ����Ļ ...
	virtual void detect(RCS &rcs, std::vector<Dir> &dirs) = 0;

	// �ҵ��ཻ������� standup
	STANDUPS::iterator find_crossed_target(const cv::Rect &motion)
	{
		int max_area = 0;
		STANDUPS::iterator found = standups_.end();

		for (STANDUPS::iterator it = standups_.begin(); it != standups_.end(); ++it) {
			int crossed = (it->pos & motion).area();
			if (crossed > max_area) {
				max_area = crossed;
				found = it;
			}
		}

		return found;
	}

	double now() const
	{
		struct timeval tv;
		ost::gettimeofday(&tv, 0);
		return tv.tv_sec + tv.tv_usec * 0.000001;
	}

	bool build_mask(cv::Mat &mask)
	{
		bool masked = false;

		const char *pts = cfg_->get_value("calibration_data", 0);
		std::vector<cv::Point> points;

		if (pts) {
			char *data = strdup(pts);
			char *p = strtok(data, ";");
			while (p) {
				// ÿ��Point ʹ"x,y" ��ʽ
				int x, y;
				if (sscanf(p, "%d,%d", &x, &y) == 2) {
					cv::Point pt(x, y);
					points.push_back(pt);
				}

				p = strtok(0, ";");
			}
			free(data);
		}

		if (points.size() > 3) {
			int n = (int)points.size();
			cv::vector<cv::Point> pts;
			for (int i = 0; i < n; i++) {
				pts.push_back(points[i]);
			}

			mask = cv::Mat::zeros(cv::Size(atoi(cfg_->get_value("video_width", "960")), 
				atoi(cfg_->get_value("video_height", "540"))), CV_8UC3);

			std::vector<std::vector<cv::Point> > ptss;
			ptss.push_back(pts);
			cv::fillPoly(mask, ptss, cv::Scalar(255, 255, 255));

			masked = true;
		}

		return masked;
	}
};
