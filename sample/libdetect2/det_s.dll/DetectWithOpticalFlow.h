#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/ocl/ocl.hpp>
#include "detect.h"
#include <deque>
#include <opencv2/ocl/ocl.hpp>

/** ���ó��ܹ�������N֡�ۻ��õ������򳤾����ƶ����򡱣�Ȼ�������������
	���ǻ����˵��������ģ�
		1. ��վ����Ҫ700ms,�����ʱ���ڣ�Ӧ���������ĵ����˶���
		2. �첲�ȵĻӶ���������ۻ�Ч������Ϊ�ٶȺ͸첲���˶�����Ŀ��;
		3. �˵Ŀ��ٻζ��ᵼ���ۻ�������

	̽��ͷ�������Ƚ����أ���Ҫ�����ǰ����ϵ����Ŀǰ����2�ζ���ʽ��ϣ����о�����һ��ָ������ ...

	��һ�����ǣ�
		1. ò���� up �����Ĺ����ۻ������������Ƚ�С�����Կ���upʹ�ø�С�ľ���������ֵ��
		2. ��վ��ʱ���ƺ���������һ��left����right����̽��ͷ���ӽ��й�ϵ���Ĺ�����Ȼ������up���������ô�����

	���̷��˼·��
		������֡��ʷ��ͬ���������������ǰ��֡����ʷ���кϲ�����

 */
class DetectWithOpticalFlow : public Detect
{
	int debug2_, debug_img3_, debug_img4_;
	bool gpu_, sum_;
	double up_rc_aspect_;	// վ��ʱ����Ӿ��ε��ݺ�ȣ�����˵��Ӧ�� width < height * up_rc_aspect_
	cv::Mat ker_, ker2_;	// ��ʴ����.
	size_t preload_frames_;	// ��ҪԤ�ȼ��ص�֡��.
	typedef std::deque<cv::Mat> FIFO;
	cv::Mat sum_x_, sum_y_;	// x,y ʸ���ۻ���.
	FIFO saved_x_, saved_y_;	// ���� preload_frames ֡.
	FIFO saved_distance_;			// ����� preload_frames �ľ���
	cv::Mat sum_dis_;
	double thres_dis_flipper_;	// flipper ģʽ�µ��ۼƾ�����ֵ ...

	double fb_pyrscale_, fb_polysigma_;
	int fb_levels_, fb_winsize_, fb_iters_, fb_polyn_;

	int up_angle_;	// ����Ϊ���ϵĽǶ��Ƕ��٣��� 90�㣬60�� ...
	double up_tune_factor_;		// ���������ϵĹ�������������С���������������ֵ���Ը�ϵ����<1���������ϸ����׼�⵽.
	double lr_tune_factor_;		// ���һ����ֵ�Ӵ󣬿��Է�ֹվ���������һζ���Ӱ��.
	double down_tune_factor_;

	std::vector<std::vector<cv::Point> > flipped_group_polys_;	// ���ڱ��淭ת���õ�С���λ��

	cv::ocl::FarnebackOpticalFlow *flow_detector_fb_;

	cv::ocl::oclMat d_gray_prev_, d_gray_curr_, d_sum_x_, d_sum_y_, d_distance_, d_angle_;
	std::deque<cv::ocl::oclMat> d_saved_x_, d_saved_y_;
	bool d_first_;

	// ��¼ preloaded ֡�����������ĸ�����Ĺ�����ʷ
	class History
	{
		int size_, pos_, width_, height_;
		cv::Mat *left_, *right_, *up_, *down_;
		cv::Mat sum_;	// ���� ..

		void init(cv::Mat *p)
		{
			for (int i = 0; i < size_; i++) {
				p[i] = cv::Mat::zeros(height_, width_, CV_8U);
			}
		}

		// id = 0 ��Ӧ��ǰ pos_�� id = 1 ��Ӧ pos_ ֮ǰһ���� ...
		cv::Mat &prev(cv::Mat *p, int id) const
		{
			id %= size_;
			
			if (pos_ - id < 0) {
				return p[size_ + pos_ - id];
			}
			else {
				return p[pos_ - id];
			}
		}

	public:
		History(KVConfig *cfg)
		{
			size_ = atoi(cfg->get_value("preload_frames", "5"));
			width_ = atoi(cfg->get_value("video_width", "480"));
			height_ = atoi(cfg->get_value("video_height", "270"));
			pos_ = 0;

			left_ = new cv::Mat[size_], init(left_);
			right_ = new cv::Mat[size_], init(right_);
			up_ = new cv::Mat[size_], init(up_);
			down_ = new cv::Mat[size_], init(down_);
			sum_ = cv::Mat::zeros(height_, width_, CV_8U);
		}

		~History()
		{
			delete []left_;
			delete []right_;
			delete []up_;
			delete []down_;
		}

		cv::Mat &left(int preid) const { return prev(left_, preid); }
		cv::Mat &right(int preid) const { return prev(right_, preid); }
		cv::Mat &up(int preid) const { return prev(up_, preid); }
		cv::Mat &down(int preid) const { return prev(down_, preid); }
		cv::Mat &sum() { return sum_; } // ???

