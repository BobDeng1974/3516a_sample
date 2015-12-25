#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/ocl/ocl.hpp>
#include "../libkvconfig/KVConfig.h"
#include <time.h>
#include <assert.h>
#include "objdet.h"
#include <cc++/thread.h>
#include "polyfit.h"
#include "SkinMask.h"
#include <list>
#include "History.h"

#define CENTER_X(RECT) ((RECT).x+(RECT).width/2)
#define CENTER_Y(RECT) ((RECT).y+(RECT).height/2)

#define _USE_MATH_DEFINES
#include <math.h>

typedef std::vector<cv::Rect> RECTS;

static double _distance(const cv::Point2f &p1, const cv::Point2f &p2)
{
	return sqrt(pow(p1.x-p2.x, 2) + pow(p1.y-p2.y, 2));
}

// ����
enum Dir
{
	RIGHT = 0,
	DOWN = 1,
	LEFT = 2,
	UP = 3,
};

static const char *DirDesc[] = {
	"right", "down", "left", "up", 
};

// �������������Ƿ��н��������߰���
inline bool is_cross(const cv::Rect &rc1, const cv::Rect &rc2)
{
	return !(rc1.x+rc1.width <= rc2.x || rc2.x+rc2.width <= rc1.x || rc1.y + rc1.height <= rc2.y || rc2.y+rc2.height <= rc1.y);
}

class Detect
{
	class Target
	{
	public:
		Target()
		{
			updated_cnt = 0;
			detecte_state_ = 0;
		}

		cv::Rect pos;	// λ��
		double stamp;	// ���� target ��ʱ�� 

		/* XXX: ����˵��ÿ��Ŀ��Ļ��Ҫ��֡��ȷ��������˵��һ��Ŀ��λ�ã����������
				�����£���˵�����λ������Ч��Ŀ�� ...
		 */
		int updated_cnt;	

		/** Ŀ��ʶ��:
				0: ��û�н���
				1: �Ѿ�ȷ��
				<0: ʶ��ʶ����� ...
		 */
		int detecte_state_;

		Dir dir;
	};

	// ��¼���һ�λ�ľ��Σ��������� 
	class MotionRect : public Target
	{
		Dir dir_;

	public:
		MotionRect(const cv::Rect &rc, double stamp, Dir dir)
		{
			pos = rc, Target::stamp = stamp, dir_ = dir;
		}

		const cv::Rect &rc() const { return pos; }
		double stamp() const { return Target::stamp; }
		Dir dir() const { return dir_; }
	};

	std::list<MotionRect> down_motion_rects_;	// ��������֮ǰ�Ļ���򣬲��������ϵ��� ...

	/** ��Ӧ��Ч� */
	struct Motions
	{
		cv::Rect rc;	// �λ��
		Dir dir;		// �����
		double stamp;	// ʱ���
	};
	std::vector<Motions> motion_hists_;

	/** ɾ����ʱ�Ļ��ʷ */
	struct TooooOldofMotionHist
	{
		double now_, delay_;

		TooooOldofMotionHist(double now, double delay)
		{
			now_ = now, delay_ = delay;
		}

		bool operator()(const Motions &m) const
		{
			return now_ - m.stamp > delay_;
		}
	};

	bool flipped_;
	ost::Mutex lock_;

	typedef std::vector<Target> TARGETS;
	TARGETS targets_;	// Ŀ�� ..

	struct TooOld
	{
	private:
		double now_;
		double duration_;

	public:
		TooOld(double now, double duration = 30.0) 
		{ 
			now_ = now; 
			duration_ = (double)duration;
		}

		bool operator ()(const Target &t) const
		{
			return now_ - t.stamp > duration_;	// ������ʮ�� ..
		}
	};

	class ToooLarge
	{
	private:
		Detect *parent_;

	public:
		ToooLarge(Detect *det): parent_(det) {}
		bool operator()(const Target &t) const;
	};
	friend class ToooLarge;

	class ToooSmall
	{
	private:
		Detect *parent_;
		double now_;

	public:
		ToooSmall(Detect *det, double now): parent_(det), now_(now) {}
		bool operator()(const Target &t) const;
	};
	friend class ToooSmall;

	cv::Mat ker_;	// �ԻҶ�ͼ�������ǿ ...

	objdet *od_;	// �ϰ�����.
	int od_max_times_;	// ����ÿ�� target ��������, Ĭ�� 5 ��

	SkinMask *skin_;
	double skin_head_ratio_;	// ��ɫ������ͷ����ڵ����������ȱʡ >=20%

	bool first_;	// �Ƿ��һ֡ ...
	cv::Mat gray_prev_, gray_curr_;	// ������֡�Ҷ�

	size_t st_cnt_;	// ͳ��
	double st_begin_, st_seg_;
	double fps_;

