#pragma once

#ifndef _libdetect_t_h_
#define _libdetect_t_h_
#include "opencv2/opencv1.hpp"
#include"blackboard_detect.h"
#include <string>
#include <cstdio>

#ifdef __cplusplus
extern "C" {
#endif
struct det_t
{
	KVConfig *cfg_;
	BlackboardDetecting *bd_detect_;
	IplImage *masked_;
	bool t_m;
	bool b_m;
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
char *det_detect(det_t * ctx, int height,int width,unsigned char *imgdata);//imgdata  YUV


#ifdef __cplusplus
};
#endif





#endif
