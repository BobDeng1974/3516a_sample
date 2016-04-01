#pragma once

#include <opencv2/opencv1.hpp>
#include "../libkvconfig/KVConfig.h"

/** �������÷�ɫ��ֵ��������ͼ��ķǷ�ɫ���ֽ�������

		���ݾ���ֵ��hsv �У��˵ķ�ɫ H = [7, 23]

 */
class SkinMask
{
	KVConfig *cfg_;
	cv::Mat ker_, ker2_;	// ��ʴ����.
	int skin_thres_low_, skin_thres_high_;	// ��ɫ��ֵ������
	int hair_thres_high_;

public:
	SkinMask(KVConfig *cfg);
	~SkinMask(void);

	std::vector<std::vector<cv::Point> > find_skin_contours(const cv::Mat &origin);
	std::vector<std::vector<cv::Point> > find_hair_contours(const cv::Mat &origin);
};
