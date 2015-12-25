#pragma once
#include "detect.h"
#include <deque>
#include "objdet.h"

/** ����֡�Ȼ���֡�������е�ѡ�������㣬��������������и��٣�Ȼ����оۺ� ...
 */
class DetectWithOF2 : public Detect
{
	/** ����Ŀ�꣬�൱��һ�Ѿۺϵ������㣬��Щ���������ȽϽ����˶�����һ��
		����ЩĿ�����һ��ʱ�䡰�������󣬾���ΪĿ��ֹͣ�ˣ���ʧ�������ٽ���
	 */
	class Tracked
	{
		DetectWithOF2 *self_;
		cv::Rect brc_;
		std::vector<cv::Point2f> features_;	// ԭʼ������ ...
		std::vector<double> pt_stamp_;	// ��Ӧ features_ �����µ�ʱ��
		bool moving_;

		// ����֡�ĸ��ٵ�
		std::deque<std::vector<cv::Point2f>> frame_trackings_;	// ÿ���������ͬ ...

	public:
		Tracked(DetectWithOF2 *self);

		// �� position λ��ѡ�������� ...
		bool init(const cv::Mat &gray, const cv::Rect &position);

		// Ŀ�����¸��ٵ�λ�� ...
		cv::Rect brc() const { return brc_; }

		void track(cv::Mat &g0, cv::Mat &g1);	// ����

		// 
		bool stopped();

		// ���ظ��ٷ���
		Dir dir() const;

		// �����˶����룬����ƽ���������С
		void distance(double &mean_dis, double &max_dis, double &min_dis) const;

		// ������ʷ�켣��ÿ����� ...
		void draw_history(cv::Mat &origin);

	private:
		void remove_pt_from_history(int n);	// ɾ������֡��n�㣬������֤��ÿ֡����Ķ�����ͬ����
	};
	friend class Tracked;

	typedef std::vector<Tracked> TRACKEDS;
	TRACKEDS trackings_;	// ���ڱ����ٵĶ���

	bool od_;
	cv::CascadeClassifier cc_;

public:
	DetectWithOF2(KVConfig *cfg);
	~DetectWithOF2(void);

private:
	virtual std::vector<cv::Rect> detect0(size_t st_cnt, cv::Mat &origin, cv::Mat &gray_prev, cv::Mat &gray_curr, cv::vector<int> &dirs);

	// ��֡��صľ����У�ɾ�����ڸ��ٵ�λ�� ...
	std::vector<cv::Rect> get_new_position(const std::vector<cv::Rect> &all_diff_brcs);

	// �� roi �ڼ�� body�����ص� bodies ת��Ϊ origin ����
	bool detect_body(const cv::Mat &origin, const cv::Rect &roi, std::vector<cv::Rect> &bodies);

	// �������� trackings_���ٴκϲ�������ͬ�����˶���Ŀ��ϲ� ..
};
