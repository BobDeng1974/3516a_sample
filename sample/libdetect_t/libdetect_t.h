#pragma once

#ifndef _libdetect_t_h_
#define _libdetect_t_h_
#include<opencv2/opencv1.hpp>
#include "detect_t.h"
#include "blackboard_detect.h"
#include "StudentTrack.h"
//#include "../libimagesource/image_source.h"
#include <string>
#include "hog.h"

#ifdef __cplusplus
extern "C" {
#endif

struct zifImage
{
    int fmt_type;
    int width;
    int height;
    unsigned char *data[4];
    int stride[4];
};

struct det_t
{
	KVConfig *cfg_;
	TeacherDetecting *detect_;
	BlackboardDetecting *bd_detect_;
    CStudentTrack *stu_detect_;
	zk_hog_detector *hog_det;
	IplImage *masked_;
	bool t_m;
	bool b_m;
    bool s_m;
	std::string result_str;	// FIXME: ϣ���㹻��;
};

typedef struct det_t det_t;
typedef struct zifImage zifImage;
/** ����ʵ��
@param cfg_name: �����ļ�����
	@return det_tʵ����0 ʧ��
 */
det_t *det_open(const char *cfg_name);

void det_close(det_t *ctx);


/**
	@param img: ͼ��
	@return  ,json��ʽ--   {"stamp":12345,"rect":[{"x":0,"y":0,"width":100,"height":100},{"x":0,"y":0,"width":100,"height":100}]}





	{
		"stamp":12345,
		"rect":
		[
			{"x":0,"y":0,"width":100,"height":100},
			{"x":0,"y":0,"width":100,"height":100}
		]
	}
 */
//char *det_detect(det_t *ctx, zifImage *img);
char *det_detect(det_t *ctx, cv::Mat &img);


#ifdef __cplusplus
};
#endif





#endif
