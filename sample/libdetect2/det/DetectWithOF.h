#pragma once

#include "detect.h"
#include <deque>

// ������Ч��ļ�������
static bool _center(const std::vector<cv::Point2f> &pts, cv::Point2f &center, const cv::Mat &valid = cv::Mat())
{
	const unsigned char *s = valid.ptr<unsigned char>(0);
	int n = 0;
	double x = 0.0, y = 0.0;

	if (valid.cols == pts.size()) {
		for (size_t i = 0; i < pts.size(); i++) {
			if (s[i]) {
				x += pts[i].x, y += pts[i].y;
				n++;
			}
		}
	}
	else {
		for (size_t i = 0; i < pts.size(); i++) {
			x += pts[i].x, y += pts[i].y;
			n++;
		}
	}

	if (n > 0) {
		center.x = x/n, center.y = y/n;
		return true;
	}
	else {
		return false;
	}
}

/** ����ϡ������ĸ��� ..

		1. ͨ��֡�����ȷ�������
		2. �ڻ�����У����С�ͷ�硱ʶ�𣬵õ��������ˣ�
		3. �ڡ��ˡ���Χ���ҽǵ㣬
		4. �ԡ��ǵ㡱���и���
		5. �������ٵ㲻�ٻʱ�����θ��ٽ���
 */
class DetectWithOF : public Detect
{
	int debug_img_;	//
	cv::Mat ker_erode_, ker_dilate;

	/** ������Ŀ�꣬
			1. �����ֽϴ�֡����ʱ����ʼ���٣� ��������Ŀ��󣬲��ٹ��Ĵ˴�֡�� ..
			2. ��Ŀ�귶Χ�ڣ�goodFeaturesToTrack�����С����ֵ���������Ŀ�� ...
			3. calcOpticalFlowPyrLK��ɾ������㣬ɾ������С����ֵ�ĵ㣬��� features ��ĿС����ֵ������ΪĿ����ٽ��� ...
			4. ѭ�� 3
	 */
	class TrackingRect
	{
		cv::Mat origin_;
		cv::Rect rc_start_;	// ��ʼλ��
		cv::Rect boundingRect_;	// ��ǰλ��
		cv::Rect last_boundingRect_;
		std::vector<cv::Point2f> features_;
		cv::Point2f center_start_;
		int cnt_min_, cnt_max_; // ���ٹؼ��� ..
		bool moving_;

		typedef std::deque<std::vector<cv::Point2f> > ALL_POINTS;
		ALL_POINTS all_points_;	// ���и��ٵ� ...

	public:
		bool init(const cv::Mat &origin, const cv::Mat &gray, const cv::Rect &rc)
		{
			origin_ = origin, rc_start_ = rc;
			cnt_max_ = sqrt(rc.area()*1.0)+1, cnt_min_ = cnt_max_ / 2;	// ��࣬����

			cv::goodFeaturesToTrack(gray(rc), features_, cnt_max_, 0.1, 3);
			for (size_t i = 0; i < features_.size(); i++) {
				features_[i].x += rc.x;	// ת��Ϊ����ͼ�������
				features_[i].y += rc.y;
			}

			// ������ʷ
			all_points_.push_front(features_);	// ��һ֡��ʷ

			_center(features_, center_start_);

			boundingRect_ = rc;
			last_boundingRect_ = rc;
			moving_ = true;

			return features_.size() > cnt_min_;
		}

		cv::Rect boundingRect() const { return boundingRect_; }		// ������
		cv::Rect last_boundingRect() const { return last_boundingRect_; } // ����

		void remove_hist_points(int n)
		{
			// n ��Ӧ�Ŵ�����λ��
			for (size_t i = 0; i < all_points_.size(); i++) {
				std::vector<cv::Point2f> &ps = all_points_[i];
				int x = 0;
				for (std::vector<cv::Point2f>::iterator it = ps.begin(); it != ps.end(); it++, x++) {
					if (x == n) {
						ps.erase(it);
						break;
					}
				}
			}
		}

		void draw_hist(cv::Mat &origin)
		{
			for (int i = 1; i < all_points_.size(); i++) {
				std::vector<cv::Point2f> &l0 = all_points_[i-1], &l1 = all_points_[i];

				if (l0.size() == l1.size()) {
					for (int j = 0; j < l0.size(); j++) {
						cv::line(origin, l0[j], l1[j], cv::Scalar(0, 255, 0));
					}
				}
			}
		}

		// ���ص�n����Ļ�켣��ʷ ..
		bool get_pt_hist(int n, std::vector<cv::Point2f> &path)
		{
			if (all_points_.empty() || n >= all_points_[0].size()) {
				return false;
			}

			path.clear();
			ALL_POINTS::const_reverse_iterator it;
			for (it = all_points_.rbegin(); it != all_points_.rend(); it++) {
				path.push_back((*it)[n]);
			}

			return true;
		}