		void clr()
		{
			// ɾ��������ʷ ...
		}
		
		void operator++()
		{
			pos_++;
			pos_ %= size_;
		}

		History &operator++(int)
		{
			pos_++;
			pos_ %= size_;
			return *this;
		}
	};

	History of_history;

public:
	DetectWithOpticalFlow(KVConfig *cfg);
	~DetectWithOpticalFlow(void);

	virtual std::vector<cv::Rect> detect0(size_t cnt, cv::Mat &origin, cv::Mat &gray_prev, cv::Mat &gray_curr, cv::vector<int> &dirs)
	{
		return (this->*detect_00)(cnt, origin, gray_prev, gray_curr, dirs);
	}

	virtual int detect0(size_t cnt, cv::Mat &origin, cv::Mat &gray_prev, cv::Mat &gray_curr);

private:
	bool load_area_rect(const char *key, cv::Rect &rc);

	enum DIR
	{
		DIR_RIGHT,
		DIR_DOWN,
		DIR_LEFT,
		DIR_UP,
	};

	DIR get_dir(const cv::Mat &dirs_mat, const cv::Point &center, const cv::Rect &boundingRect, cv::Mat &origin, 
		const cv::Scalar &color = cv::Scalar(0, 255, 255));

	void draw_optical_flows(cv::Mat &origin, const cv::Mat &dis, const cv::Mat &angs);

	void (DetectWithOpticalFlow::*calc_flows)(cv::Mat &gray_prev, cv::Mat &gray_curr, cv::Mat &distance, cv::Mat &angles);
	void (DetectWithOpticalFlow::*calc_flow)(cv::Mat &gray_prev, cv::Mat &gray_curr, cv::Mat &distance, cv::Mat &angles);
	std::vector<cv::Rect> (DetectWithOpticalFlow::*detect_00)(size_t cnt, cv::Mat &origin, cv::Mat &gray_prev, cv::Mat &gray_curr, cv::vector<int> &dirs);

	inline bool get_flow(const cv::Mat &gray_curr, cv::ocl::oclMat &x, cv::ocl::oclMat &y);
	double get_roi_flow_sum(const cv::Mat &dis, const std::vector<cv::Point> &roi, cv::Mat &rgb);
	void calc_flows_ocl(cv::Mat &gray_prev, cv::Mat &gray_curr, cv::Mat &distance, cv::Mat &angles); // �����ۻ�����..
	void calc_flows_cpu(cv::Mat &gray_prev, cv::Mat &gray_curr, cv::Mat &distance, cv::Mat &angles);
	void calc_flow_ocl(cv::Mat &gray_prev, cv::Mat &gray_curr, cv::Mat &distance, cv::Mat &angles);
	void calc_flow_cpu(cv::Mat &gray_prev, cv::Mat &gray_curr, cv::Mat &distance, cv::Mat &angles);
	void rgb_from_dis_ang(const cv::Mat &dis, const cv::Mat &angs, cv::Mat &rgb);	// ���� dis, angs ����rgb��������ʾ..
	void load_flipped_polys(std::vector<std::vector<cv::Point> > &polys);
	inline void apply_threshold(const cv::Mat &dis, double factor, cv::Mat &bin);	// ��dis��������factor�����ض�ֵͼ�����ڸ�ʴ���� ...

	void update_history(const cv::Mat &dis, const cv::Mat &angles);	// �ĸ�����Ĺ��������浽 History ��..
	void update_history0(const cv::Mat &dis, const cv::Mat &dir, cv::Mat &target, int from, int to, int from2 = -1, int to2 = -2);

	std::vector<cv::Rect> detect_with_history(size_t cnt, cv::Mat &origin, cv::Mat &gray_prev, cv::Mat &gray_curr, cv::vector<int> &dirs);
	std::vector<cv::Rect> detect_with_sum(size_t cnt, cv::Mat &origin, cv::Mat &gray_prev, cv::Mat &gray_curr, cv::vector<int> &dirs);

	struct FindTargetResult
	{
		cv::Rect brc;
		double area, area_th;
		double fy;
	};

	std::vector<FindTargetResult> find_targets(cv::Mat &(History::*dir)(int) const, double area_factor, int frames, cv::Mat &debug_sum);
	void show_history(const std::vector<FindTargetResult> &rs, cv::Mat &(History::*dir)(int) const, cv::Mat &m0)
	{		
		m0 = (of_history.*dir)(0).clone();
		for (size_t n = 1; n <= preload_frames_-1; n++) {
			m0 |= (of_history.*dir)(n);
		}

		cv::blur(m0, m0, cv::Size(3, 3));
		cv::erode(m0, m0, ker_);
		cv::dilate(m0, m0, ker2_);

		for (size_t i = 0; i < rs.size(); i++) {
			cv::rectangle(m0, rs[i].brc, cv::Scalar(255));
			char buf[64];
			snprintf(buf, sizeof(buf), "%.2f|%.0f,%.0f", rs[i].fy, rs[i].area, rs[i].area_th);
			cv::putText(m0, buf, rs[i].brc.tl(), cv::FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(255));
		}
	}
};
