#include <opencv2/opencv.hpp>
#include <iostream>
#include <opencv2/videoio.hpp>
#include <vector>
#include <algorithm>
#include "status.h"

using namespace cv;
using namespace std;

extern Car_mgr* car_mgr;

void processFrame(const Mat& frame, Car_mgr& mgr) {
    if (frame.empty()) return;

    // 清空上一次保存的车辆链表
    Car* cur = mgr.origin_head;
    while (cur != nullptr) {
        Car* next = cur->next;
        delete cur;
        cur = next;
    }
    mgr.origin_head = mgr.head = nullptr;

    // 1. 显示原始图像
    imshow("原始图像", frame);
    waitKey(500); // 暂停0.5秒查看

    Mat hsv;
    cvtColor(frame, hsv, COLOR_BGR2HSV);

    // 2. 提取紫色跑道区域
    Mat purpleMask;
    inRange(hsv, Scalar(150, 80, 80), Scalar(170, 255, 255), purpleMask);
    Mat kernel = getStructuringElement(MORPH_RECT, Size(5, 5));
    morphologyEx(purpleMask, purpleMask, MORPH_OPEN, kernel);
    morphologyEx(purpleMask, purpleMask, MORPH_CLOSE, kernel);

    imshow("紫色跑道 Mask", purpleMask);
    waitKey(500);

    vector<vector<Point>> contoursTrack;
    findContours(purpleMask, contoursTrack, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    if (contoursTrack.empty()) return;

    int maxIdx = 0;
    double maxArea = 0;
    for (size_t i = 0; i < contoursTrack.size(); i++) {
        double area = contourArea(contoursTrack[i]);
        if (area > maxArea) {
            maxArea = area;
            maxIdx = i;
        }
    }

    RotatedRect box = minAreaRect(contoursTrack[maxIdx]);
    Point2f pts[4];
    box.points(pts);
    sort(pts, pts+4, [](Point2f a, Point2f b){ return a.y < b.y; });
    Point2f topPts[2] = { pts[0], pts[1] };
    Point2f botPts[2] = { pts[2], pts[3] };
    if (topPts[0].x > topPts[1].x) swap(topPts[0], topPts[1]);
    if (botPts[0].x > botPts[1].x) swap(botPts[0], botPts[1]);

    Point2f topLeft = topPts[0], topRight = topPts[1];
    Point2f bottomLeft = botPts[0], bottomRight = botPts[1];

    double widthTop = norm(topRight - topLeft);
    double widthBot = norm(bottomRight - bottomLeft);
    double maxWidth = max(widthTop, widthBot);
    double heightLeft = norm(bottomLeft - topLeft);
    double heightRight = norm(bottomRight - topRight);
    double maxHeight = max(heightLeft, heightRight);

    vector<Point2f> srcPts = { topLeft, topRight, bottomRight, bottomLeft };
    vector<Point2f> dstPts = { Point2f(0,0), Point2f(maxWidth-1,0),
                               Point2f(maxWidth-1,maxHeight-1), Point2f(0,maxHeight-1) };
    Mat M = getPerspectiveTransform(srcPts, dstPts);
    Mat warped;
    warpPerspective(frame, warped, M, Size((int)maxWidth, (int)maxHeight));

    imshow("透视变换图像", warped);
    waitKey(500);

    // 3. 提取红色车辆
    Mat warpedHSV;
    cvtColor(warped, warpedHSV, COLOR_BGR2HSV);
    Mat redMask;
    inRange(warpedHSV, Scalar(0, 0, 0), Scalar(180, 255, 60), redMask);
    morphologyEx(redMask, redMask, MORPH_OPEN, kernel);
    morphologyEx(redMask, redMask, MORPH_CLOSE, kernel);

    imshow("红色车辆 Mask", redMask);
    waitKey(500);

    vector<vector<Point>> contoursRed;
    findContours(redMask, contoursRed, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // 可视化图像
    Mat carVis = warped.clone();

    // 4. 插入当前帧检测到的车辆
    for (auto& cnt : contoursRed) {
        double area = contourArea(cnt);
        if (area < 100.0) continue;
        Moments m = moments(cnt);
        if (m.m00 == 0) continue;
        float cx = float(m.m10 / m.m00);
        float cy = float(m.m01 / m.m00);

        double pos_x = cy / maxHeight;
        double center = cx - maxWidth / 2.0;

        Car* node = new Car();
        node->pos_x = pos_x;
        node->center = center;
        node->prev = mgr.head;
        node->next = nullptr;

        if (mgr.head) {
            mgr.head->next = node;
            mgr.head = node;
        } else {
            mgr.origin_head = mgr.head = node;
        }

        // 可视化中心点
        circle(carVis, Point((int)cx, (int)cy), 5, Scalar(0, 255, 0), -1);
    }

    imshow("检测到的车辆中心", carVis);
    waitKey(500);

    // 输出
    int idx = 0;
    for (Car* p = mgr.origin_head; p != nullptr; p = p->next) {
        cout << "Car[" << idx++ << "]: x=" << p->pos_x 
             << ", center=" << p->center << endl;
    }
}

/**
 * 处理视频流：逐帧读取并调用processFrame处理
 */
void processVideo(const string& videoPath, Car_mgr& mgr) {
    VideoCapture cap(0,CAP_V4L2);
    // 支持摄像头（参数为0）或视频文件路径
    if (videoPath == "0") cap.open(0,CAP_V4L2);
    else cap.open(videoPath,CAP_V4L2);
    if (!cap.isOpened()) {
        cerr << "无法打开视频源" << endl;
        return;
    }
    Mat frame;
    while (cap.read(frame)) {
        processFrame(frame, mgr);
        // 可视化（非必须）：将红色轮廓画回图像
        //imshow("Frame", frame);
        //if (waitKey(30) >= 0) break;
    }
    cap.release();
}

int main(int argc, char** argv) {
    Car_mgr mgr;
    mgr.car_num = 0;
    mgr.origin_head = mgr.head = nullptr;
    if (argc < 2) {
        cout << "请指定图像文件或视频路径参数" << endl;
        return -1;
    }
    string inputPath = argv[1];
    // 判断输入类型：图像还是视频/摄像头
    Mat image = imread(inputPath);
    if (!image.empty()) {
        // 单张图像处理
        processFrame(image, mgr);
    } else {
        // 视频流处理
        processVideo(inputPath, mgr);
    }
    cout << "通过终点线的车辆总数: " << mgr.car_num << endl;
    return 0;
}
