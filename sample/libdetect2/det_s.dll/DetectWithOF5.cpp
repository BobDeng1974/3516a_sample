#include <assert.h>
#include <set>
#include <list>
#include "DetectWithOF5.h"

typedef std::vector<cv::Point> CONTOUR;
typedef std::vector<CONTOUR> CONTOURS;

DetectWithOF5::DetectWithOF5(KVConfig *kv) : Detect(kv)
{
	hist_.parent = this;
	hist_.N = atoi(cfg_->get_value("of5_history_size", "7"));
	if (hist_.N < 2) {
		hist_.N = 2;
	}

	ker_erode_ = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	ker_dilate_ = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(11, 11));
	ker_open_ = cv::getStructuringElement(cv::MORPH_OPEN, cv::Size(3, 3));

	// ʹ��ֱ�߷��� ..
	int height = atoi(cfg_->get_value("video_height", "720"));
	double far_ratio = atof(cfg_->get_value("of5_far_ratio", "0.18"));
	double slope = (1.0 - far_ratio) / height;
	double intercept = far_ratio;
	factors_y_ = new double[height];
	for (int i = 0; i < height; i++) {
		factors_y_[i] = slope * i + intercept;
	}

	target_x_ = atoi(cfg_->get_value("of5_target_width", "130"));
	target_y_ = atoi(cfg_->get_value("of5_target_height", "170"));

	motion_M_ = hist_.N;
	motion_timeout_ = atof(cfg_->get_value("of5_motion_timeout", "0.3"));
	frame_idx_ = 0;
	threshold_diff_ = (float)atof(cfg_->get_value("of5_diff_threshold", "20"));

	cc_ = new cv::CascadeClassifier;
	if (!cc_->load("data/heads-22-22-haar.xml")) {
		__asm int 3;
		delete cc_;
		cc_ = 0;
	}

	cv::namedWindow("of5");
}

DetectWithOF5::~DetectWithOF5(void)
{
	delete cc_;
}

double DetectWithOF5::factor_y(int y) const
{
	return factors_y_[y];
}

/** frame λ�õ� roi ת�� */
template<class T>
static void f2r(std::vector<T> &pts, const cv::Point &offset)
{
	for (std::vector<T>::iterator it = pts.begin(); it != pts.end(); ++it) {
		it->x += offset.x, it->y += offset.y;
	}
}

void DetectWithOF5::detect(cv::Mat &origin, std::vector<cv::Rect> &targets, int &flipped_index)
{
	origin_ = origin;
	curr_ = now();
	++frame_idx_;

	// ������ʷ
	save_hist(origin);

#if 0
	// FIXME: ������diff��ʷ�У��ҿ��ܵ�Ŀ��λ��
	//	�ڻ���ͼ������ ...
	cv::Mat bin_diff = hist_.rget_diff(0);
	std::vector<cv::Rect> ts = find_diff_clusters(bin_diff);
	for (int i = 0; i < ts.size(); i++) {
		cv::rectangle(origin_, ts[i], cv::Scalar(255, 255, 255), 2);
	}
#endif

	// �����µ�diff�У���������, �ϲ�����..
	CONTOURS regions;
	find_contours(regions);
	cv::drawContours(origin, regions, -1, cv::Scalar(0, 255, 255), 1);

	// �ϲ��������������
	merge_motions(regions);
	draw_motions();

	// �� motion ���и��� ...
	tracking_motions();
	draw_tracking();

	// ���ܹ���
	sum_motions_dense_of();
	draw_motions_dense_of();

	// ����
}

/// ���� pts �ļ�������
static cv::Point center(const std::vector<cv::Point> &pts)
{
	assert(!pts.empty());

	cv::Point pt = pts[0];
	for (size_t i = 1; i < pts.size(); i++) {
		pt.x += pts[i].x;
		pt.y += pts[i].y;
	}

	pt.x /= pts.size();
	pt.y /= pts.size();

	return pt;
}

inline static cv::Point center(const cv::Rect &rc)
{
	std::vector<cv::Point> pts;
	pts.push_back(rc.tl());
	pts.push_back(rc.br());

	return center(pts);
}

std::vector<cv::Point> DetectWithOF5::Motion::get_contours_center()
{
	/** �õ���ʷ�����ļ������� */
	std::vector<cv::Point> centers;
	for (size_t i = 0; i < history.size(); i++) {
		centers.push_back(center(history[i]));
	}

	return centers;
}

