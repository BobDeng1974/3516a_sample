#pragma once

#include <opencv2/opencv1.hpp>
#include "utils.h"
#include "KVConfig.h"
#include "hi_opencv.h"

/** ��Ӧһ���Ŀ�꣬����ʱ���ҵ������㣬Ȼ����г������٣�������֡������㡱�ϳ�ʱ��
    �����ˣ�����ΪĿ����������ʼ������Ŀ��Ļ ...

    TODO:
        ��Ϊ�����㲻һ���ڻĿ���ϣ��п������ڱ����ˣ���ʱĿ�������������ѣ���ν����������أ�

 */
class Target
{
    cv::Rect outer_;    // ȫ����߿� ...
    cv::Rect brc_;  // �켣��� ...
    cv::Rect first_rc_, last_rc_;   // ��ʼλ�ã����һ��ĸ��ٵ����� ...
    double stamp_;
    int id_;
    double min_dis_5frames_;    // ǰ��֡����Ч���������ٵ��ۼƾ��� ...
    int min_pts_;               // ��Ч���������Ŀ
    double stopped_dis_;        // ����Ϊ�ǡ�ֹͣ���ľ��� ...

    KVConfig *cfg_;

    typedef std::vector<cv::Point2f> PTS;   // ���һ��������� ...
    std::vector<PTS> layers_;               // ��Ӧ���������� ...

public:
    Target();
    ~Target();

public:
    typedef PTS PATH;

    bool init(KVConfig *cfg, int id, const cv::Rect &roi, const cv::Mat &curr, double stamp,
        double min_dis_5frames, int min_pts, int max_feature_pts, double qualitylevel);
    bool track(const cv::Mat &prev, const cv::Mat &curr, double stamp);

    /// ���켣�����֡�����û�б仯������Ϊ��Ѿ������� ...
    bool is_stopped() const;

    /// ���� rc �Ƿ����� target ....
    bool is_crossed(const cv::Rect &rc) const;

    /// ����pos
    inline cv::Rect pos() const
    {
        return brc_ & outer_;
    }

    /// ���ز��� ...
    inline int layers() const
    {
        return (int)layers_.size();
    }

    /// ����path��Ŀ
    inline int get_path_cnt() const
    {
        if (layers_.empty())
            return 0;
        else
            return (int)layers_[0].size();
    }

    /// ���ظ��ݾ��볤�̽������������ path
    std::vector<PATH> get_sorted_paths() const
    {
        return get_sorted_paths(0, (int)layers_.size());
    }

    /// ��������
    std::string descr() const
    {
        std::stringstream ss;
        ss << "ID:" << id_ << ", pos:" << pos2str(brc_) << ", layers:" << layers_.size() << ", paths:" << get_path_cnt();
        return ss.str();
    }

    /// ����������һ������һ�� ...
    bool cut()
    {
        if (layers_.size() > 2) {
            layers_.erase(layers_.begin() + 1, layers_.end() - 1);

            brc_ = cv::boundingRect(layers_[0]);
            brc_ |= cv::boundingRect(layers_[1]);

            return true;
        }
        else {
            return false;
        }
    }

    ///
    void debug_draw_paths(cv::Mat &rgb, cv::Scalar &color, int n = 10) const;

private:

    /// �ֲ�ת��ȫ������ ..
    inline void l2g(PTS &pts, const cv::Point &tl) const
    {
        for (PTS::iterator it = pts.begin(); it != pts.end(); ++it) {
            it->x += tl.x;
            it->y += tl.y;
        }
    }

    /// ȫ��ת�����ֲ� ...
    inline void g2l(PTS &pts, const cv::Point &tl) const
    {
        for (PTS::iterator it = pts.begin(); it != pts.end(); ++it) {
            it->x -= tl.x;
            it->y -= tl.y;
        }
    }

    /// ɾ�� idx ��Ӧ��·�� ...
    inline void remove_path(size_t idx)
    {
        //assert(layers_.size() > 0);
        //assert(idx < layers_[0].size());

        for (std::vector<PTS>::iterator it = layers_.begin(); it != layers_.end(); ++it) {
            it->erase(it->begin() + idx);
        }
    }

    /// ����һ��·��
    inline PATH get_path(int idx) const
    {
        return get_path(idx, 0, (int)layers_.size());
    }

    /// ����һ��·����һ���֡�...
    inline PATH get_path(int idx, int from_layer, int to_layer) const
    {
        //assert(layers_.size() > to_layer && from_layer < to_layer && idx < layers_[0].size());

        PTS path;
        for (int i = from_layer; i < to_layer; i++) {
            path.push_back(layers_[i][idx]);
        }

        return path;
    }

    /// ����path������дӴ�С����
    static inline bool op_bigger_dis(const PATH &p1, const PATH &p2)
    {
        return distance(p1) > distance(p2);
    }

    /// ��������֮���·�� ...
    std::vector<PATH> get_sorted_paths(int from, int to) const
    {
        //assert(from < to);
        //assert(layers_.size() >= to);
        //assert(from >= 0);

        std::vector<PATH> paths;
        int cnt = get_path_cnt();
        for (int i = 0; i < cnt; i++) {
            paths.push_back(get_path(i, from, to));
        }

        std::sort(paths.begin(), paths.end(), op_bigger_dis);
        return paths;
    }

    /// ����·�����ۼƳ���
    static inline double distance(const PATH &path)
    {
        if (path.size() < 2)
            return 0.0;

        double dis = 0;
        for (int i = 1; i < (int)path.size(); i++) {
            dis += ::distance(path[i - 1], path[i]);
        }

        return dis;
    }

    /// ��鳤ʱ��Ļ ...
    inline bool check_paths(double stamp);

    void debug_draw_path(cv::Mat &rgb, const PATH &path, cv::Scalar &color) const;

    /// �� pts ���ҳ�����Ⱥ�ĵ㣬���ö�Ӧλ�õ� status Ϊ 1 ..
    bool check_alone_pts(const PTS &pts, std::vector<bool> &status) const;
};
