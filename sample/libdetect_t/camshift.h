#pragma once

#include <opencv2/opencv1.hpp>
#include <cstdio>
#include <iostream>
//Ŀǰʹ������: ;
//1. ����tracker_camshift�����, ����set_hist(const char *file_name)��ȡԤ���ֱ��ͼ;
//2. ����set_track_window()���ó�ʼ���ٿ�, �ɽ�hogȷ���ľ��ο���Ϊ����;
//3. ����process(), is_erode��Ϊtrue, ��ȷ������λ��;
//4. �����������ͷ���Ĺ�ϵȷ���Ƿ�Ϊ����;
// ������ͷ, �ظ����沽��;
//5. ���������ʵ���С,  ����set_hist(cv::Mat  &image_bgr, const cv::Rect &rect, int hsize, int ssize)��������ֱ��ͼ;
//6. ��������process(), is_erode�����Ϊtrueʱ�ɼ�С�����, �����׸���;
//7. ������ʱ��process���ص�Rect.area() < 1;

class tracker_camshift
{
public:
	tracker_camshift(const char* yaml_file);
	//������ʾ����״̬,  һ�����ڲ���;
	cv::Mat get_backproj(const cv::Mat &image_bgr);
	cv::Rect process(const cv::Mat &image, bool is_erode = true);
    void set_track_window(const cv::Rect &position);
	void set_hist(cv::Mat  &image_bgr, const cv::Rect &rect, int hsize = 60, int ssize = 128, int threshold_value = 50);
	void set_hist(const char *file_name);
	bool is_face(cv::Rect &face, cv::Rect upper_body);
	bool cam_shift_init_once(cv::Rect &face,cv::Mat image);
	~tracker_camshift();

private:
	cv::Rect track_window;
	cv::Mat hist;
	int vmin;
	int vmax;
	int vthreshold;
	int hist_size[2];
	float hranges[2];
	float sranges[2];
};