std::vector<cv::Point> DetectWithOF5::Motion::get_brcs_center()
{
	/** �õ���ʷ������Ӿ��ε����� */
	std::vector<cv::Point> centers;
	cv::Point last = center(cv::boundingRect(last_contour));
	for (size_t i = 0; i < history.size(); i++) {
		if (history[i].empty()) {
			centers.push_back(last);
		}
		else {
			cv::Rect brc = cv::boundingRect(history[i]);
			last = center(brc);
			centers.push_back(last);
		}
	}

	return centers;
}

void DetectWithOF5::Motion::init_tracking(cv::CascadeClassifier *cc)
{
	if (tracking_inited_)
		return;

	if (cc) {
		/** ����������������λ�ó�ʼ���������ٵ�
		 */
		cv::Mat f = parent->hist_.rget(0);
		cv::Mat roi = f(brc);
		cc->detectMultiScale(roi, faces);
		f2r<cv::Rect>(faces, brc.tl());

		/** FIXME: �������������ô�� ??? */
		if (!faces.empty()) {
			// XXX: ������ʼ����һ���� ...
			cv::Rect face = faces[0];
			roi = f(face);
			std::vector<cv::Point2f> corners;
			cv::goodFeaturesToTrack(roi, corners, 10, 0.15, 3);
			f2r<cv::Point2f>(corners, face.tl());

			/** FIXME: Ϊ�˱�֤ tracking_pts.size() == history.size() */
			for (size_t i = 0; i < history.size(); i++) {
				tracking_pts.push_back(corners);
			}

			tracking_inited_ = true;
		}
	}
	else {
		/** TODO: ���û������ģ��Ļ���������أ�
		 */
		__asm int 3;
	}
}

void DetectWithOF5::Motion::update_bounding_rc()
{
	cv::Rect rc = cv::boundingRect(last_contour);
	size_t k = 0;
	for (k = 0; k < history.size(); k++) {
		if (!history[k].empty()) {
			rc = cv::boundingRect(history[k]);
			break;
		}
	}
	for (; k < history.size(); k++) {
		if (!history[k].empty()) {
			rc |= cv::boundingRect(history[k]);
		}
	}
	brc = rc;
}

static void draw_lines(cv::Mat &img, const std::vector<cv::Point> &pts, const cv::Scalar &color)
{
	assert(!pts.empty());
	cv::Point p = pts[0];
	for (size_t i = 1; i < pts.size(); i++) {
		cv::line(img, p, pts[i], color);
		p = pts[i];
	}
}

template <class T>
static void draw_lines(cv::Mat &img, const T &ps, const T &pt, const cv::Scalar &color)
{
	assert(ps.size() == pt.size());
	for (size_t i = 0; i < ps.size(); i++) {
		if (ps[i].x > 0 && pt[i].x > 0) {
			cv::line(img, ps[i], pt[i], color);
		}
	}
}

bool DetectWithOF5::History::exist(size_t idx)
{
	/** ��� idx ָ����֡�Ƿ���� */
	return idx >= frame_idx_ && idx < frame_idx_ + frames.size();
}

cv::Mat DetectWithOF5::History::get(size_t idx)
{
	assert(exist(idx));
	return frames[idx - frame_idx_];
}

cv::Mat DetectWithOF5::History::get_diff(size_t idx)
{
	assert(exist(idx));
	return diff[idx - frame_idx_];
}

cv::Mat DetectWithOF5::History::rget(size_t id)
{
	assert(id >= 0 && id < frames.size());
	std::deque<cv::Mat>::const_reverse_iterator rit = frames.rbegin();
	return *(rit+id);
}

cv::Mat DetectWithOF5::History::rget_diff(size_t id)
{
	assert(id >= 0 && id < diff.size());
	std::deque<cv::Mat>::const_reverse_iterator rit = diff.rbegin();
	return *(rit+id);
}

