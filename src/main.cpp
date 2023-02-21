/*
 * @Version: v1.0
 * @Author: 東DONG
 * @Mail: cv_yang@126.com
 * @Date: 2022-11-01 15:05:36
 * @LastEditTime: 2023-02-20 17:44:59
 * @FilePath: /YOLO_TRT_SIM/src/main.cpp
 * @Description: 
 * Copyright (c) 2022 by ${東}, All Rights Reserved. 
 * 
 *    ┏┓　　　┏┓
 *  ┏┛┻━━━┛┻┓
 *  ┃　　　　　　　┃
 *  ┃　　　━　　　┃
 *  ┃　＞　　　＜　┃
 *  ┃　　　　　　　┃
 *  ┃...　⌒　...　┃
 *  ┃　　　　　　　┃
 *  ┗━┓　　　┏━┛
 *      ┃　　　┃　
 *      ┃　　　┃
 *      ┃　　　┃
 *      ┃　　　┃  神兽保佑
 *      ┃　　　┃  代码无bug　　
 *      ┃　　　┃
 *      ┃　　　┗━━━┓
 *      ┃　　　　　　　┣┓
 *      ┃　　　　　　　┏┛
 *      ┗┓┓┏━┳┓┏┛
 *        ┃┫┫　┃┫┫
 *        ┗┻┛　┗┻┛
 */

#include "yolo.hpp"

#if defined(_WIN32)
#	include <Windows.h>
#   include <wingdi.h>
#	include <Shlwapi.h>
#	pragma comment(lib, "shlwapi.lib")
#   pragma comment(lib, "ole32.lib")
#   pragma comment(lib, "gdi32.lib")
#	undef min
#	undef max
#else
#	include <dirent.h>
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <unistd.h>
#   include <stdarg.h>
#endif


namespace YOLO_TRT_SIM{

    using namespace std;

    // 优化打印INFO
    namespace log{

            #define INFO(...)  YOLO_TRT_SIM::log::__printf(__FILE__, __LINE__, __VA_ARGS__)

            void __printf(const char* file, int line, const char* fmt, ...){

                va_list vl;
                va_start(vl, fmt);

                printf("\e[32m[%s:%d]:\e[0m ", file, line);
                vprintf(fmt, vl);
                printf("\n");
            }
        };

    /*
     static 全局变量：改变作用范围，不改变存储位置
     static 局部变量：改变生命周期，不改变作用范围
     static 静态函数: 只能在声明文件中可见，其他不可见 ，也可避免重名冲突
    */

    // 训练标签
    static const char* cocolabels[] = {
        "person", "bicycle", "car", "motorcycle", "airplane",
        "bus", "train", "truck", "boat", "traffic light", "fire hydrant",
        "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse",
        "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack",
        "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis",
        "snowboard", "sports ball", "kite", "baseball bat", "baseball glove",
        "skateboard", "surfboard", "tennis racket", "bottle", "wine glass",
        "cup", "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich",
        "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake",
        "chair", "couch", "potted plant", "bed", "dining table", "toilet", "tv",
        "laptop", "mouse", "remote", "keyboard", "cell phone", "microwave",
        "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase",
        "scissors", "teddy bear", "hair drier", "toothbrush"
    };


    // hsv-->bgr
    static std::tuple<uint8_t, uint8_t, uint8_t> hsv2bgr(float h, float s, float v){
        // static_cast<type基本数据类型>(content)
        
        const int h_i = static_cast<int>(h * 6);
        const float f = h * 6 - h_i;
        const float p = v * (1 - s);
        const float q = v * (1 - f*s);
        const float t = v * (1 - (1 - f) * s);
        float r, g, b;
        switch (h_i) {
        case 0:r = v; g = t; b = p;break;
        case 1:r = q; g = v; b = p;break;
        case 2:r = p; g = v; b = t;break;
        case 3:r = p; g = q; b = v;break;
        case 4:r = t; g = p; b = v;break;
        case 5:r = v; g = p; b = q;break;
        default:r = 1; g = 1; b = 1;break;}
        return make_tuple(static_cast<uint8_t>(b * 255), static_cast<uint8_t>(g * 255), static_cast<uint8_t>(r * 255));
    }