	double factor_equation_x_[3], factor_equation_y_[3];	// ���η���ϵ�����ֱ�����΢��
	double factor_equation_linear_y_[2];	// y�����Է��� ...
	double factor_equation_area_y_[2];	// ֱ�߷��̣� 

	bool wait_key_;	// �Ƿ���� waitkey

	double max_duration_;	// ������ʱ�䣬��ֹ�����ж�
	int min_updated_;	// ��С���´���
	double min_updated_delay_; // �����С���´�����ʱ������������֡ʱ���� ..
	double matched_area_factor_;	// �����Ͽ�����ıȽ���ֵ������˵��̫С�Ŀ�Ӧ��Ӱ��ܴ��Ŀ��
	double up_area_tolerance_factor_; // target �����ֵϵ����������ֵ�����̷�Χ��Ĭ�� 1.3

	double motion_hist_delay_;	// �����೤ʱ��Ļ��ʷ�����������Ƿ�Ϊ��Ч��վ��Ŀ�� ...

	bool save_history_;
	int target_x_, target_y_;	// �������������˵Ĵ�С��Ĭ�� 130 x 170

public:
	Detect(KVConfig *cfg);
	virtual ~Detect();

	virtual void set_param(int thres_dis, int thres_area, double factor0, double factor05)
	{
		//thres_dis_ = thres_dis;
		thres_area_ = thres_area;
		
		factor_0_ = factor0;
		//factor_05_ = factor05;

		//polyfit(factor_0_, factor_05_, factor_1_, factor_equation_y_);
		polyfit_linear(factor_0_, factor_1_, factor_equation_linear_y_);
	}

	void set_flipped_mode(int enabled);
	std::vector<cv::Rect> current_targets();
	virtual void detect(cv::Mat &origin, std::vector<cv::Rect> &targets, int &flipped_index);

	void (*log)(const char *fmt, ...);

	/** �� 32FC1 ���ҳ��ۺϵ㣬������Ӿ���

		fy = factor_y();
		size *= (fy * fy);
		threshold *= fy
		
		��һ��������ֵ�ĺʹ��� threshold ʱ������Ϊ�������������Ч��Ŀ�� ...
	 */
	std::vector<cv::Rect> find_clusters(const cv::Mat &m, const cv::Size &size = cv::Size(100, 100), const double threshold = 2500,
		int stepx = 4, int stepy = 4) const;

protected:
	// ������ȥʵ�֣� assert(ret.size() == dirs.size())
	virtual std::vector<cv::Rect> detect0(size_t st_cnt, cv::Mat &origin, cv::Mat &gray_prev, cv::Mat &gray_curr, cv::vector<int> &dirs) 
	{
		return std::vector<cv::Rect>();
	}
	virtual int detect0(size_t st_cnt, cv::Mat &origin, cv::Mat &gray_prev, cv::Mat &gray_curr) { return -1; };	// ��תģʽ̽��

	void draw_area_thres_info(cv::Mat &origin);


	// ��������.
	inline int distance(int x1, int y1, int x2, int y2)
	{
		return (int)sqrt((float)(x2-x1) * (x2-x1) + (y2-y1) * (y2-y1));
	}

	KVConfig *cfg_;	//
	int debug_, debug_log_, debug_img_, debug_img2_;
	int video_width_, video_height_;
	cv::Mat origin_;		// Ϊ�˷��㻭ͼ ...
	double factor_0_, factor_1_; //, factor_05_;	// ͼ��y�����ϵ��µ���ֵϵ��.
	double thres_dis_, thres_dis_far_, thres_area_, thres_flipped_dis_;	// ������ֵ��������ֵ��Զ�����������ֵ����ת���þ�����ֵ
	double far_ratio_;
	bool face_detect_far_;	// �����Ƿ����ü��..
	double *factor_y_tables_;	// Ԥ�ȼ��㣬�Ż��������Է��̵��� :)
	cv::Rect area_min_rect_, area_max_rect_;	// ���Ŀ����Ϊ�˴����Ƶ��������ֵ ...
	int area_max_, area_min_;	// �����С��������� area_max_rect_, area_min_rect_ ����..
	int max_rect_factor_;	// Ŀ��λ�������е�ϵ����Ĭ��1.1�����λ�õ�Ŀ�궼��̬�Ĵ� 
	double curr_stamp_;
	double max_target_area_;	// ���Ŀ�����
	History<cv::Mat> frames_history_;	// ����N֡��ʷ�ĻҶ�ͼ���� frames_history_.full() �󣬲���ʹ�� ...

	/// ���� factor_y_ ����Ŀ���С
	cv::Size est_target_size(int y)
	{
		return cv::Size((int)(target_x_ * factor_y(y)), (int)(target_y_ * factor_y(y)));
	}

