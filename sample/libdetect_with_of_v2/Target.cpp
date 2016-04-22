#include "Target.h"
#include "cluster.h"

Target::Target()
{
}

Target::~Target()
{
}

bool Target::init(KVConfig *cfg, int id, const cv::Rect &roi, const cv::Mat &curr_gray,
    double stamp, double min_dis_5frames, int min_pts, int max_feature_pts, double qualitylevel)
{
    first_rc_ = roi;
    outer_.x = 0, outer_.y = 0, outer_.width = curr_gray.cols, outer_.height = curr_gray.rows;
    stamp_ = stamp;
    cfg_ = cfg;
    id_ = id;
    min_dis_5frames_ = min_dis_5frames;
    min_pts_ = min_pts;

    stopped_dis_ = atof(cfg->get_value("pd_target_stopped_dis", "2.0"));

    //int max_features_pts = atoi(cfg->get_value("pd_max_features_pts", "300"));
    int max_features_pts = 200;

    PTS pts;
    //cv::goodFeaturesToTrack(curr_gray(roi), pts, max_feature_pts, qualitylevel, 1.5);
    hi_goodFeaturesToTrack(curr_gray(roi), pts, 200, qualitylevel, 1.5);

    if ((int)pts.size() < min_pts_) {
        return false;
    }

    l2g(pts, roi.tl());

    layers_.push_back(pts);

    brc_ = roi;
    last_rc_ = cv::boundingRect(pts);

    return true;
}

bool Target::track(const cv::Mat &prev, const cv::Mat &curr, double stamp)
{
    // TODO: Ӧ�ø��ݹ켣�ķ�����չ������Χ ...
    // FIXME: �򵥵�������չ ...
    // ��һ�εõ��������㣬���в��ֲ��ڻĿ���ϣ�����Ӧ����N֮֡���ӵ���Щ�㣬�á����ٵ㡱��������Ŀ���� ...

    int exp = 60;   // ��֪����������Ƿ���� ...
    cv::Rect search_roi = last_rc_;
    search_roi.x -= exp;
    search_roi.y -= exp;
    search_roi.width += 2 * exp;
    search_roi.height += 2 * exp;
    search_roi &= outer_;

    PTS last_pts = layers_.back(), curr_pts, last_pts2;
    g2l(last_pts, search_roi.tl());

    //cv::Mat status, err, status2, err2;
    std::vector<unsigned char> status, err, status2, err2;
    //cv::calcOpticalFlowPyrLK(prev(search_roi), curr(search_roi), last_pts, curr_pts, status, err);
    calcLKOpticalFlow(prev(search_roi), curr(search_roi), last_pts, curr_pts, status);

//#define REVERSE_FIND
#ifdef REVERSE_FIND
    //cv::calcOpticalFlowPyrLK(curr(search_roi), prev(search_roi), curr_pts, last_pts2, status2, err2);   // ������� ..
    calcLKOpticalFlow(curr(search_roi), prev(search_roi), curr_pts, last_pts2, status2);   // ������� ..
#endif //

    //for (int r = 0; r < status.rows; r++) {
    //    // ����Ҵ��
    //    if (status.at<uchar>(r, 0) != 1) {
    //        curr_pts[r].x = -10000;
    //    }
    //}

    for (int r = 0; r < status.size(); r++) {
        // ����Ҵ��
        if (status[r] != 1) {
            curr_pts[r].x = -10000;
        }
    }

#ifdef REVERSE_FIND
    /// �Ƚ� last_pts �� last_pts2������ȽϽӽ���˵��������ҳɹ� ...
    for (int i = 0; i < (int)last_pts.size(); i++) {
        if (::distance(last_pts[i], last_pts2[i]) > 5.0) {
            curr_pts[i].x = -10000;
        }
    }
#endif

    /// ɾ��������Ӧ�Ĺ켣 ...
    PTS valid_pts;
    for (int i = (int)curr_pts.size() - 1; i >= 0; i--) {
        if (curr_pts[i].x < -5000) {
            remove_path(i);
        }
        else {
            valid_pts.push_back(curr_pts[i]);
        }
    }

    if ((int)valid_pts.size() < min_pts_) {
        return false;
    }

    std::reverse(valid_pts.begin(), valid_pts.end());   // ��Ҫ���� ...

    l2g(valid_pts, search_roi.tl());

    layers_.push_back(valid_pts);
    last_rc_ = cv::boundingRect(valid_pts);
    brc_ |= last_rc_;

    return check_paths(stamp);
}

bool Target::is_crossed(const cv::Rect &rc) const
{
    return (rc & brc_).area() > 10;
}

bool Target::is_stopped() const
{
    /** ���path�����5�������ƽ��ֵС�� 2.0������ΪĿ��ֹͣ��� ...
     */
    if (layers_.size() <= 5 || (int)layers_[0].size() < min_pts_) {
        return false;
    }

    std::vector<PATH> paths = get_sorted_paths((int)layers_.size() - 6, (int)layers_.size());

    // ͳ�ƾ������5��·�������5֡�ľ��� ...
    double sum = 0.0;
    for (size_t i = 0; i < 5; i++) {
        sum += distance(paths[i]);
    }
    sum /= 5.0 * 5.0;

    return sum < stopped_dis_;
}