void DetectWithOF5::History::push(const cv::Mat &img)
{
	// fifo��ɾ����ʱ�� ..
	if (frames.size() > N) {
		frames.pop_front();
		diff.pop_front();
		frame_idx_++;
	}

	// XXX: ��������� gray
	cv::Mat gray;
	cv::cvtColor(img, gray, CV_RGB2GRAY);
	frames.push_back(gray);

	if (frames.size() == 1) {
		// ��һ֡��diff ȫ�� ..
		diff.push_back(cv::Mat::zeros(img.rows, img.cols, CV_8UC1));
		frame_idx_ = parent->frame_idx_;
	}
	else {
		cv::Mat d;
		
		// ���������֡ frames �� diff
		cv::Mat m = rget(1);	// �����ڶ�֡ ..
		cv::absdiff(m, gray, d);

		cv::threshold(d, d, parent->threshold_diff_, 255, cv::THRESH_BINARY);
	
		// FIXME: ���Ƕ� diff ����open���� ..
		cv::morphologyEx(d, d, cv::MORPH_OPEN, parent->ker_open_);
		diff.push_back(d);
	}

	assert(diff.size() == frames.size());
}

void DetectWithOF5::save_hist(const cv::Mat &img)
{
	hist_.push(img);
}

// Ϊ�˷���ϲ����� ..
struct MergeContour
{
	CONTOUR contour;
	cv::Point2f center;
	float radius;
	double area;	// �������
	bool merged;	// �Ƿ񱻺ϲ�

	static bool op_sort_mc(const MergeContour &p1, const MergeContour &p2)
	{
		return p1.radius > p2.radius;	// �Ӵ�С���� ..
	}
};

// ����������֮��ľ���
static inline float distance(const cv::Point2f &c1, const cv::Point2f &c2)
{
	return sqrt(pow(c1.x-c2.x, 2) + pow(c1.y-c2.y, 2));
}

// ����Բ�ľ������Ƿ��ཻ.
static inline bool is_cross(const cv::Point2f &c1, const float r1,
	const cv::Point2f &c2, const float r2)
{
	return distance(c1, c2) < r1 + r2;
}

static inline bool is_cross(const MergeContour &mc1, const MergeContour &mc2)
{
	return is_cross(mc1.center, mc1.radius, mc2.center, mc2.radius);
}

static CONTOURS merge_contours(CONTOURS &cs)
{
	/** �ϲ��ٽ�������

			1. ����������С���򣺴Ӵ�С
			2. ����ÿ�����������Բ
			3. �Ӵ�С�����μ���Ƿ����Բ�ཻ������ཻ����С��������ϲ������ ...
			4. δ���ϲ���С��������Ϊ����������
			5. �������������С���ܴ󣬲������ľ���С�ڴ��������1.5����Ҳ��ΪС������������� ???
	 */
	CONTOURS merged;
	std::vector<MergeContour> mcs;
	for (CONTOURS::const_iterator it = cs.begin(); it != cs.end(); ++it) {
		MergeContour mc;
		mc.merged = false;
		mc.contour = *it;
		cv::minEnclosingCircle(mc.contour, mc.center, mc.radius);
		mc.area = cv::contourArea(mc.contour);
		mcs.push_back(mc);
	}

	std::sort(mcs.begin(), mcs.end(), MergeContour::op_sort_mc);	// �Ӵ�С����

	for (std::vector<MergeContour>::iterator it = mcs.begin(); it != mcs.end(); ++it) {
		if (!it->merged) {
			for (std::vector<MergeContour>::iterator it2 = it+1; it2 != mcs.end(); ++it2) {
				if (::is_cross(*it, *it2)) {
					// �ཻ��С�ĺϲ�������� ..
					it2->merged = true;
					it->contour.insert(it->contour.end(), it2->contour.begin(), it2->contour.end());
				}

				if (it->radius > 3.0 * it2->radius) { // ˵��������������㹻��
					if (distance(it->center, it2->center) < it->radius * 1.5) {
						it2->merged = true;
						it->contour.insert(it->contour.end(), it2->contour.begin(), it2->contour.end());
					}
				}
			}
		}
	}

	for (std::vector<MergeContour>::const_iterator it = mcs.begin(); it != mcs.end(); ++it) {
		if (!it->merged) {
			CONTOUR hull;
			cv::convexHull(it->contour, hull); // ������͹���ͣ�Ӧ���ܹ����ٺܶ��ڲ��ĵ��Ӱ� ..
			merged.push_back(hull);
		}
	}

	return merged;
}

/* �ж� contour ����Ӧ��λ�ã��Ƿ�̫С ? */
class op_contour_is_toooo_small
{
	double area_threshold_, *factor_y_;

public:
	op_contour_is_toooo_small(double area_threshold, double factor_y[])
		: area_threshold_(area_threshold)
		, factor_y_(factor_y)
	{
	}

