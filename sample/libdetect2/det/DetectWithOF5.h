#pragma once
#include "detect.h"

/** ʵ�����̷��ģʽ

		0. ��Ҫ������򡱵ĸ��ÿ��������򡱱���N֡���ʷ�������ͨ��֡����ۺϵõ���
		1. ÿ���������N֡��ʷ
		2. ���������еĳ��ܹ������ҳ������ϡ��Ļ;
		3. �ڡ����ϡ��Ļ�������ҡ��������Ļ ..
 */

class DetectWithOF5: public Detect
{
	/** ����� */
	struct Motion
	{
		DetectWithOF5 *parent;
		size_t M;	// ��ౣ��M����ʷ���� ...
		int id;
		double stamp;	// ������ʱ���.
		size_t frame_idx_;		// ��һ����ʷ��Ӧ��֡��ţ����ڴ� hist_ �в��Ҷ�Ӧ����ʷͼ�� ..
		std::deque<std::vector<cv::Point> > history;	// ��ʷ����
		std::vector<cv::Point> last_contour;	// �����Ч������
		cv::Rect brc;	// ������ʷ����Ӿ��Σ�
		bool tracking_inited_;	// �Ƿ��Ѿ���ʼ���� ..
		bool discard_;	// ���� merge_overlapped_motions() ʹ�ã����Ϊ��Ҫɾ�� ...

		cv::Scalar color;	// debug

		std::deque<std::vector<cv::Point2f> > tracking_pts;	// ���ٵ������㣬tracking_pts.size() == history.size()
		std::vector<cv::Rect> faces;	// FIXME: ��������������ô�죿��

		// ������ʷ���ܹ���
		std::deque<cv::Rect> dense_of_poss;	// ���������λ�ã�
		std::deque<cv::Mat> dense_of_xs;	// ˮƽ����ֱ����Ĺ��� ...
		std::deque<cv::Mat> dense_of_ys;	// 
		cv::Mat dense_sum_dis;
		cv::Mat dense_sum_dir;	// �ۼƺ�Ĺ���������ģʽ ...

		std::vector<cv::Point> get_contours_center(); // ������ʷ���������ģ�һ���̶��ϣ��ܷ�Ӧmotion���ƶ� ...
		std::vector<cv::Point> get_brcs_center();		// ������ʷ���������ε����ģ�����˵�� get_contours_center() ���ȶ���
		void update_bounding_rc();
		void init_tracking(cv::CascadeClassifier *cc); // ��ʼ������ ...
		void track();		// ���������� ...
		bool has_same_last_hist(const Motion &m);	// �� m �Ƚϣ��Ƿ����������ʷһ�� ??
		bool last_hist(std::vector<cv::Point> &cont, int rid); // ���� rid ����Ч��ʷ������rid=0, 1, 2 ...
		void merge_dense_ofs(cv::Mat &xx, cv::Mat &yy);	// �ϲ���ʷ��������СΪ brc
		
	private:
		void hlp_make_of_brc(size_t idx, cv::Mat &x, cv::Mat &y);	// idx Ϊ dense_of_xxx ����ţ����� brc ��С�ģ����ⲿ��Ϊ 0
	};

	friend struct Motion;
	typedef std::vector<Motion> MOTIONS;
	MOTIONS motions_;	// ����� ..

	struct History
	{
		std::deque<cv::Mat> frames;	// ��ʷͼ�� 
		std::deque<cv::Mat> diff;	// ��ʷ֡�� 
		size_t frame_idx_;			// frames.front() ��֡��� ...
		size_t N;					// ϣ����������ʷ ..
		DetectWithOF5 *parent;

		void push(const cv::Mat &img);
		bool exist(size_t idx);
		cv::Mat get(size_t idx);  // assert(exist(idx))
		cv::Mat rget(size_t rid); // �Ӻ���ǰ�� ... rid=0 ���һ֡��1 �����ڶ�֡ ...
								  // assert(rid < frames.size())