static bool op_more_pts(const std::vector<cv::Point2f> &ps1, const std::vector<cv::Point2f> &ps2)
{
    return ps1.size() > ps2.size();
}

static void draw_pts(cv::Mat &rgb, const std::vector<cv::Point2f> &pts, cv::Scalar &color)
{
    for (size_t i = 0; i < pts.size(); i++) {
        cv::circle(rgb, pts[i], 1, color);
    }
}

void Target::debug_draw_paths(cv::Mat &rgb, cv::Scalar &color, int n) const
{
    cv::Scalar path_color = color;
    for (int i = 0; i < (int)layers_[0].size() && i < n; i++) {
        PATH path = get_path(i);
        if (distance(path) > 5.0) {
            debug_draw_path(rgb, path, path_color);
        }
    }

    // ����һ�����ӣ������һ������
    if (layers_.size() >= 2) {
        cv::rectangle(rgb, cv::boundingRect(layers_.front()), cv::Scalar(0, 0, 127));
        cv::rectangle(rgb, cv::boundingRect(layers_.back()), cv::Scalar(0, 128, 0));

        // ���һ��㣬����Ƿ���ɢ ...
        std::vector<bool> status;
        const PTS &back = layers_.back();
#if 0
        if (check_alone_pts(back, status)) {
            for (size_t i = 0; i < back.size(); i++) {
                if (status[i]) {
                    cv::circle(rgb, back[i], 1, cv::Scalar(255, 32, 0));
                }
                else {
                    cv::circle(rgb, back[i], 1, cv::Scalar(0, 200, 200));
                }
            }
        }
#else
        std::vector<std::vector<cv::Point2f> > clusters;
        cluster_builder cb(40);
        cb.calc(back, clusters);
        std::sort(clusters.begin(), clusters.end(), op_more_pts);

        /// ������ǰ���� ...
        cv::Scalar colors[3] = { cv::Scalar(0, 200, 200), cv::Scalar(255, 64, 0), cv::Scalar(0, 255, 0) };
        for (int i = 0; i < (int)clusters.size() && i < 3; i++) {
            if (clusters[i].size() < 3)
                break;
            draw_pts(rgb, clusters[i], colors[i]);
            cv::rectangle(rgb, cv::boundingRect(clusters[i]), colors[i]);
        }

#endif //
    }

    char info[64];
    sprintf(info, "%d", id_);
    cv::putText(rgb, info, brc_.br(), cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(128, 255, 255));
    cv::rectangle(rgb, brc_, cv::Scalar(0, 255, 255));
}

void Target::debug_draw_path(cv::Mat &rgb, const Target::PATH &path, cv::Scalar &color) const
{
    if (path.size() >= 2) {
        cv::circle(rgb, path[0], 2, cv::Scalar(0, 0, 200));         // ��ʼ�㣬
        //cv::circle(rgb, path.back(), 2, cv::Scalar(0, 200, 200)); // ������ //
    }

    for (size_t i = 1; i < path.size(); i++) {
        cv::line(rgb, path[i - 1], path[i], color);
    }
}

bool Target::check_paths(double stamp)
{
    // ��Ϊ��һ֡ѡ��������ʱ������ĳЩ�����㲻���˶�Ŀ���ϣ�Ӧ��ȥ�� ...
    // FIXME: �����ڵ�5֡ʱ���м�� ....
    if (layers_.size() != 5) {
        return true;
    }

    // �ڵ���֡��ǿ��ɾ������̫С�������� ...
    int n = get_path_cnt();
    for (int i = n - 1; i >= 0; i--) {
        PATH path = get_path(i);
        if (distance(path) < min_dis_5frames_) {
            remove_path(i);
        }
    }

    return get_path_cnt() > min_pts_;
}

bool Target::check_alone_pts(const Target::PTS &pts, std::vector<bool> &status) const
{
    /**
        1. ��� pts ����10������������
        2. ����ƽ�����ģ�Ȼ�����ÿ���㵽ƽ�����ĵľ��룬Ȼ�������Щ�����ƽ��ֵ ..
        3. ���ĳ���㵽ƽ�����ĵľ���������� 2 �е�ƽ��ֵ������Ϊ�ĵ��������Ⱥ�㣬
     */
    if (pts.size() < 10)
        return false;

    cv::Point2f center(0, 0);
    for (PTS::const_iterator it = pts.begin(); it != pts.end(); ++it) {
        center.x += it->x, center.y += it->y;
    }

    center.x /= pts.size(), center.y /= pts.size();

    double sum = 0.0;
    std::vector<double> diss;
    for (size_t i = 0; i < pts.size(); i++) {
        double dis = ::distance(pts[i], center);
        diss.push_back(dis);
        sum += dis;
    }

    double avg_dis = sum / pts.size();

    for (size_t i = 0; i < diss.size(); i++) {
        if (diss[i] > 2.0 * avg_dis) {
            status.push_back(true);
        }
        else {
            status.push_back(false);
        }
    }

    return true;
}