	bool operator()(const CONTOUR &contour) const
	{
		cv::Rect brc = cv::boundingRect(contour);
		double fy = factor_y_[CENTER_Y(brc)];
		return cv::contourArea(contour) < fy*fy*area_threshold_*0.1;	// 
	}
};

void DetectWithOF5::merge_contours(CONTOURS &contours)
{
	contours = ::merge_contours(contours);

	//// DEBUG: �����������������ۡ��ˡ��Ĵ�С ...
	//for (int i = 0; i < contours.size(); i++) {
	//	cv::Rect brc = cv::boundingRect(contours[i]);
	//	cv::Size size = est_target_size(CENTER_Y(brc));
	//	cv::Rect estrc(CENTER_X(brc)-size.width/2, CENTER_Y(brc)-size.height/2, size.width, size.height);

	//	cv::rectangle(origin_, brc, cv::Scalar(0, 0, 0), 2);
	//	cv::rectangle(origin_, estrc, cv::Scalar(255, 255, 255));
	//}

	// TODO: ɾ�����̫С�� ..Ӧ�ø���y����ֵ ...
	op_contour_is_toooo_small ts(target_x_ * target_y_, factors_y_);
	//op_contour_is_toooo_small ts(500, factors_y_);
	contours.erase(std::remove_if(contours.begin(), contours.end(), ts), contours.end());
}

