#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

#define ARRAYNUM 10
class CImageArry
{
private:
	bool mask_on;
	int mHeigth;
	int mWidth;
	int mIndex;
	long mAddImageCount;
public:
	CImageArry(void);
	~CImageArry(void);

	cv::Mat imagecoo[ARRAYNUM];//��ԭʼͼ��
	cv::Mat imagegraycoo[ARRAYNUM];//ԭʼͼ��Ҷ�ͼ

	cv::Mat imagecolor[ARRAYNUM];//��С��Ĳ�ͼ
	cv::Mat imagegray[ARRAYNUM];//��С��ĻҶ�ͼ

	cv::Mat imagefg[ARRAYNUM];//������ģ���ȡ��ǰ��ͼ��
	cv::Mat imageflowup[ARRAYNUM];//վ�����Ĺ���(Ŀ�ĵ�)
	cv::Mat imageflowdown[ARRAYNUM];//�������Ĺ���
	cv::Mat imageflowupfrom[ARRAYNUM];//վ�����Ĺ���(Դ��)

	cv::Mat imageflowleft[ARRAYNUM];
	cv::Mat imageflowright[ARRAYNUM];
	
	cv::Mat imagecolorflow[ARRAYNUM];//��С��Ĳ�ͼ

	cv::Mat imageflowupdown[ARRAYNUM];//�ۼƎ������¹��������������� ���� �ӽ�������ȥ�����ǎ��Ĺ�������һ�� ֻ��¼����  �� �����½���� �����

	cv::Mat imageflowupdowncur[ARRAYNUM];

	cv::Mat maskcolor;

	int getHeight();
	int getWidth();

	bool AddNewImage(cv::Mat icolorimage);
	bool SetMask(std::vector<cv::Point> maskpoints);
	int newindex();

	int preindex(int prenum=0);//0 ��ǰ���µģ�1 ǰһ����2 ǰ2����...
};