    // 随机颜色
    static std::tuple<uint8_t, uint8_t, uint8_t> random_color(int id){
        float h_plane = ((((unsigned int)id << 2) ^ 0x937151) % 100) / 100.0f;;
        float s_plane = ((((unsigned int)id << 3) ^ 0x315793) % 100) / 100.0f;
        return hsv2bgr(h_plane, s_plane, 1);
    }


    static bool exists(const string& path){

    #ifdef _WIN32
        return ::PathFileExistsA(path.c_str());
    #else
        return access(path.c_str(), R_OK) == 0;
    #endif
    }


    // 获取文件名 
    static string get_file_name(const string& path, bool include_suffix){

        if (path.empty()) return "";

        int p = path.rfind('/');
        int e = path.rfind('\\');
        p = std::max(p, e);
        p += 1;

        if (include_suffix)
            return path.substr(p);

        int u = path.rfind('.');
        if (u == -1)
            return path.substr(p);

        if (u <= p) u = path.size();
        return path.substr(p, u - p);
    }


    static double timestamp_now_float() {
        return chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
    }

    
    // 在线获取onnx model
    bool requires_model(const string& name) {

        auto onnx_file = cv::format("%s.onnx", name.c_str());
        if (!exists(onnx_file)) {
            
            printf("Auto download %s\n", onnx_file.c_str());
            system(cv::format("wget http://zifuture.com:1556/fs/25.shared/%s", onnx_file.c_str()).c_str());
        }

        bool isexists = exists(onnx_file);
        if (!isexists) {
            printf("Download %s failed\n", onnx_file.c_str());
        }
        return isexists;
    }


    // inference image
    static void inference_image(int deviceid, const string& engine_file, YOLO::Mode mode, YOLO::Type type, const string& model_name){
        // 创建推理引擎，同时初始化处理线程
        auto engine = YOLO::create_infer(engine_file, type, deviceid, 0.25f, 0.5f);

        if(engine == nullptr){
            printf("Engine is nullptr\n");
            return;
        }

        vector<cv::String> files_;
        /*
        std::vector::reserve
        void reserve(szie_type n)
        @function 容器预留n个元素长度，预留位置不初始化，元素不可访问
                  避免内存重新分配，如果需要更多空间，则会发生重新分配
        */
        files_.reserve(10000);

        cv::glob("inference/*.jpg", files_, true);
        
        vector<string> files(files_.begin(), files_.end());
        
        vector<cv::Mat> images;
        for(int i = 0; i < files.size(); ++i){
            auto image = cv::imread(files[i]);
            images.emplace_back(image);
        }

        // warmup 热启动:模型预热
        vector<shared_future<YOLO::BoxArray>> boxes_array;
        for(int i = 0; i < 100; ++i)
            boxes_array = engine->commits(images);

        boxes_array.back().get();
        boxes_array.clear();
        
        const int ntest = 100;
        auto begin_timer = timestamp_now_float();

        for(int i  = 0; i < ntest; ++i)
            boxes_array = engine->commits(images);
        
        // wait all result
        boxes_array.back().get();

        float infer_average_time = (timestamp_now_float() - begin_timer) / ntest / images.size();
        INFO("%s average: %.2f ms/image, fps: %.2f", engine_file.c_str(), infer_average_time, 1000 / infer_average_time);

        string infer_result = "result";

        // 画框
        for(int i = 0; i < boxes_array.size(); ++i){

            auto& image = images[i];
            auto boxes  = boxes_array[i].get();
            
            for(auto& obj : boxes){
                uint8_t b, g, r;
                tie(b, g, r) = random_color(obj.class_label);
                cv::rectangle(image, cv::Point(obj.left, obj.top), cv::Point(obj.right, obj.bottom), cv::Scalar(b, g, r), 5);

                auto name    = cocolabels[obj.class_label];
                auto caption = cv::format("%s %.2f", name, obj.confidence);
                int width    = cv::getTextSize(caption, 0, 1, 2, nullptr).width + 10;
                cv::rectangle(image, cv::Point(obj.left-3, obj.top-33), cv::Point(obj.left + width, obj.top), cv::Scalar(b, g, r), -1);
                cv::putText(image, caption, cv::Point(obj.left, obj.top-5), 0, 1, cv::Scalar::all(0), 2, 16);
            }

            string file_name = get_file_name(files[i], false);
            string save_path = cv::format("%s/%s.jpg", infer_result.c_str(), file_name.c_str());

            INFO("save to %s, %d object, average time %.2f ms", save_path.c_str(), boxes.size(), infer_average_time);
            cv::imwrite(save_path, image);
        }
        engine.reset();
    }

