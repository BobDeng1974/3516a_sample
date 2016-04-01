#include <opencv2/imgproc/imgproc.hpp>
#include<stdio.h>
#include<iostream>
#include<stdexcept>
#include<cstdlib>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "hog.h"
using namespace cv;


int HOG_MIN_WIDTH = 148;
int HOG_MIN_HEIGHT = 131;



static std::vector<float> load_lear_model(const char* model_file)
{
     std::vector<float>  detector;
     FILE *modelfl;
     if ((modelfl = fopen (model_file, "rb")) == NULL)
     {
         return detector;
     }

     char version_buffer[10];
     if (!fread (&version_buffer,sizeof(char),10,modelfl))
     {
         return detector;
     }

     if(strcmp(version_buffer,"V6.01"))
     {
        return detector;
     }
     // read version number
     int version = 0;
     if (!fread (&version,sizeof(int),1,modelfl))
     {
         return detector;
     }

     if (version < 200)
     {
         return detector;
      }

     long kernel_type;
     fread(&(kernel_type),sizeof(long),1,modelfl);

     {// ignore these
        long poly_degree;
        fread(&(poly_degree),sizeof(long),1,modelfl);

        double rbf_gamma;
        fread(&(rbf_gamma),sizeof(double),1,modelfl);

        double  coef_lin;
        fread(&(coef_lin),sizeof(double),1,modelfl);
        double coef_const;
        fread(&(coef_const),sizeof(double),1,modelfl);

        long l;
        fread(&l,sizeof(long),1,modelfl);
        char* custom = new char[l];
        fread(custom,sizeof(char),l,modelfl);
        delete[] custom;
    }

    long totwords;
    fread(&(totwords),sizeof(long),1,modelfl);

    {// ignore these
        long totdoc;
        fread(&(totdoc),sizeof(long),1,modelfl);

        long sv_num;
        fread(&(sv_num), sizeof(long),1,modelfl);
    }

    double linearbias = 0.0;
    fread(&linearbias, sizeof(double),1,modelfl);

    if(kernel_type == 0) { /* linear kernel */
        /* save linear wts also */
        double* linearwt = new double[totwords+1];
        int length = totwords;
        fread(linearwt, sizeof(double),totwords+1,modelfl);

        for(int i = 0;i<totwords;i++){
            float term = linearwt[i];
            detector.push_back(term);
        }
        float term = -linearbias;
        detector.push_back(term);
        delete [] linearwt;

    } else {
    }

    fclose(modelfl);
    return detector;
}

zk_hog_detector::zk_hog_detector(const char *alt_file,KVConfig *cfg, cv::Size winsize)
{
    hog.winSize = winsize;
   // hog.blockSize = cv::Size(16, 16);
   // hog.blockStride = cv::Size(8, 8);
    std::vector<float> vec = load_lear_model(alt_file);
    if (vec.empty())
        throw std::runtime_error("zk_hog_detector: can't find the alt file");
    //for(std::vector<float>::iterator iter = vec.begin(); iter != vec.end(); iter++)
    //{
    //    printf("%lf ",*iter);
    //}
    //printf("\n");
    hog.blockSize.width = 16;
    hog.blockSize.height = 16;
    hog.blockStride.width = 2;
    hog.blockStride.height = 2;
    printf("%d,%d\n",hog.winSize.width, hog.winSize.height);
    printf("%d,%d,%d,%d\n",hog.blockSize.width, hog.blockSize.height, hog.blockStride.width, hog.blockStride.height);
    printf("line=%d\n",__LINE__);
    hog.setSVMDetector(vec);
    printf("line=%d\n",__LINE__);
    //************************
    UP_BODY_WIDTH =  atoi(cfg->get_value("t_upbody_w","70"));
    UP_BODY_HEIGHT = atoi(cfg->get_value("t_upbody_h","90"));
    det_.long_inter_time = atoi(cfg->get_value("t_hog_l_inter_time","1000"));//ÿ���100������һ��ͷ��;
    det_.short_inter_time = atoi(cfg->get_value("t_hog__s_inter_time","200"));//ÿ���100������һ��ͷ��;
    det_.inter_time =det_.long_inter_time;//ÿ���100������һ��ͷ��;
    det_.start = false;
    det_.timeing_start = false;
    if(atoi(cfg->get_value("debug","0"))>0)
        is_debug = true;
    else
        is_debug = false;

    //***********************
    cf_ = new tracker_camshift(cfg->get_value("trace_camshift_meta_file", "data/hist.yaml"));
    printf("line=%d\n",__LINE__);
    if(atoi(cfg->get_value("t_upbody_strict","0"))>0) det_.is_strict = true;
    else det_.is_strict = false;
    printf("line=%d\n",__LINE__);
}

std::vector<cv::Rect> zk_hog_detector::zk_hog_upperbody(const cv::Mat &gray, int group_threshold , double scale)
{
    std::vector<cv::Rect> found;
    hog.detectMultiScale(gray, found, 0, cv::Size(16, 16), cv::Size(), scale, group_threshold);
    return found;
}

//��չ�ڵ��˶���;
//���룺target���ڵ�������У�
//�������չ���ڵ�������У����Խ���ͷ��ʶ��
void zk_hog_detector::expend_target_rect(cv::Rect &target,cv::Size image)
{
    //������չʵ���˿��2��;
    int up_w = (image.width/480) * UP_BODY_WIDTH;
    int up_h = (image.width/480) * UP_BODY_HEIGHT;

    target.x = target.x+target.width/2 - (up_w);
    target.width = (up_w)*2;

    target.y = target.y - (up_h + 30);
    target.height = 2 * (up_h+30);

    target &= cv::Rect(0,0,image.width,image.height);
}