		/// ���ػ�ĵ���Ŀ
		int get_moving_points() const
		{
			assert(!all_points_.empty());	// track() ���غ󣬲��ܵ���
			return all_points_[0].size();
		}

		/** ������֡ͼ����и��� ...
		 */
		bool track(const cv::Mat &prev, const cv::Mat &curr, cv::Rect &boundingRc, std::vector<cv::Point2f> &next_pts)
		{
			std::vector<cv::Point2f> next_features;
			cv::Mat status, err;

			cv::calcOpticalFlowPyrLK(prev, curr, features_, next_features, status, err);

			cv::Mat s = status.reshape(1, 1), e = status.reshape(1, 1);
			unsigned char *ps = s.ptr<unsigned char>(0), *pe = s.ptr<unsigned char>(0);

			std::vector<cv::Point2f> moved;

			bool moving = false;

			for (size_t i = 0; i < features_.size(); i++) {
				if (ps[i] == 1 && (next_features[i].x > 0 && next_features[i].y > 0 && next_features[i].x < origin_.cols-1 && next_features[i].y < origin_.rows-1)) {
					if (_distance(next_features[i], features_[i]) >= 3) {	// �����нϴ�λ��
						moving = true;	// ���ڻ�� ...
						moved.push_back(next_features[i]);
					}
				}
				else {
					// ����㣬ɾ����ʷ ...
					remove_hist_points(i);
				}
			}

			if (moving) {
				all_points_.push_front(moved);

				//next_pts = keeper;
				next_pts = moved;

				boundingRc = cv::boundingRect(moved);

				//boundingRc |= this->rc_start_;
				boundingRect_ = boundingRc;
				//features_ = keeper;	// ���¸��ٵ�
				features_ = moved;

				last_boundingRect_ = cv::boundingRect(moved);	// ��ĵ���

				// ����ÿ��������ʷ�켣 ...
			}
			else {
				moving_ = false;	// ֹͣ���� ...
			}

			return moving;
		}

		bool moving() const { return moving_; }

		bool get_result(cv::Rect &rc, int &dir)
		{
			// ������ȶ����򷵻�������Ӿ��Σ����˶�����
			if (get_moving_points() > 0) {
				rc = boundingRect_;
				dir = get_dir();
				return true;
			}
			else {
				return false;
			}
		}

		// ���е��ƽ������
		cv::Point2f mean_of_points(const std::vector<cv::Point2f>&pts) const
		{
			assert(!pts.empty());

			double sx = 0, sy = 0;
			for (size_t i = 0; i < pts.size(); i++) {
				sx += pts[i].x, sy += pts[i].y;
			}

			return cv::Point2f(sx/pts.size(), sy/pts.size());
		}

		// 0=right, 1=down, 2=left, 3=up
		int get_dir()
		{
			// �򵥵ļ����������ĵ�����������ĵ�ƫ�� ...
			cv::Point2f start = mean_of_points(all_points_.back());
			cv::Point2f stop = mean_of_points(all_points_.front());

			double a = atan2(start.y-stop.y, stop.x-start.x) * 180 / M_PI;	// ע�⣺y ��Ϊ��

			if (a < 0) {
				a = 0.0 - a;	// �ң��£���
			}
			else {
				a = 360 - a;	// ���ϣ���
			}

			int dir_idx = 1;	// Ĭ�� down
			int up_half = 110 / 2;
			int up_min = 270 - up_half, up_max = 270 + up_half;
			if (a >= up_min && a <= up_max) {
				dir_idx = 3;	// ��
			}
			else if (a >= 45 && a <= 135) {
				dir_idx = 1;	// ��
			}
			else if (a > 135 && a < up_min) {
				dir_idx = 2;	// ��
			}
			else {
				dir_idx = 0;	// ��
			}

			return dir_idx;
		}

		int hist_cnt() const
		{
			return all_points_.size();
		}

		static bool is_no_moving(const TrackingRect &tr)
		{
			return !tr.moving_;
		}

		cv::Point2f start_mean_pt() const
		{
			return mean_of_points(all_points_.back());
		}

		cv::Point2f end_mean_pt() const
		{
			return mean_of_points(all_points_.front());
		}
	};

	std::vector<TrackingRect> trackingRects_;	// ���ڸ��ٵĶ���

public:
	DetectWithOF(KVConfig *cfg);
	virtual ~DetectWithOF(void);

private:
	virtual std::vector<cv::Rect> detect0(size_t st_cnt, cv::Mat &origin, cv::Mat &gray_prev, cv::Mat &gray_curr, cv::vector<int> &dirs);

	// ����ÿ����������Ӿ��� ...
	std::vector<cv::Rect> getBoundingRects(const std::vector<std::vector<cv::Point> > &contours);
};
