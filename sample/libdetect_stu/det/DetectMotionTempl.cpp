#include "DetectMotionTempl.h"

DetectMotionTempl::DetectMotionTempl(KVConfig *cfg)
	: Detect(cfg)
{
}

DetectMotionTempl::~DetectMotionTempl(void)
{
}

std::vector<cv::Rect> DetectMotionTempl::detect0(cv::Mat &origin, cv::Mat &gray_prev, cv::Mat &gray_curr, cv::vector<int> &dirs)
{
	dirs.clear();
	std::vector<cv::Rect> rcs;

	/** TODO: ͨ��֡��õ��仯λ�ã��ٸ�����Щλ�� ?
	 */

	return rcs;
}
