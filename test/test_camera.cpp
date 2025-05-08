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
    Mat hsv;
    cvtColor(frame, hsv, COLOR_BGR2HSV);

    // --- 1. 提取紫色跑道区域，检测跑道轮廓 --- 
    Mat purpleMask;
    // 紫色HSV范围 (需要根据实际情况调整阈值)
    inRange(hsv, Scalar(150, 80, 80), Scalar(170, 255, 255), purpleMask);
    // 形态学处理：开运算去噪、闭运算填充
    Mat kernel = getStructuringElement(MORPH_RECT, Size(5, 5));
    morphologyEx(purpleMask, purpleMask, MORPH_OPEN, kernel);
    morphologyEx(purpleMask, purpleMask, MORPH_CLOSE, kernel);

    // 查找紫色区域轮廓，选择最大区域作为跑道
    vector<vector<Point>> contoursTrack;
    findContours(purpleMask, contoursTrack, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    if (contoursTrack.empty()) return; // 未检测到跑道
    // 找到面积最大的轮廓
    int maxIdx = 0;
    double maxArea = 0;
    for (unsigned long i = 0; i < contoursTrack.size(); i++) {
        double area = contourArea(contoursTrack[i]);
        if (area > maxArea) { maxArea = area; maxIdx = i; }
    }
    // 最小外接矩形获取四个角点
    RotatedRect box = minAreaRect(contoursTrack[maxIdx]);
    Point2f pts[4];
    box.points(pts);
    // 对四个点按照纵坐标排序（前两个为顶部，后两个为底部）
    sort(pts, pts+4, [](Point2f a, Point2f b){ return a.y < b.y; });
    Point2f topPts[2] = { pts[0], pts[1] };
    Point2f botPts[2] = { pts[2], pts[3] };
    // 对左右进行排序
    if (topPts[0].x > topPts[1].x) swap(topPts[0], topPts[1]);
    if (botPts[0].x > botPts[1].x) swap(botPts[0], botPts[1]);
    Point2f topLeft = topPts[0], topRight = topPts[1];
    Point2f bottomLeft = botPts[0], bottomRight = botPts[1];
    
    // --- 2. 透视变换：将跑道校正为矩形俯视图 ---
    // 计算输出图像大小：宽度取顶边或底边最大值，高度取左右边最大值
    double widthTop = norm(topRight - topLeft);
    double widthBot = norm(bottomRight - bottomLeft);
    double maxWidth = max(widthTop, widthBot);
    double heightLeft = norm(bottomLeft - topLeft);
    double heightRight = norm(bottomRight - topRight);
    double maxHeight = max(heightLeft, heightRight);
    // 源图4个点和目标图4个点
    vector<Point2f> srcPts = { topLeft, topRight, bottomRight, bottomLeft };
    vector<Point2f> dstPts = { Point2f(0,0), Point2f(maxWidth-1,0), 
                               Point2f(maxWidth-1,maxHeight-1), Point2f(0,maxHeight-1) };
    Mat M = getPerspectiveTransform(srcPts, dstPts);
    Mat warped;
    warpPerspective(frame, warped, M, Size((int)maxWidth, (int)maxHeight));
    // 处理完成后，warped图像中跑道纵向坐标与真实距离线性对应

    // --- 3. 颜色分割：提取红色车辆区域 ---
    Mat warpedHSV;
    cvtColor(warped, warpedHSV, COLOR_BGR2HSV);
    Mat redMask;
    // 红色HSV范围：红色在0度和180度处，分两段处理
    //Mat redMask1, redMask2;
    //inRange(warpedHSV, Scalar(0, 120, 70), Scalar(10, 255, 255), redMask1);
    //inRange(warpedHSV, Scalar(170, 120, 70), Scalar(180, 255, 255), redMask2);
    inRange(warpedHSV, Scalar(0, 0, 0), Scalar(180, 255, 60), redMask);
    // 形态学去噪：开运算去小噪点，闭运算填充车体区域
    morphologyEx(redMask, redMask, MORPH_OPEN, kernel);
    morphologyEx(redMask, redMask, MORPH_CLOSE, kernel);

    // --- 4. 轮廓检测：找到红色车辆并计算质心 ---
    vector<vector<Point>> contoursRed;
    findContours(redMask, contoursRed, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    vector<Point2f> carCenters;  // 存储检测到的车辆质心
    for (auto& cnt : contoursRed) {
        double area = contourArea(cnt);
        if (area < 100.0) continue; // 忽略过小噪声
        Moments m = moments(cnt);
        if (m.m00 == 0) continue;
        float cx = float(m.m10 / m.m00);
        float cy = float(m.m01 / m.m00);
        carCenters.push_back(Point2f(cx, cy));
    }

    // --- 5. 更新链表：匹配新检测的车辆位置与现有车辆 ---
    // 首先，标记所有节点为未更新状态
    for (Car* p = mgr.origin_head; p != nullptr; p = p->next) {
        // 使用pos_x < 0表示未更新的标记 (这里简化处理)
        p->pos_x = -1.0;
    }
    // 对每个检测到的新车辆质心
    for (auto& cen : carCenters) {
        // 计算归一化纵坐标 pos_x 和横向偏移 center
        double pos_x = cen.y / maxHeight;          // 归一化纵坐标 (0~1)
        double center = cen.x - maxWidth / 2.0;    // 相对跑道中心线偏移
        // 在链表中查找匹配节点（横向距离最小且尚未更新）
        Car* bestMatch = nullptr;
        double bestDist = 1e9;
        for (Car* p = mgr.origin_head; p != nullptr; p = p->next) {
            if (p->pos_x < 0) continue; // 跳过标记为未更新的（已使用）
            double dx = p->center - center;
            double dist = fabs(dx);
            if (dist < bestDist && dist < maxWidth*0.05) { // 阈值匹配
                bestDist = dist;
                bestMatch = p;
            }
        }
        if (bestMatch) {
            // 更新匹配车辆节点
            bestMatch->pos_x = pos_x;
            bestMatch->center = center;
        } else {
            // 新车辆：创建节点加入链表尾部
            Car* node = new Car();
            node->pos_x = pos_x;
            node->center = center;
            node->next = nullptr;
            if (mgr.origin_head == nullptr) {
                // 链表为空，初始化头尾指针
                node->prev = nullptr;
                mgr.origin_head = mgr.head = node;
            } else {
                // 追加到末尾
                node->prev = mgr.head;
                mgr.head->next = node;
                mgr.head = node;
            }
            mgr.head->next = nullptr;
        }
    }

    // --- 6. 移除通过终点线的车辆节点 ---
    Car* cur = mgr.origin_head;
    while (cur != nullptr) {
        Car* next = cur->next;
        if (cur->pos_x > 1.0) {
            // 已过终点，删除节点并计数
            if (cur->prev) cur->prev->next = cur->next;
            if (cur->next) cur->next->prev = cur->prev;
            if (cur == mgr.origin_head) mgr.origin_head = cur->next;
            if (cur == mgr.head) mgr.head = cur->prev;
            mgr.car_num += 1;
            delete cur;
        }
        cur = next;
    }

    // --- 输出当前车辆信息 ---
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
