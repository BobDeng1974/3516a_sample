#pragma once

#include "detect.h"

/** ϣ�����Ͻ��������ϲ�
 */
class ClusterForBody
{
	KVConfig *cfg_;

public:
	ClusterForBody(KVConfig *cfg);
    std::vector<std::vector<cv::Point> > merge(const std::vector<std::vector<cv::Point> > &contours);
	~ClusterForBody(void);
};