/** �� hist_ �����һ֡ diff �в������������ϲ��ٽ������� */
void DetectWithOF5::find_contours(CONTOURS &regions)
{
	cv::Mat diff_sum = hist_.rget_diff(0);

	CONTOURS contours;
	cv::findContours(diff_sum, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

	merge_contours(contours);
	regions = contours;
}

void DetectWithOF5::remove_timeout_motions()
{
	/** ɾ������300ms û�и��µ� Motion
	 */
	for (MOTIONS::iterator it = motions_.begin(); it != motions_.end(); ) {
		if (curr_ - it->stamp > motion_timeout_) {
			it = motions_.erase(it);
		}
		else {
			++it;
		}
	}
}

/** �����µ������ϲ������еġ�������С������û���ص������½��������
 */
void DetectWithOF5::merge_motions(const CONTOURS &regions)
{
	remove_timeout_motions();

	/** ��� regions �Ƿ�����ĳ�� motions ...
	 */
	std::set<size_t> merged_idxs;	// regions �У����ϲ���idx
	std::vector<size_t> exp_idxs;
	for (size_t i = 0; i < motions_.size(); i++) {
		/** ���ÿ�� motion���ϲ����������λ���ཻ������ */
		Motion &m = motions_[i];
		exp_idxs.clear();

		// motion ����������� M ֡
		if (m.history.size() > m.M) {
			m.history.pop_front();	// FIXME: �п��� history ��ʣ�µĶ��ǡ��ա������ˣ���һ������²���� ...
			if (m.tracking_inited_) {
				m.tracking_pts.pop_front();	// FIXME: ��֤ size() ...
			}

			m.dense_of_poss.pop_front();
			m.dense_of_xs.pop_front();
			m.dense_of_ys.pop_front();

			m.frame_idx_ ++;
		}
		
		assert(!m.tracking_inited_ || m.history.size() == m.tracking_pts.size());

		// ���һ����Ч���������Բ ..
		cv::Point2f c0;
		float r0;
		cv::minEnclosingCircle(m.last_contour, c0, r0);	// ������������Բ

		for (size_t j = 0; j < regions.size(); j++) {
			cv::Point2f c;
			float r;
			cv::minEnclosingCircle(regions[j], c, r);
			if (::is_cross(c0, r0, c, r)) {
				exp_idxs.push_back(j);
				merged_idxs.insert(j);
			}
		}

		/** FIXME: �����Ҫ��չ�����������������������������뻹�Ƚ�Զ����κϲ���
				���ܸ��õķ����ǣ�������ʷ�˶��ķ���ѡ������ĳ���������� ....
		 */

		bool update = false;
		if (exp_idxs.size() == 1) {
			// ���汻�ϲ������� ..
			m.last_contour = regions[exp_idxs[0]];
			motions_[i].history.push_back(m.last_contour);
			motions_[i].stamp = curr_;
			update = true;
		}
		else if (exp_idxs.size() == 0) {
			// ��ʱ˵�� motion û�л ???
			motions_[i].history.push_back(CONTOUR());	// 
		}
		else {
			// TODO: Ӧ�ø�����ʷ�켣����ѡ����п��ܵ�һ�� ...
			m.last_contour = regions[exp_idxs[0]];
			motions_[i].history.push_back(m.last_contour);
			motions_[i].stamp = curr_;
			update = true;
		}

		if (update) {
			// ���¼��������� ...
			m.update_bounding_rc();
		}

		if (m.dense_of_poss.size() > 0) {
			// ���³��ܹ���
			cv::Mat x, y;
			if (calc_dense_of(m.brc, x, y)) {
				m.dense_of_poss.push_back(m.brc);
				m.dense_of_xs.push_back(x);
				m.dense_of_ys.push_back(y);
			}
		}

		if (!m.tracking_inited_) {
			m.init_tracking(cc_);
		}
	}

	// �ϲ��ص��� motions
	merge_overlapped_motions();

	// �����µ� motions
	for (size_t i = 0; i < regions.size(); i++) {
		if (merged_idxs.find(i) == merged_idxs.end()) { // ����δ���ϲ���������Ӧ�ô���Ϊ�µ� motions ??
			const CONTOUR &c = regions[i];

			/** ʹ�ò�ͬ��ɫ��ʶmotion��������� ...
			 */
			const cv::Scalar _colors[] = { cv::Scalar(0, 128, 0), cv::Scalar(0, 0, 128), cv::Scalar(0, 128, 128), cv::Scalar(0, 255, 128), cv::Scalar(0, 255, 0),
				cv::Scalar(128, 255, 0), cv::Scalar(128, 128, 0), cv::Scalar(255, 0, 0) };
			const size_t _colors_cnt = sizeof(_colors) / sizeof(cv::Scalar);
			static int _mid = 0;

			Motion m;
			m.parent = this;
			m.discard_ = false;
			m.id = _mid++;
			m.M = motion_M_;
			m.history.push_back(c);
			m.last_contour = c;
			m.brc = cv::boundingRect(c);
			m.stamp = curr_;
			m.frame_idx_ = hist_.frame_idx_ + hist_.frames.size() - 1;	// FIXME: ��Ӧ����ʷ�����һ֡ͼ�� ...
			m.color = _colors[m.id % _colors_cnt];
			m.tracking_inited_ = false;
			m.init_tracking(cc_);

			// ���ó��ܹ��� ...
			cv::Mat x, y;
			if (calc_dense_of(m.brc, x, y)) {
				m.dense_of_poss.push_back(m.brc);
				m.dense_of_xs.push_back(x);
				m.dense_of_ys.push_back(y);
			}

			motions_.push_back(m);
		}
	}
}

void DetectWithOF5::draw_motions()
{
	/** ʹ�ò�ͬ��ɫ��ʾ���������� */

	for (size_t i = 0; i < motions_.size(); i++) {
		cv::rectangle(origin_, motions_[i].brc, motions_[i].color, 2);
		char buf[16];
		snprintf(buf, sizeof(buf), "#%d", motions_[i].id);
		cv::putText(origin_, buf, motions_[i].brc.tl(), cv::FONT_HERSHEY_PLAIN, 1.5, motions_[i].color, 2);

		// ��ʷ���������� ..
		//std::vector<cv::Point> centers = motions_[i].get_contours_center();
		//std::vector<cv::Point> centers = motions_[i].get_brcs_center();
		//draw_lines(origin_, centers, motions_[i].color);

		// FIXME: ����Ŀ���С ...
		cv::Size ts = est_target_size(CENTER_Y(motions_[i].brc));
		cv::Rect estrc, brc = cv::boundingRect(motions_[i].last_contour);

		estrc.x = CENTER_X(motions_[i].brc) - ts.width / 2;
		estrc.y = CENTER_Y(motions_[i].brc) - ts.height / 2;
		estrc.width = ts.width;
		estrc.height = ts.height;
		cv::rectangle(origin_, estrc, cv::Scalar(255, 255, 255));
	}
}

/** ͨ��ϡ��������� motions ...
 */
void DetectWithOF5::Motion::track()
{
	if (!tracking_inited_)
		return;

	// ��Ҫ�õ������֡��ʷ
	if (parent->hist_.frames.size() < 2) {
		return;
	}

	/** 
	 */
	cv::Mat error;
	std::vector<unsigned char> status;
	std::vector<cv::Point2f> &prev_pts = tracking_pts.back(), next_pts;
	cv::Mat prev_img = parent->hist_.rget(1), next_img = parent->hist_.rget(0);
	cv::calcOpticalFlowPyrLK(prev_img, next_img, prev_pts, next_pts, status, error);
	
	for (size_t i = 0; i < status.size(); i++) {
		if (status[i] != 1) {
			next_pts[i].x = -1, next_pts[i].y = -1;
		}
	}

	tracking_pts.push_back(next_pts);
}

void DetectWithOF5::tracking_motions()
{
	for (size_t i = 0; i < motions_.size(); i++) {
		motions_[i].track();
	}
}

void DetectWithOF5::draw_motion_tracking(DetectWithOF5::Motion &m)
{
	if (m.tracking_pts.empty()) 
		return;

	std::vector<cv::Point2f> l0 = m.tracking_pts[0];
	for (size_t i = 1; i < m.tracking_pts.size(); i++) {
		std::vector<cv::Point2f> l = m.tracking_pts[i];
		draw_lines<std::vector<cv::Point2f> >(origin_, l0, l, m.color);
		l0 = l;
	}

	// �����ٵ�ľۺ����� ..
	cv::vector<cv::Point> last_cluster;
	std::vector<cv::Point2f> &l1 = m.tracking_pts.back();
	for (size_t i = 0; i < l1.size(); i++) {
		if (l1[i].x < 0) continue;
		last_cluster.push_back(l1[i]);
	}

	if (last_cluster.size() > 3) {
		cv::rectangle(origin_, cv::boundingRect(last_cluster), cv::Scalar(0, 0, 255), 3);
	}
}

/** �����ٵ�Ĺ켣 */
void DetectWithOF5::draw_tracking()
{
	for (size_t i = 0; i < motions_.size(); i++) {
		Motion &m = motions_[i];
		draw_motion_tracking(m);
	}
}

bool DetectWithOF5::Motion::last_hist(CONTOUR &contour, int rid)
{
	int n = -1;
	std::deque<CONTOUR>::const_reverse_iterator rit;
	for (rit = history.rbegin(); rit != history.rend(); ++rit) {
		if (rit->empty()) continue;
		n++;
		if (n == rid) {
			contour = *rit;
			return true;
		}
	}

	return false;
}

bool DetectWithOF5::Motion::has_same_last_hist(const DetectWithOF5::Motion &m)
{
	// �� history ������ǰ�ң����������һ���ģ��򷵻���ͬ ..
	CONTOUR s0, m0, s1, m1;
	if (last_hist(m0, 0) && last_hist(m1, 1) && last_hist(s0, 0) && last_hist(s1, 1)) {
		if (m0.size() == s0.size() && m1.size() == s1.size()) {
			for (size_t i = 0; i < m0.size(); i++) {
				if (m0[i].x != s0[i].x || m0[i].y != s0[i].y) {
					return false;
				}
			}

			for (size_t i = 0; i < m1.size(); i++) {
				if (m1[i].x != s1[i].x || m1[i].y != m1[i].y) {
					return false;
				}
			}

			return true;
		}
		else {
			return false;
		}
	}
	else {
		return false;
	}
}

bool DetectWithOF5::op_larger_motion(const DetectWithOF5::Motion &m0, const DetectWithOF5::Motion &m1)
{
	return m0.brc.area() > m1.brc.area();
}

/** �ϲ��ص��� motions

		��Ϊmotion �� contour ����ɣ��п�����һ��Ŀ��һ��ʼ�γ���������� contours�����������ϲ�
		�п��ܳ������������ص��� motions����ʱ��Ҫ�ӵ�һ�� ...

	�ж��㷨��
		���� brc �Ӵ�С���� 
		���αȽ� motions.history ����������Ƿ���ͬ�������ͬ�����ӵ�С�ģ��������������Ǹ� ...
 */
void DetectWithOF5::merge_overlapped_motions()
{
	std::sort(motions_.begin(), motions_.end(), op_larger_motion);
	for (size_t i = 0; i < motions_.size(); i++) {
		if (!motions_[i].discard_)  {
			for (size_t j = i+1; j < motions_.size(); j++) {
				if (motions_[i].has_same_last_hist(motions_[j])) {
					motions_[j].discard_ = true;
				}
			}
		}
	}

	// ɾ�� discarded ..
	for (std::vector<Motion>::iterator it = motions_.begin(); it != motions_.end(); ) {
		if (it->discard_)
			it = motions_.erase(it);
		else
			++it;
	}
}

cv::Size DetectWithOF5::est_target_size(int y)
{
	return cv::Size((int)(target_x_ * factors_y_[y]), (int)(target_y_ * factors_y_[y]));
}

#if 0
std::vector<cv::Rect> DetectWithOF5::find_diff_clusters(const cv::Mat &diff)
{
	/** FIXME: diff Ϊ��ֵ��Ķ�ֵͼ���ȼ������ͼ��Ȼ���������ʹ�û��������������������ڵĴ�С���ݹ����Ŀ���С���ã�
		������������е� 1 ��������ڴ��ڵ�һ�룬����Ϊ�ô���λ����һ��Ŀ�� ...
	 */

	std::vector<cv::Rect> rcs;

	cv::Mat integraled;
	cv::integral(diff, integraled);

	cv::imshow("diff", diff);
	//cv::imshow("integral", integraled);

#define Y_STEP 10
#define X_STEP 10

	bool yoverflow = false;
	// û��Ҫ����Ͽ�ʼ ...
	for (int y = 10; y < video_height_ && !yoverflow; y += Y_STEP) {
		cv::Size ts = est_target_size(y);
		int height = ts.height;
		if (y + height > video_height_) {
			yoverflow = true;
			height = video_height_ - y;
		}

		int width = ts.width;

		for (int x = 0; x < video_width_; x += X_STEP) {
			if (x + width > video_width_) width = video_width_ - x;

			int p0 = integraled.at<int>(cv::Point(x, y));
			int p1 = integraled.at<int>(cv::Point(x+width, y));
			int p2 = integraled.at<int>(cv::Point(x, y+height));
			int p3 = integraled.at<int>(cv::Point(x+width, y+width));

			int sum = p3 + p0 - p1 - p2;

			// ��Ϊ"����"��1������ sum / area ��������ı��� ..
			cv::Rect rc(x, y, width, height);
			double ratio = sum / 255.0 / rc.area();

			if (ratio > 0.3) { // FIXME: ???
				rcs.push_back(rc);
			}
		}
	}

	return rcs;
}
#endif //

bool DetectWithOF5::calc_dense_of(const cv::Rect &roi, cv::Mat &x, cv::Mat &y)
{
	if (hist_.frames.size() < 2)
		return false;

	cv::Mat prev = hist_.rget(1)(roi), next = hist_.rget(0)(roi), flow;
	cv::calcOpticalFlowFarneback(prev, next, flow, 0.5, 2, 13, 1, 1.5, 1.1, 0);
	std::vector<cv::Mat> xy;
	cv::split(flow, xy);
	x = xy[0], y = xy[1];

	return true;
}

// ���� s �Ƿ��� l �ڲ�
inline bool is_in(const cv::Rect &l, const cv::Rect &s)
{
	return (l.x <= s.x && l.x+l.width >= s.x+s.width && l.y <= s.y && l.y+l.height >= s.y+s.height);
}

void DetectWithOF5::Motion::hlp_make_of_brc(size_t idx, cv::Mat &x, cv::Mat &y)
{
	assert(idx >= 0 && idx < dense_of_poss.size());
	assert(dense_of_poss.size() == desnse_of_xs.size());
	assert(dense_of_poss.size() == desnse_of_ys.size());

	x = cv::Mat::zeros(cv::Size(brc.width, brc.height), CV_32F);
	y = cv::Mat::zeros(cv::Size(brc.width, brc.height), CV_32F);

	cv::Rect rc = dense_of_poss[idx];
	if (!is_in(brc, rc)) {
		rc &= brc;

		if (rc.width == 0 || rc.height == 0) {
			__asm int 3;
		}
	}

	rc.x -= brc.x;
	rc.y -= brc.y;

	cv::Mat t = x(rc);
	dense_of_xs[idx].copyTo(t);

	t = y(rc);
	dense_of_ys[idx].copyTo(t);
}

void DetectWithOF5::Motion::merge_dense_ofs(cv::Mat &xx, cv::Mat &yy)
{
	xx = cv::Mat::zeros(cv::Size(brc.width, brc.height), CV_32F);
	yy = cv::Mat::zeros(cv::Size(brc.width, brc.height), CV_32F);

	for (size_t i = 0; i < dense_of_poss.size(); i++) {
		cv::Mat x, y;
		hlp_make_of_brc(i, x, y);
		xx += x;
		yy += y;
	}
}

void DetectWithOF5::sum_motions_dense_of()
{
	for (size_t i = 0; i < motions_.size(); i++) {
		Motion &m = motions_[i];

		cv::Mat xx, yy;	// �ϲ���Ĺ���ʸ��
		m.merge_dense_ofs(xx, yy);

		// xx, yy Ϊ����ʸ���ͣ�ת��Ϊ�����꣬�õ����򣬸��ݷ���ȡ���ĸ�����Ĺ������� ...
		cv::cartToPolar(xx, yy, m.dense_sum_dis, m.dense_sum_dir, true);
	}
}

void DetectWithOF5::rgb_from_dis_ang(const cv::Mat &distance, const cv::Mat &angles, cv::Mat &rgb)
{
	cv::Mat hsv;
	cv::Mat tmp_dis;
	cv::Mat tmp_ang = angles;

	double minVal, maxVal;
	cv::minMaxLoc(distance, &minVal, &maxVal);
	distance.convertTo(tmp_dis, -1, 1 / maxVal);

	cv::minMaxLoc(tmp_dis, &minVal, &maxVal);

	cv::Mat chs[3];
	chs[0] = tmp_ang;
	chs[1] = cv::Mat::ones(tmp_ang.size(), CV_32F);
	chs[2] = tmp_dis;
	cv::merge(chs, 3, hsv);

	cv::cvtColor(hsv, rgb, cv::COLOR_HSV2BGR);	// rgb�� 32FC3
}

void DetectWithOF5::draw_motions_dense_of()
{
	cv::Mat vis = cv::Mat::zeros(cv::Size(origin_.cols, origin_.rows), CV_32FC3); // rgb ��ʽΪ 32FC3

	for (size_t i = 0; i < motions_.size(); i++) {
		Motion &m = motions_[i];

		cv::Mat rgb;
		if (m.dense_sum_dis.cols > 0) {
			cv::Mat ndis, ndir;
			normalize_dis(m.dense_sum_dis, ndis);
			normalize_dir(m.dense_sum_dir, ndir);
			rgb_from_dis_ang(ndis, ndir, rgb);

			cv::Mat r = vis(m.brc);
			rgb.copyTo(r);
		}
	}

	cv::imshow("dense of", vis);
}

enum {
	DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN,
//   ��ɫ		��ɫ		  ��ɫ		��ɫ
};

const float _dirs0[] = { 180, 0, 270, 90 };

inline int _dir(float i)
{
	int up_angle_ = 110;	// FIXME: Ӧ�ö�ȡ�����ļ� ...
	int up_half = up_angle_ / 2;
	int up_min = 270 - up_half, up_max = 270 + up_half;

	if (i >= up_min && i <= up_max) {
		return DIR_UP;
	}
	else if (i >= 45 && i <= 135) {
		return DIR_DOWN;
	}
	else if (i > 135 && i < up_min) {
		return DIR_LEFT;
	}
	else {
		return DIR_RIGHT;
	}
}

/** �Ƕȱ�׼����������ɫ�ܵ������ ...
 */
void DetectWithOF5::normalize_dir(const cv::Mat &dir, cv::Mat &m)
{
	m = cv::Mat::zeros(dir.rows, dir.cols, CV_32F);

	for (int y = 0; y < dir.rows; y++) {
		const float *ps = dir.ptr<float>(y);
		float *pt = m.ptr<float>(y);

		for (int x = 0; x < dir.cols; x++) {
			pt[x] = _dirs0[_dir(ps[x])];
		}
	}
}

/** �Ծ���Ӧ����ֵ������������Ӧ�ø����� ...
 */
void DetectWithOF5::normalize_dis(const cv::Mat &dis, cv::Mat &m)
{
	// FIXME: ��ֵӦ�ø���ǰ��ʹ�ò�ͬ�� ...
	double minVal, maxVal;
	cv::minMaxLoc(dis, &minVal, &maxVal);
	dis.convertTo(m, -1, 1/maxVal);

	float threshold = 0.3;

	cv::threshold(m, m, threshold, 1.0, cv::THRESH_BINARY);
}
