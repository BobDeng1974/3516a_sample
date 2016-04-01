#include "SkinMask.h"

SkinMask::SkinMask(KVConfig *cfg)
	: cfg_(cfg)
{
	ker_ = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	ker2_ = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));

	skin_thres_low_ = atoi(cfg_->get_value("skin_thres_low", "8"));
	skin_thres_high_ = atoi(cfg_->get_value("skin_thres_high", "20"));

	hair_thres_high_ = atoi(cfg_->get_value("hair_thres_high", "30"));
}

SkinMask::~SkinMask(void)
{
}

std::vector<std::vector<cv::Point> > SkinMask::find_skin_contours(const cv::Mat &origin)
{
	/** �� origin ת��Ϊ hsv �ռ䣬Ȼ���� H ����ֵ���������� ....
	 */

	cv::Mat hsv;
	cv::cvtColor(origin, hsv, cv::COLOR_BGR2HSV);
	std::vector<cv::Mat> channels;
	cv::split(hsv, channels);
	cv::Mat H = channels[0]; // H 0, S 1, V 2

	cv::threshold(H, H, skin_thres_low_, -1, cv::THRESH_TOZERO);	// ɾ��С��7��
	cv::threshold(H, H, skin_thres_high_, -1, cv::THRESH_TOZERO_INV); // ɾ������29��

	cv::erode(H, H, ker_);
	cv::dilate(H, H, ker2_);
	cv::blur(H, H, cv::Size(3, 3));

	std::vector<std::vector<cv::Point> > contours;
	cv::findContours(H, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
	return contours;
}

std::vector<std::vector<cv::Point> > SkinMask::find_hair_contours(const cv::Mat &origin)
{
	/** ͷ��һ���Ǻ�ɫ�ɣ���������ֵ����ƥ�䣿
	 */

	cv::Mat gray;
	cv::cvtColor(origin, gray, cv::COLOR_BGR2GRAY);
	
	cv::threshold(gray, gray, hair_thres_high_, 255, cv::THRESH_BINARY_INV);
	cv::erode(gray, gray, ker_);
	cv::dilate(gray, gray, ker_);

	std::vector<std::vector<cv::Point> > contours;
	cv::findContours(gray, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
	return contours;
}
