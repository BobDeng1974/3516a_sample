#pragma once
#include "detect.h"

/** ���ڳ��ܹ������ɼ���֮֡�������� 2 �ĵ㣬���ݷ��򣬽����ں�
 */
class DetectWithOF3 : public Detect
{
	cv::ocl::FarnebackOpticalFlow of_;	//
	double fb_pyrscale_, fb_polysigma_;
	int fb_levels_, fb_winsize_, fb_iters_, fb_polyn_;
	cv::ocl::oclMat d_gray_prev_, d_gray_curr_, d_distance_, d_angle_, d_dx_, d_dy_;
	bool d_first_;

	double threshold_optical_flow_;	// ��֮֡��Ĺ�����ֵ��ȱʡ 3.0
	
	cv::Mat ker_erode_, ker_dilate_;

	int up_angle_; // ���ϵĽǶȷ�Χ��Ĭ�� 110

	int merge_mode_;	// �ϲ�ģʽ��1: ������ϲ���2: ��������ʷ��ӿ�ϲ�

	double delay_;	// �ж�Ŀ�����ӳ٣�����˵���� delay_ ��󣬿�ʼ���� Target ����Ϊ��ȱʡ 0.3

	int debug_lmax_, debug_lmin_;
	double debug_max_dis_;

	double area_factor_ab_[2];	// ����仯ʹ��ֱ�ߣ��� y=ax+b
	double area_bottom_y_;	// ���ţ����ţ��о�Ŀ������仯�ȽϾ��ȣ���ǰ����Ҫ�ܴ󣡣����ò�������ǰ�ŵķָ�λ�ã�ȱʡ 0.667
	double area_bottom_max_, area_max_, area_min_;	// ���� Detect.h �е� ...���� area_max_, area_min_ �����к��ŵ� Target ���,
	double area_bottom_min_;						// area_bottom_max_ area_bottom_min_, ������ǰ�� ...

	// �ĸ������ͳ��
	struct DirCnt
	{
		int left, right, up, down;
		double dis_left, dis_right, dis_up, dis_down;	// ÿ������ľ����ۼƺ�
	};

	// ��¼һ��λ�ã�һ��ʱ�䣬
	struct Target
	{
		double stamp_first, stamp_last;	// ����ʱ�䣬������ʱ��

		cv::Rect rc;	// �����Ӿ��Σ��ϲ� ...

		std::deque<cv::Rect> hist_rcs;	// ��¼����
		std::deque<int> hist_dirs;	// ��ʷ����
		std::deque<cv::Mat> hist_of_dx;	// �����ڵĹ�����dx,dy ��ʽ
		std::deque<cv::Mat> hist_of_dy;
		std::deque<DirCnt> hist_dir_cnt;	// ÿ֡�е��ĸ������ͳ��
	};

	typedef std::vector<Target> TARGETS;
	TARGETS targets_;	//

public:
	DetectWithOF3(KVConfig *cfg);
	~DetectWithOF3(void);

private:
	virtual std::vector<cv::Rect> detect0(size_t st_cnt, cv::Mat &origin, cv::Mat &gray_prev, cv::Mat &gray_curr, cv::vector<int> &dirs);

private:
	void calc_optical_flow(const cv::Mat &p0, const cv::Mat &p1, cv::Mat &dis, cv::Mat &ang, cv::Mat &dx, cv::Mat &dy);
	void show_optical_flow(cv::Mat &origin, const cv::Mat &dis, const cv::Mat &ang);
	void get_motion_rects(const cv::Mat &dis, const cv::Mat &ang, std::vector<cv::Rect> &rcs, std::vector<int> &dirs,
		std::vector<DirCnt> &dircnts);
	Dir get_roi_property(const cv::Rect &roi, const cv::Mat &dis, const cv::Mat &dis_bin, const cv::Mat &ang, DirCnt &dc);
	inline Dir ang2dir(float ang) const;
	void merge_targets(const std::vector<cv::Rect> &motion_rcs, const std::vector<int> &dirs, 
		const std::vector<DirCnt> &dircnts, const cv::Mat &dx, const cv::Mat &dy);
	void merge_targets();	// 
	bool analyze_target_hist(const Target &target, Dir &dir, cv::Rect &rc) const;	// �����ȷ֪����ĳ���������򷵻�true��������dir�ͻ����
	bool calc_area(int y, double &area_min, double &area_max) const;	// ����y�ᣬ�������ϵ�� ...�������Ƿ� bottom 
};
