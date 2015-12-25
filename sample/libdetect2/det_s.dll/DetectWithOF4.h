#pragma once
#include "detect.h"

/** ʹ��ϡ���������������Ŀ�꣬������ N ��û�нϴ��ƶ�ʱ������Ϊ���θ��ٽ���

 */
class DetectWithOF4 : public Detect
{
	typedef std::vector<cv::Point> CONTOUR;	// ����
	typedef std::vector<cv::Point2f> FEATURES;	// ������

	/** ����һ������������
		
			����Ĵ�����
				����֡������ֽϴ�֡��ʱ����ʴ���ͺ󣬵õ�������Ȼ��Ϊÿ����������һ�� MotionObject����MotionObject��Χ���ҡ������㡱��ʼ���٣�

			����ĸ��٣�
				����ϡ����������٣��ۺ���Ҫ����������㣬ɾ������������㣻��ʱ����������

			��������٣�
				����������Ŀ������ֵ������������ƶ��ٶ�С����ֵ����ΪĿ���Ѿ�ֹͣ�������٣�
	 */
	class MotionObject
	{
		DetectWithOF4 *parent_;
		std::vector<CONTOUR> contours_hist_;	// 
		std::vector<cv::Point2f> mean_pt_hist_;	// �˶���ʷ���������ĵ��λ�ã���
		int max_features_, min_features_;
		FEATURES curr_features_;
		bool moving_;
		cv::Mat prev_;
		double stamp_update_;

	public:
		MotionObject(DetectWithOF4 *parent, const cv::Rect &brc, const cv::Mat &curr_gray) : parent_(parent)
		{
			max_features_ = 300;
			min_features_ = 10;

			CONTOUR region;
			region.push_back(brc.tl());
			region.push_back(cv::Point(brc.x, brc.y+brc.height));
			region.push_back(brc.br());
			region.push_back(cv::Point(brc.x+brc.width, brc.y));

			curr_features_ = update_features(max_features_, curr_features_, region, curr_gray);
			moving_ = curr_features_.size() > min_features_;
			prev_ = curr_gray;

			if (moving_) {
				contours_hist_.push_back(region);
				mean_pt_hist_.push_back(mean_pt(curr_features_));
			}
		}

		bool is_moving() const { return moving_; }

		void track(const cv::Mat &gray)
		{
			FEATURES next_pts;
			cv::Mat status, err;
			cv::calcOpticalFlowPyrLK(prev_, gray, curr_features_, next_pts, status, err);
			cv::Mat s = status.reshape(1, 1), e = status.reshape(1, 1);
			unsigned char *ps = s.ptr<unsigned char>(0), *pe = s.ptr<unsigned char>(0);

			FEATURES features;
			for (size_t i = 0; i < status.cols; i++) {
				if (ps[i] == 1) {
					features.push_back(next_pts[i]);
				}
			}

			if (features.size() < min_features_) {
				moving_ = false;
				return;
			}

			remove_stray(features);	// 
			if (features.size() < min_features_) {
				curr_features_ = update_features(max_features_, features, get_contour(features), gray);
			}
			else {
				curr_features_ = features;
			}

			prev_ = gray;	// 
		}

		cv::Rect get_curr_brc() const
		{
			return cv::boundingRect(curr_features_);
		}

		void draw_hist(cv::Mat &img) const
		{
			if (mean_pt_hist_.size() > 0) {
				cv::circle(img, mean_pt_hist_[0], 2, cv::Scalar(0, 0, 255));
			}

			for (size_t i = 1; i < mean_pt_hist_.size(); i++) {
				cv::line(img, mean_pt_hist_[i-1], mean_pt_hist_[i], cv::Scalar(0, 0, 255));
			}

			for (size_t i = 0; i < curr_features_.size(); i++) {

			}
		}

	private:
		// ���� features ������
		CONTOUR get_contour(const FEATURES &pts)
		{
			CONTOUR tmp, c;
			for (size_t i = 0; i < pts.size(); i++) {
				tmp.push_back(pts[i]);
			}

			cv::convexHull(tmp, c);
			return c;
		}