//����ʵ�ʳߴ��ȡ��Ҫ��С�����߶�;
float zk_hog_detector::set_zoom_ratio(int source_width)
{
    float ratio;
    //������������С���Լ���ʶ��ʱ�䣩;
    float w_ratio = HOG_MIN_WIDTH*1.0/((source_width/480)*UP_BODY_WIDTH);
    float h_ratio = HOG_MIN_HEIGHT*1.0/((source_width/480)*UP_BODY_HEIGHT);
    if(w_ratio>h_ratio)
    {
        ratio = w_ratio;
    }
    else
    {
        ratio = h_ratio;
    }
    return ratio;
}


//�� rc ��Χ�ڣ����С�ͷ�硱ʶ��
bool zk_hog_detector::zk_hog_detect( cv::Mat img,const cv::Rect rc ,cv::Rect &face)
{
    bool is_face = false;
    cv::Rect rc_temp = rc;
    expend_target_rect(rc_temp,Size(img.cols,img.rows));
    float ratio = set_zoom_ratio(img.cols);
    Mat img_mask = Mat(img,rc_temp);
    Mat img_mask_gray(img.cols,img.rows,CV_8UC1);
    cvtColor(img_mask,img_mask_gray,CV_BGR2GRAY);
    resize(img_mask_gray,img_mask_gray,Size(img_mask_gray.cols*ratio, img_mask_gray.rows*ratio));
    std::vector<cv::Rect> up_body_t = zk_hog_upperbody(img_mask_gray,1);

    std::vector<cv::Rect> up_body;
    if(up_body_t.size()>0)
    {
        for(int i = 0;i<up_body_t.size();i++)
        {
            if (is_debug)
            {
               rectangle(img_mask_gray, up_body_t[i], Scalar(0, 0, 0), 2);//���зŴ�ͷ���+++++++
            }
            //�ȷŴ�
            up_body_t[i].x /= ratio;
            up_body_t[i].y /= ratio;
            up_body_t[i].width /= ratio;
            up_body_t[i].height /= ratio;

            up_body_t[i].x = up_body_t[i].x+rc_temp.x;
            up_body_t[i].y = up_body_t[i].y+rc_temp.y;
            up_body_t[i] &= cv::Rect(0,0,img.cols,img.rows);
            if (is_debug)
            {
                rectangle(img, up_body_t[i], Scalar(0, 0, 255), 2);//����ͷ���+++++++
            }
            //������ֱ��ͼ��ͷ���;
            cv::Rect r_t = up_body_t[i];
            cf_->set_track_window(r_t);
            cv::Rect face = cf_->process(img,true);
            if(face.width>0&&face.height>0 && cf_->is_face(face,up_body_t[i]))
            {
                up_body.push_back(up_body_t[i]);
                if (is_debug)
                {
                    rectangle(img, face, Scalar(255, 0, 0), 2);//����ͷ���+++++++
                }
            }
        }
        if(!det_.is_strict)
        {
            if(up_body.size()<1)//�����û�����������Ǳ���ԭ����;
            {up_body = up_body_t;}
        }
    }
    //ѡȡ��Ŀ�������ƫ�������Ŀ��;
    if(up_body.size()>0)
    {
        is_face = true;
        face = up_body[0];
        Point t_up = Point((up_body[0].x+up_body[0].width/2),(up_body[0].y+up_body[0].height/2));
        Point t_rc = Point((rc.x+rc.width/2),(rc.y+rc.height/2));
        float len_min = abs(t_up.x-t_rc.x);
        for(int i = 1;i<up_body.size();i++)
        {
            Point t_up = Point((up_body[i].x+up_body[i].width/2),(up_body[i].y+up_body[i].height/2));
            Point t_rc = Point((rc.x+rc.width/2),(rc.y+rc.height/2));
            float len = abs(t_up.x-t_rc.x);
            if(len<len_min)
            {
                len_min = len;
                face = up_body[i];
            }
        }
        if (is_debug)
        {
            rectangle(img, face, Scalar(255, 255, 0), 4);//��ȷͷ���+++++++
        }
    }
    if (is_debug)
    {
        rectangle(img, rc_temp, Scalar(255, 255, 255), 2);//+++++++
        imshow("img_mask_gray", img_mask_gray);//++++++++++
        waitKey(1);
    }
    return is_face;
}



//����������õ�ͷ�������;
bool zk_hog_detector::up_bd_detect(cv::Mat Img,std::vector <cv::Rect> r,cv::Rect &up_body)
{
    bool is_upbody = false;
    bool time_ok = false;
    if(!det_.timeing_start)
    {
        ftime(&det_.pre_time);ftime(&det_.cur_time);
        det_.timeing_start = true;
    }
    else
    {
        ftime(&det_.cur_time);
        double time = (det_.cur_time.time - det_.pre_time.time)*1000+(det_.cur_time.millitm-det_.pre_time.millitm);
        if(time>=det_.inter_time)//����;
        {
            time_ok = true;det_.timeing_start = false;
        }
    }
    if(  time_ok && r.size() == 1 && r[0].width < UP_BODY_WIDTH*(Img.cols/480)*3)
    {
        //fprintf(stderr,"%f**********************\n",inter);
        is_upbody = zk_hog_detect(Img,r[0],up_body);
        if(!is_upbody) { det_.inter_time = det_.short_inter_time;}
        else {det_.inter_time = det_.long_inter_time;}
    }
    return is_upbody;
}


zk_hog_detector::~zk_hog_detector()
{

}

