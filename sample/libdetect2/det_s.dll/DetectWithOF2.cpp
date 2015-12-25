#include "DetectWithOF2.h"


DetectWithOF2::DetectWithOF2(KVConfig *cfg)
	: Detect(cfg)
{
	od_ = cc_.load(cfg_->get_value("faces_meta_fname", "data/faces_12_12.xml"));
}

DetectWithOF2::~DetectWithOF2(void)
{
}

static std::vector<cv::Rect> _get_diff_brcs(const cv::Mat &gray_prev, const cv::Mat &gray_curr)
{
	cv::Mat diff;
	cv::absdiff(gray_prev, gray_curr, diff);
	cv::threshold(diff, diff, 20, 255, cv::THRESH_BINARY);

	cv::imshow("diff", diff);

	cv::Mat ker_erode_ = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	cv::Mat ker_dilate = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(11, 11));

	cv::erode(diff, diff, ker_erode_);
	cv::dilate(diff, diff, ker_dilate);

	cv::imshow("ed", diff);

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(diff, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

	std::vector<cv::Rect> brcs;

	for (std::vector<std::vector<cv::Point>>::const_iterator it = contours.begin(); it != contours.end(); ++it) {
		brcs.push_back(cv::boundingRect(*it));
	}

	return brcs;
}

/** ʵ�ֻ����ѧ��̽�⣺

		����֡��õ������
		���ٻ����
 */
std::vector<cv::Rect> DetectWithOF2::detect0(size_t st_cnt, cv::Mat &origin, cv::Mat &gray_prev, cv::Mat &gray_curr, cv::vector<int> &dirs)
{
	std::vector<cv::Rect> rcs;
	dirs.clear();

	// �õ�����֡��λ�õİ�������
	std::vector<cv::Rect> brcs_diff = _get_diff_brcs(gray_prev, gray_curr);

	// �ڸ���λ�ã�����ͷ�磬����ͷ����ΪĿ�������
	//if (od_) {
	//	// ����ͷ��ʶ��
	//	std::vector<cv::Rect> headers;
	//	detect_body(origin, 

	//	for (int i = 0; i < headers.size(); i++) {
	//		cv::rectangle(origin, headers[i], cv::Scalar(0, 0, 255), 3);
	//	}
	//}
	//else {
		// ��û�и���λ�õ�ѡ�������� ...
		std::vector<cv::Rect> new_position = get_new_position(brcs_diff);

		for (size_t i = 0; i < new_position.size(); i++) {		// �¼Ӹ���λ��
			cv::rectangle(origin, new_position[i], cv::Scalar(0, 0, 255));
		}

		// ��ʼ���µ�λ��
		for (std::vector<cv::Rect>::const_iterator it = new_position.begin(); it != new_position.end(); ++it) {
			Tracked t(this);
			if (t.init(gray_curr, *it)) {
				trackings_.push_back(t);
			}
		}

		// ���٣�ɾ����Ҫֹͣ���ٵ�λ��
		for (TRACKEDS::iterator it = trackings_.begin(); it != trackings_.end(); ) {
			it->track(gray_prev, gray_curr);
		
			cv::rectangle(origin, it->brc(), cv::Scalar(0, 255, 0));	// ���ڸ��ٵ�λ�ã�

			if (it->stopped()) {
				it = trackings_.erase(it);
			}
			else {
				it->draw_history(origin);

				++it;
			}
		}
	//}
	return rcs;
}

std::vector<cv::Rect> DetectWithOF2::get_new_position(const std::vector<cv::Rect> &all_diff_brcs)
{
	std::vector<cv::Rect> rcs;

	for (std::vector<cv::Rect>::const_iterator it = all_diff_brcs.begin(); it != all_diff_brcs.end(); ++it) {
		// ����Ƿ�����ٵ�֮���н��� 
		bool crossed = false;
		for (TRACKEDS::const_iterator itt = trackings_.begin(); !crossed && itt != trackings_.end(); ++itt) {
			crossed = is_cross(*it, itt->brc());
		}

		if (!crossed) {
			rcs.push_back(*it);
		}
	}

	return rcs;
}