		// ƽ�����λ��
		cv::Point2f mean_pt(const FEATURES &pts) const
		{
			double sx = 0.0, sy = 0.0;
			for (size_t i = 0; i < pts.size(); i++) {
				sx += pts[i].x, sy += pts[i].y;
			}
			cv::Rect all(0, 0, parent_->video_width_, parent_->video_height_);
			cv::Point2f m = cv::Point2f(sx/pts.size(), sy/pts.size());
			if (m.x < 0) m.x = 0.1;
			if (m.y < 0) m.y = 0.1;
			if (m.x >= parent_->video_width_-0.1) m.x = parent_->video_width_-1;
			if (m.y >= parent_->video_height_-0.1) m.y = parent_->video_height_-1;
			return m;	// ƽ��λ��
		}

		// ��ƽ����ľ���
		double mean_dis(const cv::Point2f &mean_pt, const FEATURES &pts) const
		{
			double sd = 0.0;
			for (size_t i = 0; i < pts.size(); i++) {
				sd += _distance(mean_pt, pts[i]);
			}
			return sd / pts.size();
		}

		// ɾ����Ⱥ��
		void remove_stray(FEATURES &pts)
		{
			if (pts.size() < 3) {
				return;
			}

			/** ���ȼ������е�����ģ��������е㵽���ĵľ��룬ɾ������ƽ������ĵ�
			 */
			cv::Point2f mean = mean_pt(pts);
			double mdis = mean_dis(mean, pts);

			for (int i = pts.size()-1; i >= 0; i--) { // �Ӻ���ǰɾ��
				if (_distance(pts[i], mean) > 2 * mdis) {
					pts.erase(pts.begin() + i);
				}
			}
		}

		// ����
		FEATURES update_features(int max, const FEATURES &old, const CONTOUR &region, const cv::Mat &curr_gray)
		{
			FEATURES features;
			cv::Rect brc = cv::boundingRect(region);
			cv::goodFeaturesToTrack(curr_gray(brc), features, max - old.size(), 0.05, 1.0);

			for (size_t i = 0; i < features.size(); i++) {
				features[i].x += brc.x;
				features[i].y += brc.y;
			}

			// �ϲ� old
			for (size_t i = 0; i < old.size(); i++) {
				if (!has_same_feature_pt(features, old[i])) {
					features.push_back(old[i]);
				}
			}

			return features;
		}

		bool has_same_feature_pt(const FEATURES &fs, const cv::Point2f &pt)
		{
			for (size_t i = 0; i < fs.size(); i++) {
				if (distance(pt, fs[i]) < 1.0) {
					return true;
				}
			}
			return false;
		}

		inline double distance(const cv::Point2f &p0, const cv::Point2f &p1) const
		{
			return sqrt((p0.x-p1.x)*(p0.x-p1.x) + (p0.y-p1.y)*(p0.y-p1.y));
		}
	};
	friend class MotionObject;
	std::vector<MotionObject> motion_objects_;

	/** ����һ��Ŀ�꣬Ŀ����һ�����߶�� MotionObject ���ɣ���һ���ˣ����������ɺ���֫������˶��õ�

			������
				???? 
	 */
	class Target
	{
	public:
		Target()
		{
		}
	};

	double threshold_diff_;	// ȱʡ 25
	cv::Mat ker_erode_, ker_dilate_;

public:
	DetectWithOF4(KVConfig *cfg);
	~DetectWithOF4(void);

private:
	virtual std::vector<cv::Rect> detect0(size_t st_cnt, cv::Mat &origin, cv::Mat &gray_prev, cv::Mat &gray_curr, cv::vector<int> &dirs);
	std::vector<cv::Rect> get_diff_rects(const cv::Mat &prev, const cv::Mat &curr)	// ����֡���ʴ���ͣ�Ȼ��������������������Ӿ���
	{
		cv::Mat diff;
		cv::absdiff(prev, curr, diff);
		cv::threshold(diff, diff, threshold_diff_, 255.0, cv::THRESH_BINARY);

		cv::erode(diff, diff, ker_erode_);
		cv::dilate(diff, diff, ker_dilate_);

		cv::imshow("ed", diff);

		std::vector<CONTOUR> contours;
		cv::findContours(diff, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

		std::vector<cv::Rect> rcs;
		for (size_t i = 0; i < contours.size(); i++) {
			rcs.push_back(cv::boundingRect(contours[i]));
		}

		return rcs;
	}
};