	void (*log_init)(const char *fname);

	bool is_far(const cv::Rect &rc);	// �����Ƿ�Ϊ����

	double now() const
	{
		timeval tv;
		ost::gettimeofday(&tv, 0);
		return tv.tv_sec + tv.tv_usec/1000000.0;
	}

	//void polyfit(double f0, double f05, double f1, double *factors)
	//{
	//	double xx[3] = { 0, video_height_ / 2, video_height_ };
	//	double yy[3] = { f0, f05, f1 };
	//	::polyfit(3, xx, yy, 2, factors);

	//	fprintf(stderr, "calc factors of equation: %f %f %f\n",
	//		factor_equation_y_[2], factor_equation_y_[1], factor_equation_y_[0]);
	//}

	// f0 ��Ӧ��0�е�ϵ����ȱʡ0.2 �ɣ�f1 ��Ӧ������еģ����� 1 ��
	void polyfit_linear(double f0, double f1, double *factors)
	{
		double xx[2] = { 0, video_height_ };
		double yy[2] = { f0, f1 };
		::polyfit(2, xx, yy, 1, factors);
	}

	double factor_y(int y) const
	{
		/** ��̽��Ƕ��Ǹ��ģ�����ǰ��ϵ���仯�������Եģ�
			ֱ������ǰ�ŵ����ŵ���ֵϵ��Ӧ������һ�������ߣ�ǰ�ű仯�죬���ű仯�� 
			��Ҫ���Ƴ�һ���� [0  video_height) �����ڵ������ߵ�ϵ��

			f(y) = a * y^2 + b * y + c

			��֪:
				f(0) = factor_0_;
				f(video_height_) = factor_1_;

			factor_0_ �� factor_1_ ֮��Ĺ�ϵ������ͼ���п��������ź�ǰ�š�ˮƽ�ȳ����ı���
			�����ٹ���һ���м�λ�õ�ˮƽ����(factor_0.5_)�����ܹ��õ� a,b,c ������

			FIXME: ���������߿϶����Ƕ��εģ��о�....

			���ڳ�����˵���߶γ��ȱ������������Եģ����ǵ�ǰ�ŵ����ǣ��ƺ����Բ������Թ��ɣ������м�ϵ���ͺ���ϵ����������Ϊ���Եľ�����

		 */
//		return factor_equation_y_[0] + factor_equation_y_[1] * y + factor_equation_y_[2] * y * y;
		return factor_equation_linear_y_[0] + factor_equation_linear_y_[1] * y;	//
	}

private:
	void try_object_detect();	// ���Խ��ж���ʶ�� ...
	void merge_rcs(const std::vector<cv::Rect> &rcs, const std::vector<int> &dirs, double stamp);
	void remove_bigger_smaller();
	void remove_up_of_last_motion(std::vector<cv::Rect> &rcs, std::vector<int> &dirs);
	std::vector<Target>::iterator find_matched(const cv::Rect &rc, int dir);
	bool is_target(const cv::Rect &rc, const Dir &dir);
	bool load_area_rect(const char *key, cv::Rect &rc)
	{
		const char *v = cfg_->get_value(key, 0);
		int left, top, right, bottom;
		if (v && 4 == sscanf(v, "(%d,%d),(%d,%d)", &left, &top, &right, &bottom)) {
			rc = cv::Rect(cv::Point(left, top), cv::Point(right, bottom));
			return true;
		}

		return false;
	}
	void update_motion_hist(const std::vector<cv::Rect> &rcs, const std::vector<int> &dirs);
	void remove_timeouted_motion_hist();
	std::vector<Motions> get_nearby_motion_hist(const cv::Rect &rc, const Dir &d);	// ������rc�ӽ���λ�õ���ʷ ...

	// ��־ʵ��.
	static std::string _log_fname;
	static void log_file(const char *fmt, ...)
	{
		va_list args;
		char buf[1024];

		va_start(args, fmt);
		vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);

		FILE *fp = fopen(_log_fname.c_str(), "at");
		if (fp) {
			time_t now = time(0);
			struct tm *ptm = localtime(&now);
			fprintf(fp, "%02d:%02d:%02d.%03d: %s", 
				ptm->tm_hour, ptm->tm_min, ptm->tm_sec, GetTickCount() % 1000, buf);
			fclose(fp);
		}
	}

	static void log_init_file(const char *fname)
	{
		_log_fname = fname;
		FILE *fp = fopen(fname, "w");
		if (fp) {
			fprintf(fp, "------ log begin ---------\n");
			fclose(fp);
		}
	}

	static void log_dummy(const char *fmt, ...)
	{
		(void)fmt;
	}

	static void log_init_dummy(const char *notused)
	{
		(void)notused;
	}
};
