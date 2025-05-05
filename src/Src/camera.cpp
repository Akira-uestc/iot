#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include "status.h"

using namespace cv;
using namespace std;

extern Car_mgr* car_mgr;

/*
 * 检测图像中的白色水平线（假设有起点和终点两条明显的白线）
 * 返回检测到的候选线（使用最小外接矩形）
 */
vector<RotatedRect> detect_white_lines(const Mat& img) {
    Mat gray, thresh;
    // 转为灰度图
    cvtColor(img, gray, COLOR_BGR2GRAY);
    // 二值化阈值（突出白线，视情况可调整参数）
    threshold(gray, thresh, 200, 255, THRESH_BINARY);
    // 形态学操作（闭运算）填补线条间隙
    Mat kernel = getStructuringElement(MORPH_RECT, Size(15, 5));
    morphologyEx(thresh, thresh, MORPH_CLOSE, kernel);
    // 查找轮廓
    vector<vector<Point>> contours;
    findContours(thresh, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    vector<RotatedRect> lines;
    for (size_t i = 0; i < contours.size(); i++) {
        // 用最小外接矩形拟合轮廓
        RotatedRect rect = minAreaRect(contours[i]);
        float width = rect.size.width;
        float height = rect.size.height;
        // 计算长宽比，过滤出长而窄的矩形（水平线）
        if (width < height) std::swap(width, height);
        float aspect = width / height;
        // 宽度较长且长宽比大（>10可判定为横向白线）
        if (aspect > 10 && width > 50) {
            lines.push_back(rect);
        }
    }
    // 按中心y坐标排序（从上到下）
    sort(lines.begin(), lines.end(), [](const RotatedRect &a, const RotatedRect &b){
        return a.center.y < b.center.y;
    });
    // 如果检测到多于两条线，只取上下两条
    if (lines.size() > 2) {
        vector<RotatedRect> selected;
        selected.push_back(lines.front());
        selected.push_back(lines.back());
        return selected;
    }
    return lines;
}

/*
 * 获取赛道四个角点坐标，用于透视变换
 * 参数：两条检测到的白线（farLine为远端白线，nearLine为近端白线）
 */
vector<Point2f> getTrackCorners(const RotatedRect& farLine, const RotatedRect& nearLine) {
    // 提取白线矩形的顶点
    Point2f ptsFar[4], ptsNear[4];
    farLine.points(ptsFar);
    nearLine.points(ptsNear);
    // 将点按x坐标排序，从左到右
    sort(ptsFar, ptsFar+4, [](const Point2f &a, const Point2f &b){ return a.x < b.x; });
    sort(ptsNear, ptsNear+4, [](const Point2f &a, const Point2f &b){ return a.x < b.x; });
    // 左侧点和右侧点
    Point2f farLeft = ptsFar[0];
    Point2f farRight = ptsFar[3];
    Point2f nearLeft = ptsNear[0];
    Point2f nearRight = ptsNear[3];
    // 返回点序列：近端左, 近端右, 远端右, 远端左（逆时针）
    return vector<Point2f>{ nearLeft, nearRight, farRight, farLeft };
}

/*
 * 核心函数：检测跑道上的车辆
 * - 白线检测 -> 透视变换 -> 跑道遮罩（紫色） -> 红色车辆检测 -> 结果存储
 * 返回指向Car_mgr的指针，包含所有检测到的车辆信息链表
 */
Car_mgr* detect_cars_on_track(const Mat& input_frame) {
    Mat frame = input_frame.clone();
    // 1. 检测白线（跑道起点和终点）
    vector<RotatedRect> white_lines = detect_white_lines(frame);
    if (white_lines.size() < 2) {
        // 未检测到足够的白线，无法判断跑道
        return nullptr;
    }
    // 按y排序，假设摄像头在近端（下方），近端白线y值较大
    sort(white_lines.begin(), white_lines.end(), [](const RotatedRect &a, const RotatedRect &b){
        return a.center.y < b.center.y;
    });
    RotatedRect farLine = white_lines[0];   // 远端（起点）白线
    RotatedRect nearLine = white_lines[1];  // 近端（终点）白线
    // 2. 计算透视变换
    vector<Point2f> srcPts = getTrackCorners(farLine, nearLine);
    // 目标图像大小（根据需要可调整大小）
    int outWidth = 1000;
    int outHeight = 200;
    // 目标四点：将跑道映射到输出图像的四个角
    vector<Point2f> dstPts = {
        Point2f(0, outHeight),
        Point2f(0, 0),
        Point2f(outWidth, 0),
        Point2f(outWidth, outHeight)
    };
    Mat M = getPerspectiveTransform(srcPts, dstPts);  // 计算透视变换矩阵:contentReference[oaicite:7]{index=7}
    Mat warped;
    warpPerspective(frame, warped, M, Size(outWidth, outHeight));
    // 3. 跑道（紫色）区域检测
    Mat hsv;
    cvtColor(warped, hsv, COLOR_BGR2HSV);
    Mat maskPurple;
    // 设定紫色阈值范围（根据具体情况调整HSV范围）
    Scalar lowerPurple(125, 50, 50), upperPurple(155, 255, 255);
    inRange(hsv, lowerPurple, upperPurple, maskPurple);
    // 膨胀腐蚀去噪
    morphologyEx(maskPurple, maskPurple, MORPH_OPEN, getStructuringElement(MORPH_ELLIPSE, Size(5,5)));
    morphologyEx(maskPurple, maskPurple, MORPH_CLOSE, getStructuringElement(MORPH_ELLIPSE, Size(15,15)));
    // 4. 红色车辆检测
    Mat maskRed;
    Mat maskRed1, maskRed2;
    // 红色在HSV色调上有两个区间（绕过0°边界）
    Scalar lowerRed1(0, 70, 50),  upperRed1(10, 255, 255);
    Scalar lowerRed2(170, 70, 50), upperRed2(180, 255, 255);
    inRange(hsv, lowerRed1, upperRed1, maskRed1);
    inRange(hsv, lowerRed2, upperRed2, maskRed2);
    maskRed = maskRed1 | maskRed2;
    // 去噪
    morphologyEx(maskRed, maskRed, MORPH_OPEN, getStructuringElement(MORPH_ELLIPSE, Size(5,5)));
    morphologyEx(maskRed, maskRed, MORPH_CLOSE, getStructuringElement(MORPH_ELLIPSE, Size(15,15)));
    // 查找红色区域轮廓
    vector<vector<Point>> contours;
    findContours(maskRed, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE); // :contentReference[oaicite:8]{index=8}
    // 准备结果链表
    Car_mgr* mgr = new Car_mgr();
    mgr->car_num = 0;
    mgr->origin_head = nullptr;
    mgr->head = nullptr;
    Car* prevCar = nullptr;
    // 遍历每个轮廓，提取中心坐标并判断是否在跑道内
    for (size_t i = 0; i < contours.size(); i++) {
        Rect bbox = boundingRect(contours[i]);
        double cx = bbox.x + bbox.width / 2.0;
        double cy = bbox.y + bbox.height / 2.0;
        // 只考虑在跑道（紫色区域）内的车辆
        if (maskPurple.at<uchar>((int)cy, (int)cx) == 0)
            continue;
        double pos_x = 1.0 - (cx / outWidth);  // x坐标归一化（远端为0，近端为1）
        // 创建新节点存储车辆信息
        Car* carEntry = new Car();
        carEntry->pos_x = pos_x;
        carEntry->center = cy;
        carEntry->next = nullptr;
        carEntry->prev = prevCar;
        if (prevCar) prevCar->next = carEntry;
        prevCar = carEntry;
        if (mgr->origin_head == nullptr) {
            mgr->origin_head = carEntry;
        }
        mgr->car_num++;
    }
    mgr->head = mgr->origin_head;
    return mgr;
}

/* // 示例主程序：打开摄像头或视频，检测并输出车辆位置
int main(int argc, char** argv) {
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cerr << "无法打开视频或摄像头。" << endl;
        return -1;
    }
    Mat frame;
    while (true) {
        cap >> frame;
        if (frame.empty()) break;
        Car_mgr* mgr = detect_cars_on_track(frame);
        // 输出检测结果
        if (mgr) {
            Car* cur = mgr->head;
            while (cur) {
                cout << "车辆: 归一化X=" << cur->pos_x 
                     << ", 垂直中心=" << cur->center << endl;
                cur = cur->next;
            }
            // 释放链表内存
            Car* temp = mgr->origin_head;
            while (temp) {
                Car* toDelete = temp;
                temp = temp->next;
                delete toDelete;
            }
            delete mgr;
        }
        // 显示当前帧（可选）
        imshow("Frame", frame);
        if (waitKey(30) == 27) break; // 按ESC退出
    }
    return 0;
}
 */