#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include "ImageArry.h"
#include "FileTan.h"
#include <time.h>

//#define WIDTH 320
//#define HEIGHT 180

unsigned long GetTickCount();

#define CONDIDATECOUNT 10

struct upStudentTarget
{
	long tickcount;//����ʱ��ʱ���
	cv::Rect position;//Ŀ������

	int tag;//0 վ��������Ч 1 ʱ�䵽��ȡ�� 2 ���������¹���ȡ��  3 �й������ȡ��  4 �������ƶ��Ĺ����뿪������
	int deletecount;//ɾ��������ֻ����tag>0ʱ����Ч��Ϊ0ʱ��ɾ������Ϊ0ʱÿ�δ����һ

	cv::Rect pos0,pos1,pos2;//���������� �����õ���������ͼ�������0 ���� 1 ǰһ�� 2 ǰ2��
	int showposcount;

	cv::Mat pregray2,pregray,curgray;//վ����֮ǰ�ĻҶ�ͼ

};


class CStudentTrack
{
public:
	CStudentTrack(void);
	virtual ~CStudentTrack(void);

	void setParam(int yfirst,int standupareafirst,int widthfirst,int ylast,int standuparealast,int widthlast);

	// ������
	void process(const cv::Mat& img_color);

	void setduration(int sec);

	void setdebug(bool b);

	void readconfig(char *cfg_name);

	void start();

	void set_maskpoint(cv::Point p);

	std::vector<upStudentTarget> up_students;//վ����������

	long tt,t1,t2,t3,t4,t5,t6,t7,t8;//��¼����ʱ��
	long flowcount;

	std::vector<upStudentTarget> up_studentsTmp;//��ʱ����չʾ��ʱ����
	void showStudenttmp();
	int ishowtmp;
	int height_input,width_input;
private:
	int y_firstline,y_lastline;
	int width_first,width_last;
	int standuparea_first,standuparea_last;

	int frameNum;
	int min_student_area;//Ѱ�ҹ��������� ��С���10*10
	int up_student_duration;//������ѧ�� ʱ�� �ڣ��������� �͵��� �Ѿ���������msΪ��λ

	cv::Rect MaxRect,MinRect;
	float splitvalue;

	cv::Mat pre_gray;
	cv::Mat cur_gray;
	cv::Mat cur_color;
	cv::Mat flowimg;//���ӹ����Ĳ�ɫͼ��
	cv::Mat flowupbw,flowdownbw;//���¹�����ֵ��ͼ

	cv::Mat flowup,flowdown;
	cv::Mat flowleft,flowright;

	cv::Mat fgmask,imgdiff;

	cv::Mat imgdiffforflow;

	cv::Mat flowmove;

	CImageArry imageArry;

	CFileTan fileTan;

	bool is_debug;
	void showDebug();

	//��������
	std::vector<cv::Point> mask_points;


	//������ͨ��
	cv::Ptr<cv::DenseOpticalFlow> tvl1;

	//������ģ
	cv::BackgroundSubtractorMOG *bg_model;

	cv::Mat modelBG;

	//ѧ��վ�𲿷�
	int ican_head;
	std::vector<cv::Rect> up_student_candidates[CONDIDATECOUNT];
	int getNewCondidate();
	int getPreCondidate(int prenum);//0 ��ǰ���µģ�1 ǰһ����2 ǰ2����...


	void checkupstudent(cv::Mat mat);//Ѱ�ҹ��������䣬�������� �����µĹ������ҹ���������Ҳ�����������

	std::vector<upStudentTarget> up_studentsSave;//վ���������� �����������жϸ����� ������1�����и������Ĺ������ų� ѧ����������������������� �����Ĺ���
	bool checkupStudentSave(cv::Rect rc);//�ж�վ��Ĵ�ѡ�����Ƿ����ڸ���ʧ�����������ų�ѧ����������������

	bool checkupOnlyUpflow(cv::Rect rc);//�жϸ������Ƿ����ϵĹ������
	bool checkupfg(cv::Rect p0,cv::Rect p1,cv::Rect p2,cv::Rect p);//ͨ��ǰ�����жϣ�Ҫ�ı��� ������� ����վ����

	//bool check


	//ѧ�����²���
	int ican_head_down;
	std::vector<cv::Rect> down_student_candidates[CONDIDATECOUNT];
	int getNewCondidatedown();
	int getPreCondidatedown(int prenum);//0 ��ǰ���µģ�1 ǰһ����2 ǰ2����...
	void checkDownStudentByFlow(cv::Mat mat);//Ѱ�ҹ��������䣬�������� �����µĹ������ҹ���������Ҳ��������������� ��ȥվ������
	void checkDownStudentByArea();//�������������ͼ��Ĺ�����������ж��Ƿ��ȥվ������
	void checkDownStudentNormal();//����ʱ���Ƿ�����ɾ��

	////ѧ���߿�
	std::vector<cv::Rect> move_student[CONDIDATECOUNT];//�����ж� �Ƿ� ѧ���߿�
	void checkMoveStudentByFlow(cv::Mat mat);//�ж�ѧ���Ƿ��Ѿ��߿����ƶ��Ĺ��� �Ƿ������� �� �뿪վ�������

	void showStudentup();

	void AddUpstudent(cv::Rect rc,cv::Rect p0,cv::Rect p1,cv::Rect p2);
	void readyDeleteUpstudent(std::vector<upStudentTarget>::iterator &it,int itag,int ideletecount=5);//׼��ɾ���ڵ�

	bool InitAndFlow();//�����Ƿ��й�����

	//���ܺ���
	bool haveSameRect(cv::Rect r1,cv::Rect r2);
	cv::Rect getMaxRect(cv::Rect r1,cv::Rect r2);

	int getMaxFlowDis(cv::Rect p0,cv::Rect p1,cv::Rect p2,int i1=0,int i2=1,int i3=2);
	int getAvrFlowDis(cv::Rect p0,cv::Rect p1,cv::Rect p2,int i1=0,int i2=1,int i3=2);
	int getMaxFlowDis(cv::Rect p0);
	int getAvrFlowDis(cv::Rect p0);

	int getMinArea(cv::Rect r);

	bool checkHead(cv::Mat img1,cv::Mat img2,cv::Mat img3,int headwidth,int upheight,cv::Mat img4,cv::Mat img5,cv::Mat img6);



	//����ͼ�����
	bool bdebugsaveimage;
	void saveimage();

};

