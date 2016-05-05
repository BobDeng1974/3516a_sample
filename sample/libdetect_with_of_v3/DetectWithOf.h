#pragma once

#define _USE_MATH_DEFINES

#include "Detect.h"
#include "DiffMotion.h"
#include "Target.h"

/** ʹ��ϡ�������¼����������ֹͣ�󣬼���Ĺ켣��������Ч�Ļ ...

    @date: pc ���ʺ�һ֡���ٶ��С���򣨶�ξֲ��Ĺ��������� arm �Ͽ����ʺ���һ�������ĸ��� ...
 */
class DetectWithOf : public Detect
{
    cv::Mat debug_img_;     // ��ʾ������Ϣ ..

    bool track_once_;   // �Ƿ����һ�� ...

    DiffMotion dm_;
    std::vector<Target> targets_;   // �Ŀ�� ...

    bool support_long_time_tracking_;   // �Ƿ�֧�ֳ�ʱ����٣�����˵����⵽վ���󣬼������٣�ֱ��ȡ��վ��״̬ ...
    std::vector<Target> standups_;  // վ����Ŀ�� ...

    // ѧ��������ţ���ǰ�ŵ�λ�ã��ڴ�֮�䣬ʹ��������� ...
    float far_y_, near_y_;  // ���� y �� ...

    float far_width_, near_width_;  // ֡����ε�����y�Ŀ����ֵ�����С�ڸ�ֵ������Ϊ֡�����̫С�� ...
    double a_width_, b_width_;      // ��ȵ�ֱ�߷���ϵ�� ...

    float far_dis_[4], near_dis_[4];    // �ĸ�����ľ�����ֵ ...
    double a_dis_[4], b_dis_[4];    //

    int up_angle_;      //

    int next_tid_;      // Target ID

    bool check_larger_; // �Ƿ��鳬������ ...
    int large_threshold_;   //

    // �������㳤������ N ��·�� ...
    int n_paths_for_stats_; // Ĭ�� 8�����ܳ��� 10

    double target_min_dis_5frames_; // һ��Ŀ�����Ч�����㣬��ǰ5֡�ڣ����������ƶ��ľ��룬���ǰ5֡����û�л��������Ϊ��Щ�������Ҵ��� ...
    int target_min_pts_;            // һ����ЧĿ�꣬����ӵ�е���������Ŀ ...
    int max_feature_pts_;           // ��ʼ�����������Ŀ ...
    double feature_quality_level_;  // goodFeaturesToTrack() �� quality level ���� ..

public:
    explicit DetectWithOf(KVConfig *cfg);
    ~DetectWithOf();

    virtual void reset();

private:
    virtual void detect(RCS &motions, std::vector<Dir> &dirs);
    virtual void detect2(RCS &standups);

    virtual bool support_detect2() const
    {
        return support_long_time_tracking_;
    }

    inline std::vector<Target>::iterator find_matched_target_from(const cv::Rect &rc, std::vector<Target> &targets)
    {
        for (std::vector<Target>::iterator it = targets.begin(); it != targets.end(); ++it) {
            if (it->is_crossed(rc)) {
                return it;
            }
        }
        return targets.end();
    }

    /// �ҵ��� rc �ཻ�� target ...
    inline std::vector<Target>::iterator find_matched_target(const cv::Rect &rc)
    {
        return find_matched_target_from(rc, targets_);
    }

    /// �� standups �в��� ..
    inline std::vector<Target>::iterator find_matched_target_from_ltt(const cv::Rect &rc)
    {
        return find_matched_target_from(rc, standups_);
    }

    /// ����Զ�����жϻ����Ŀ�� ...
    bool too_small(const cv::Rect &rc) const;

    enum {
        E_OK,
        E_NotEnoughLayers,
        E_NotEnoughPaths,
        E_NotEnoughDistance,
    };

    /// ����target�Ļ�������Ч���򷵻� OK
    int estimate(const Target &target, cv::Rect &pos, Dir &dir) const;

    /// ����������ֱ�ߵ� a,b ϵ��
    inline void polyfit_line(const cv::Point2f &p1, const cv::Point2f &p2, double &a, double &b) const
    {
        cv::Point2f p = p2;
        if (p.x == p1.x) {   // NOTE: ��ֹ��ֱֱ�� ...
            p.x += (float)0.01;
        }

        a = (p1.y - p.y) / (p1.x - p.x);
        b = p.y - a * p.x;
    }

    /// �������㣬��Ƕȣ��� x ��˳ʱ����תһ�ܣ�[0..360)
    inline double calc_angle(const cv::Point2f &p1, const cv::Point2f &p2) const
    {
        double a = atan2(-(p2.y - p1.y), p2.x - p1.x) * 180.0 / M_PI;
        if (a > 0) {  // ��һ������ ..
            a = 360.0 - a;
        }
        else {  // �������� ..
            a = -a;
        }

        return a;
    }

    /// �������� targets_������ֹͣ��� targets
    std::vector<Target> track_targets();

    /// �����µĻ���� ...
    void process_diff_motions();

    /// ������Ч� ...
    void debug_save_motion(const Target &target, const cv::Rect &pos, Dir &dir) const;

    /// ���θ��� ...
    std::vector<Target> track_targets_once();
};
