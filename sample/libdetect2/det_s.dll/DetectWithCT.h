#pragma once
#include "detect.h"
#include "ct/CompressiveTracker.h"

/** ����ct�ĸ���

	1. ֡��ҵ��ϴ����� MotionRect
	2. �� MotionRect �������С�ͷ�硱��
	3. ÿ��ͷ����ΪĿ�꣬��ʼ�������� CT ���и��٣�
	4. �� CT Ŀ���С�ʱ������ΪĿ��վס����ʱ����Ŀ���˶���ʷ���ж�Ŀ���������ҵĻ;
 */
class DetectWithCT : public Detect
{
	/** �� CT ���ٵ�Ŀ��
	 */
	struct Target
	{
		static Target *new_target(const cv::Rect &header, double stamp)
		{
			Target *t = new Target;
			t->hist_rcs.push_back(header);
			t->stamp_begin = stamp;
		}

		double stamp_begin, stamp_last_updated;
		std::deque<cv::Rect> hist_rcs;	// ��ʷλ�ã���ౣ��10֡���㹻��..
		
		cv::Rect rc() const { return hist_rcs.back(); }
	};

	typedef std::list<Target> TARGETS;
	TARGETS targets_;			// ��ǰ���ڸ��ٵ�Ŀ��

	cv::Mat ker_erode_, ker_dilate_;

	cv::CascadeClassifier cc_;

public:
	DetectWithCT(KVConfig *cfg);
	~DetectWithCT(void);

private:
	virtual std::vector<cv::Rect> detect0(size_t st_cnt, cv::Mat &origin, cv::Mat &gray_prev, cv::Mat &gray_curr, cv::vector<int> &dirs);

	// ͨ��֡��ںϣ��õ��˶�����
	RECTS get_motion_rects_using_diff(const cv::Mat &prev, const cv::Mat &curr);

	// ����"ͷ�硰
	RECTS get_headers(const cv::Mat &range);
};
