#pragma once

#define _USE_MATH_DEFINES

#include "Detect.h"
#include "DiffMotion.h"
#include "Target.h"

/** ʹ��ϡ�������¼����������ֹͣ�󣬼���Ĺ켣��������Ч�Ļ ...
 */
class DetectWithOf : public Detect
{
	DiffMotion dm_;
	std::vector<Target> targets_;	// �Ŀ�� ...
	std::vector<Target> long_timed_targets_;	// ���ԣ���ʱ�����Ŀ�꣬����Ч�� ...

	// ѧ��������ţ���ǰ�ŵ�λ�ã��ڴ�֮�䣬ʹ��������� ...
	float far_y_, near_y_;	// ���� y �� ...
	
	float far_width_, near_width_;	// ֡����ε�����y�Ŀ����ֵ�����С�ڸ�ֵ������Ϊ֡�����̫С�� ...
	double a_width_, b_width_;		// ��ȵ�ֱ�߷���ϵ�� ...
	
	float far_dis_[4], near_dis_[4];	// �ĸ�����ľ�����ֵ ...
	double a_dis_[4], b_dis_[4];	// 
	
	int up_angle_;		// 

	int next_tid_;		// Target ID

public:
	explicit DetectWithOf(KVConfig *cfg);
	~DetectWithOf();

	virtual void reset();

private:
	virtual void detect(RCS &motions, std::vector<Dir> &dirs);

	std::vector<Target>::iterator find_matched_target(const cv::Rect &rc)	// �ҵ��� rc �ཻ�� target ...
	{
		for (std::vector<Target>::iterator it = targets_.begin(); it != targets_.end(); ++it) {
			if (it->is_crossed(rc)) {
				return it;
			}
		}
		return targets_.end();
	}

	bool too_small(const cv::Rect &rc) const;

	enum {
		E_OK,
		E_NotEnoughLayers,
		E_NotEnoughPaths,
		E_NotEnoughDistance,
	};

	/// ����target�Ļ�������Ч���򷵻� OK
	int estimate(const Target &target, cv::Rect &pos, Dir &dir) const;

	/// ����������ֱ�ߵ� a,b ϵ��
	inline void polyfit_line(const cv::Point2f &p1, const cv::Point2f &p2, double &a, double &b) const
	{
		cv::Point2f p = p2;
		if (p.x == p1.x) {	 // NOTE: ��ֹ��ֱֱ�� ...
			p.x += (float)0.01;
		}

		a = (p1.y - p.y) / (p1.x - p.x);
		b = p.y - a * p.x;
	}

	/// �������㣬��Ƕȣ��� x ��˳ʱ����תһ�ܣ�[0..360) 
	inline double calc_angle(const cv::Point2f &p1, const cv::Point2f &p2) const
	{
		double a = atan2l(-(p2.y - p1.y), p2.x - p1.x) * 180.0 / M_PI;
		if (a > 0) {  // ��һ������ ..
			a = 360.0 - a;
		}
		else {	// �������� ..
			a = -a;	
		}
		
		return a;
	}
};
