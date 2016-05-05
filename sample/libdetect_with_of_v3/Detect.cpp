#include "Detect.h"
#include "utils.h"

Detect::Detect(KVConfig *cfg)
    : cfg_(cfg)
{
    system("rm detlog/*.jpg");
    system("rm detlog/*.path");

    debug_ = atoi(cfg->get_value("debug", "0")) == 1;
    log_init = log_init_file;
    log = log_file;

    log_init(0);

    /// ǿ��ʹ�� 960x540
    //cfg->set_value("video_width", "960");
    //cfg->set_value("video_height", "540");

    cfg->set_value("video_width", "480");
    cfg->set_value("video_height", "360");

    cfg->save_as(0);


    od_ = new ObjectDetect(cfg_);

    reset();
}

Detect::~Detect()
{
}

void Detect::reset()
{
    od_->reload();

    cnt_ = 0;
    max_duration_ = atof(cfg_->get_value("max_duration", "15.0"));
    waiting_ = atof(cfg_->get_value("max_wait", "2.0"));

    debug_ = atoi(cfg_->get_value("debug", "0")) == 1;
    log_init = log_init_file;
    log = log_file;

    log("Detect config:\n");
    log("\tmax_duration: %.2f\n", max_duration_);
    log("\tmax_wait: %.2f\n", waiting_);
    log("\n");

    masked_ = build_mask(mask_);

    standups_.clear();
    gray_prev_.release();
    gray_prev_.cols = 0;
    gray_prev_.rows = 0;
}

void Detect::detect(const cv::Mat &origin, Detect::RCS &standups)
{
    stamp_ = now();
    cnt_++;
    origin_ = origin;

    //if (masked_) {
    //    cv::bitwise_and(origin, mask_, origin);
    //}
    //printf("line:%d, time:%ld\n", __LINE__, GetTickCount());
    cv::cvtColor(origin, gray_curr_, cv::COLOR_BGR2GRAY);
    if (gray_prev_.cols == 0) {
        gray_prev_ = gray_curr_.clone();
    }
    //printf("line:%d, time:%ld\n", __LINE__, GetTickCount());

    if (support_detect2()) {
        detect2(standups);
        //printf("line:%d, time:%ld\n", __LINE__, GetTickCount());
    }
    else {
        RCS motions;
        std::vector<Dir> dirs;
        detect(motions, dirs);
        //printf("line:%d, time:%ld\n", __LINE__, GetTickCount());

        cv::swap(gray_prev_, gray_curr_);
        //printf("line:%d, time:%ld\n", __LINE__, GetTickCount());

        // ����  motions/dirs �� standups_ ��Ӱ�� ...
        for (size_t i = 0; i < dirs.size(); i++) {
            STANDUPS::iterator it = find_crossed_target(motions[i]);
            if (it == standups_.end()) {
                // �µĻ���� ...
                // ������������򴴽�һ���µ� standup
                if (dirs[i] == UP) {
                    Standup s;
                    s.enable_od = od_->enabled();

                    std::vector<cv::Rect> faces;
                    if (s.enable_od) {
                        // �� motions[i].pos ���ϰ벿�ֽ���ͷ��ʶ�� ...
                        cv::Rect roi = motions[i];
                        roi.height /= 2;    // FIXME:
                        //printf("line:%d, time:%ld\n", __LINE__, GetTickCount());
                        faces = od_->detect(gray_curr_, roi);
                        //printf("line:%d, time:%ld\n", __LINE__, GetTickCount());
                        if (faces.empty()) {
                            log("WRN: %u: ��Ŀ�꣺%s, �����Ҳ���ͷ�磬ʧ��\n", cnt_, pos2str(motions[i]).c_str());
                            continue;
                        }
                        else {
                            s.face = faces[0];
                        }
                    }

                    s.pos = motions[i];
                    s.stamp = stamp_;
                    s.waiting = false;
                    standups_.push_back(s);

                    if (s.enable_od) {
                        log("INFO: %u: ��Ŀ�꣺%s, ͷ��λ��: %s, %lu\n", cnt_, pos2str(s.pos).c_str(), pos2str(s.face).c_str(), faces.size());
                    }
                    else {
                        log("INFO: %u: ��Ŀ�꣺%s\n", cnt_, pos2str(s.pos).c_str());
                    }
                }
            }
            else if (it->waiting) {
                // �ȴ��ڣ�ֱ�Ӻ����µĻ ...
                log("WRN: %u: ���� Waiting �, %s: pos=%s\n", cnt_, DirDesc[dirs[i]], pos2str(it->pos).c_str());
            }
            else {
                // ����ȴ��� ...
                it->waiting = true;
                it->stamp = stamp_;
                log("INFO: %u: Ŀ����ʧ������Waiting��%s: pos=%s\n", cnt_, DirDesc[dirs[i]], pos2str(it->pos).c_str());
            }
        }
        //printf("line:%d, time:%ld\n", __LINE__, GetTickCount());

        // ��鳬ʱ ...
        for (STANDUPS::iterator it = standups_.begin(); it != standups_.end();) {
            if (stamp_ - it->stamp > max_duration_) {
                log("WRN: %u: ɾ����ʱվ��: %s, max_duration=%.2f\n",
                    cnt_, pos2str(it->pos).c_str(), max_duration_);
                it = standups_.erase(it);   // ��ʱɾ�� ...
            }
            else if (it->waiting) {
                if (stamp_ - it->stamp > waiting_) {
                    log("INFO: %u: ɾ��waiting����: %s\n", cnt_, pos2str(it->pos).c_str());
                    it = standups_.erase(it);  // ɾ�� waiting ...
                }
                else {
                    ++it;
                }
            }
            else {
                standups.push_back(it->pos);  // ��ЧĿ�� ...
                ++it;
            }
        }
        //printf("line:%d, time:%ld\n", __LINE__, GetTickCount());
    }
}