DetectWithOF2::Tracked::Tracked(DetectWithOF2 *self)
{
	self_ = self;
}

bool DetectWithOF2::Tracked::init(const cv::Mat &gray, const cv::Rect &position)
{
	int cnt_max = sqrt(position.area()*1.0)+1, cnt_min = cnt_max / 2;	// ��࣬����

	cv::goodFeaturesToTrack(gray(position), features_, cnt_max, 0.1, 3);
	for (std::vector<cv::Point2f>::iterator it = features_.begin(); it != features_.end(); ++it) {
		it->x += position.x;
		it->y += position.y;

		pt_stamp_.push_back(self_->curr_stamp_);	// ÿ����ĳ�ʼʱ��
	}

	brc_ = position;

	return features_.size() >= cnt_min;
}

bool DetectWithOF2::Tracked::stopped()
{
	// �����Ƿ�ֹͣ�
	return !moving_;
}

void DetectWithOF2::Tracked::track(cv::Mat &prev, cv::Mat &curr)
{
	/** �� features_ ���и��٣������нϴ�λ�Ƶĵ� ...
	 */
	std::vector<cv::Point2f> next_features;
	cv::Mat status, err;

	cv::calcOpticalFlowPyrLK(prev, curr, features_, next_features, status, err);

	cv::Mat s = status.reshape(1, 1), e = status.reshape(1, 1);
	unsigned char *ps = s.ptr<unsigned char>(0), *pe = s.ptr<unsigned char>(0);

	std::vector<cv::Point2f> moved;

	for (int i = (int)features_.size()-1; i >= 0; i--) {	// FIXME: �Ӻ���ǰɾ������ frame_trackings_ ����� ...
		if (ps[i] == 1) {
			// ��Ч�� ...
			double dis = _distance(features_[i], next_features[i]);
			if (dis > 100) {
				// ��ʱ˵�������쳣�ˣ�
			}
			else if (dis > 3) {
				// �л��
				moved.push_back(next_features[i]);
				pt_stamp_[i] = self_->curr_stamp_;
				continue;
			}
			else if (self_->curr_stamp_ - pt_stamp_[i] < 0.300) {
				// ��ʱ���ڲ�����Ҳ����
				moved.push_back(next_features[i]);
				continue;
			}
		}

		// ɾ���ĵ��Ӧ��������ʷ���͵��ʱ��� ...
		remove_pt_from_history(i);
		pt_stamp_.erase(pt_stamp_.begin() + i);
	}

	std::reverse(moved.begin(), moved.end());	// ǰ���ǵ���������Ҫ��ת
	features_ = moved;	// �������ٵĵ� ...

	if (!moved.empty()) {
		brc_ = cv::boundingRect(moved);

		// ������ʷ֡ ...
		frame_trackings_.push_back(moved);
	}

	moving_ = !moved.empty();
}

Dir DetectWithOF2::Tracked::dir() const
{
	// ���ظ��ٵ�������˶����� ...
	return LEFT;
}

void DetectWithOF2::Tracked::distance(double &mean, double &max, double &min) const
{
}

void DetectWithOF2::Tracked::remove_pt_from_history(int n)
{
	for (std::deque<std::vector<cv::Point2f>>::iterator it = frame_trackings_.begin(); it != frame_trackings_.end(); ++it) {
		it->erase(it->begin() + n);	//
	}
}

void DetectWithOF2::Tracked::draw_history(cv::Mat &origin)
{
	for (size_t i = 1; i < frame_trackings_.size(); i++) {
		assert(frame_trackings_[i-1].size() == frame_trackings_[i].size());

		for (size_t j = 0; j < frame_trackings_[i].size(); j++) {
			cv::line(origin, frame_trackings_[i-1][j], frame_trackings_[i][j], cv::Scalar(0, 255, 255));
		}
	}

	cv::Rect brc = cv::boundingRect(frame_trackings_[0]);	// ��ʼλ��
	cv::rectangle(origin, brc, cv::Scalar(255, 0, 0));
}

bool DetectWithOF2::detect_body(const cv::Mat &origin, const cv::Rect &roi, std::vector<cv::Rect> &rcs)
{
	if (od_) {

	}

	return false;
}