		cv::Mat get_diff(size_t idx);
		cv::Mat rget_diff(size_t rid);
	};
	friend struct History;
	History hist_;

	cv::Mat ker_erode_, ker_dilate_, ker_open_;
	cv::Mat origin_;
	double curr_;	// ��ǰʱ�� ..
	size_t frame_idx_;	// ��ǰ֡��� ..
	float threshold_diff_;	// ֡����ֵ��Ĭ�� 30

	double *factors_y_;	// ͼ���У����ϵ��µ�ϵ���������棨������Ϊ1.0�������棨Զ��Ϊ��������ֵ����0.1��ʹ�����Ա任 ..

	int motion_M_;	// ÿ�����ౣ����ʷ��Ŀ��ȱʡ = hist_.N
	double motion_timeout_;	// �����ʱʱ�䣬Ĭ�� 300ms

	cv::CascadeClassifier *cc_;

	int target_x_, target_y_; // Ŀ������� ...

public:
	DetectWithOF5(KVConfig *kv);
	~DetectWithOF5(void);

private:
	virtual void detect(cv::Mat &origin, std::vector<cv::Rect> &targets, int &flipped_index);

	/** ������ʷ����ʷ����ԭʼͼ�񣬺�֡��ĻҶȽ�� */
	void save_hist(const cv::Mat &origin);

	/** ������֡���ҿ��ܵ�Ŀ�� */
	void find_contours(std::vector<std::vector<cv::Point> > &regions);

	// �Ƚ�motion��С
	static bool op_larger_motion(const DetectWithOF5::Motion &m0, const DetectWithOF5::Motion &m1);

	/** �ϲ� find_motions() �õ��� motions */
	void merge_motions(const std::vector<std::vector<cv::Point> > &regions);

	/** �ϲ��ص��� motions
	 */
	void merge_overlapped_motions();

	/** ���� motions */
	void tracking_motions();
	void draw_tracking();
	void draw_motion_tracking(Motion &m);

	/** �ϲ��ӽ������� */
	void merge_contours(std::vector<std::vector<cv::Point> > &contours);

	/** draw motions */
	void draw_motions();

	/** ɾ����ʱ�� motions */
	void remove_timeout_motions();

	/// ����Ӵ�С����
	static inline bool op_sort_motion_by_area(const Motion &m1, const Motion &m2)
	{
		return m1.brc.area() > m2.brc.area();
	}

	/** ���� y ��λ�ã��������ϵ�� */
	double factor_y(int y) const;

	/// ���� factor_y_ ����Ŀ���С
	cv::Size est_target_size(int y);

	/** �� m �������ڽ��ľ��࣬ʹ�ù��Ƶ�Ŀ���Сģ������ ...
			diff Ҫ���� 0 �� 1 �Ķ�ֵͼ
	 */
	std::vector<cv::Rect> find_diff_clusters(const cv::Mat &bin_diff);

	/** Ϊ�˵��Է��㣬��ʾ��ͬλ�ã�Ŀ��Ĵ�С */
	void show_targets_size();

	/** ���������ʷ֮��ĳ��ܹ��������� ..*/
	bool calc_dense_of(const cv::Rect &roi, cv::Mat &x, cv::Mat &y);

	/** ͳ�Ƴ��ܹ������ۼƺ� */
	void sum_motions_dense_of();
	void draw_motions_dense_of();

	/** ���� dis, ang ��Ӧ�� HSV��ת��Ϊ rgb ͼ 32FC3��������ʾ */
	void rgb_from_dis_ang(const cv::Mat &dis, const cv::Mat &ang, cv::Mat &rgb);

	/** ���� 32FC1 �� dir���ĸ�����ѡ�����м��ֵ������˵���ң�0���� 270���� 90���� 180 */
	void normalize_dir(const cv::Mat &dir, cv::Mat &n);

	/** ���� 32FC1 �� dis��ʹ����ֵ ... */
	void normalize_dis(const cv::Mat &dis, cv::Mat &n);
};