    // inference video
    static void inference_video(int deviceid, const string& engine_file, YOLO::Mode mode, YOLO::Type type, const string& model_name){
        // 创建推理引擎，同时初始化处理线程
        auto engine = YOLO::create_infer(engine_file, type, deviceid, 0.25f, 0.5f);

        if(engine == nullptr){
            printf("Engine is nullptr\n");
            return;
        }

        cv::Mat image;
        cv::VideoCapture cap("vtest.avi");

        while(cap.read(image)){

            auto begin_timer = timestamp_now_float();
            auto boxes_array = engine->commit(image); 
          
            // wait all result
            auto boxes = boxes_array.get();

            float infer_average_time = (timestamp_now_float() - begin_timer);
            INFO("%s average: %.2f ms/image, fps: %.2f", engine_file.c_str(), infer_average_time, 1000 / infer_average_time);
            
            std::stringstream fpss;
            fpss << "FPS:" << float(1000.0f / infer_average_time);

            // 画框
            for(auto& obj : boxes){
                uint8_t b, g, r;
                tie(b, g, r) = random_color(obj.class_label);    
                cv::rectangle(image, cv::Point(obj.left, obj.top), cv::Point(obj.right, obj.bottom), cv::Scalar(b, g, r), 5);
                auto name    = cocolabels[obj.class_label];
                auto caption = cv::format("%s %.2f", name, obj.confidence);
                int width    = cv::getTextSize(caption, 0, 1, 2, nullptr).width + 10;
                cv::rectangle(image, cv::Point(obj.left-3, obj.top-33), cv::Point(obj.left + width, obj.top), cv::Scalar(b, g, r), -1);
                cv::putText(image, caption, cv::Point(obj.left, obj.top-5), 0, 1, cv::Scalar::all(0), 2, 16);

                cv::putText(image, fpss.str(), cv::Point(0, 25), 0, 1, cv::Scalar::all(0), 2, 16);

                }

            cv::imshow("test", image);
            if(cv::waitKey(1) == 27) break;
            
        }

        cap.release();
        cv::destroyAllWindows();

        engine.reset();
           
    }
    
    // test
    static void test(YOLO::Type type, YOLO::Mode mode, const string& model){

        int deviceid = 0;
        auto mode_name = YOLO::mode_string(mode);
        YOLO::set_device(deviceid);

        const char* name = model.c_str();
        INFO("====================== test %s %s ==============================", mode_name, name);
        
        // 判断模型是否存在，不存在就在线下载
        if(!requires_model(name))
            return;

        string onnx_file = cv::format("%s.onnx", name);
        string model_file = cv::format("%s_fp16.engine", name);
        
        int test_batch_size = 1;
        
        // 判断是否导出trt模型，如果导出则继续向下执行，反之进行导出操作
        if(!exists(model_file)){
            YOLO::compile(
                mode, type,                 // FP32、FP16、INT8
                test_batch_size,            // max batch size
                onnx_file,                  // source 
                model_file,                 // save to
                1 << 30,
                "calibrate/calib",          // int8 量化校准
                "calibrate/calib.cache"
            );
        }
        // 推理
        inference_image(deviceid, model_file, mode, type, name);
        // inference_video(deviceid, model_file, mode, type, name);
    }
    
}

int main(){
   //X V E 
    YOLO_TRT_SIM::test(YOLO::Type::E, YOLO::Mode::FP16, "edgeyolo");
    return 0;
}