void Detect::detect(const cv::Mat &origin, std::vector<Detect::STANDUP> &ups)
{
    stamp_ = now();
    cnt_++;
    origin_ = origin;

    //if (masked_) {
    //    cv::bitwise_and(origin, mask_, origin);
    //}

    cv::cvtColor(origin, gray_curr_, cv::COLOR_BGR2GRAY);
    if (gray_prev_.cols == 0) {
        gray_prev_ = gray_curr_.clone();
    }

    RCS motions;
    std::vector<Dir> dirs;
    detect(motions, dirs);

    cv::swap(gray_prev_, gray_curr_);

    // ����  motions/dirs �� standups_ ��Ӱ�� ...
    for (size_t i = 0; i < dirs.size(); i++) {
        STANDUPS::iterator it = find_crossed_target(motions[i]);
        if (it == standups_.end()) {
            // �µĻ���� ...
            // ������������򴴽�һ���µ� standup
            if (dirs[i] == UP) {
                Standup s;
                s.enable_od = od_->enabled();

                std::vector<cv::Rect> faces;
                if (s.enable_od) {
                    // �� motions[i].pos ���ϰ벿�ֽ���ͷ��ʶ�� ...
                    cv::Rect roi = motions[i];
                    roi.height /= 2;    // FIXME:
                    faces = od_->detect(gray_curr_, roi);
                    if (faces.empty()) {
                        log("WRN: %u: ��Ŀ�꣺%s, �����Ҳ���ͷ�磬ʧ��\n", cnt_, pos2str(motions[i]).c_str());
                        continue;
                    }
                    else {
                        s.face = faces[0];
                    }
                }

                s.pos = motions[i];
                s.stamp = stamp_;
                s.waiting = false;
                standups_.push_back(s);

                if (s.enable_od) {
                    log("INFO: %u: ��Ŀ�꣺%s, ͷ��λ��: %s, %lu\n", cnt_, pos2str(s.pos).c_str(), pos2str(s.face).c_str(), faces.size());
                }
                else {
                    log("INFO: %u: ��Ŀ�꣺%s\n", cnt_, pos2str(s.pos).c_str());
                }
            }
        }
        else if (it->waiting) {
            // �ȴ��ڣ�ֱ�Ӻ����µĻ ...
            log("WRN: %u: ���� Waiting �, %s: pos=%s\n", cnt_, DirDesc[dirs[i]], pos2str(it->pos).c_str());
        }
        else {
            // ����ȴ��� ...
            it->waiting = true;
            it->stamp = stamp_;
            log("INFO: %u: Ŀ����ʧ������Waiting��%s: pos=%s\n", cnt_, DirDesc[dirs[i]], pos2str(it->pos).c_str());
        }
    }

    // ��鳬ʱ ...
    for (STANDUPS::iterator it = standups_.begin(); it != standups_.end();) {
        if (stamp_ - it->stamp > max_duration_) {
            log("WRN: %u: ɾ����ʱվ��: %s, max_duration=%.2f\n",
                cnt_, pos2str(it->pos).c_str(), max_duration_);
            it = standups_.erase(it);   // ��ʱɾ�� ...
        }
        else if (it->waiting) {
            if (stamp_ - it->stamp > waiting_) {
                log("INFO: %u: ɾ��waiting����: %s\n", cnt_, pos2str(it->pos).c_str());
                it = standups_.erase(it);  // ɾ�� waiting ...
            }
            else {
                ++it;
            }
        }
        else {
            STANDUP up;
            up.pos = it->pos;
            up.stamp_begin = it->stamp;
            up.enable_od = it->enable_od;
            up.obj_pos = it->face;
            ups.push_back(up);

            ++it;
        }
    }
}
