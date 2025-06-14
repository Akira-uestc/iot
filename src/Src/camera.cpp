//
// Created by akira on 6/14/25.
//
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <iostream>
#include <opencv2/videoio.hpp>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include "status.h"

extern Car_mgr* car_mgr;

void show_image(const cv::Mat& image, const std::string& window_name) {
    cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);
    cv::imshow(window_name, image);
    cv::waitKey(0);
    cv::destroyAllWindows();
}

std::vector<float> calculateZeroSegmentCenters(const std::vector<int>& column_mean) {
    std::vector<float> centers;
    int n = column_mean.size();
    if(n == 0) return centers;

    int start = -1;
    bool in_segment = false;

    for(int i = 0; i < n; ++i) {
        if(column_mean[i] == 0) {
            if(!in_segment) {
                start = i;
                in_segment = true;
            }
        } else {
            if(in_segment) {
                float center = (start + (i-1)) / 2.0f;
                centers.push_back(center / n);
                in_segment = false;
            }
        }
    }

    if(in_segment) {
        float center = (start + (n-1)) / 2.0f;
        centers.push_back(center / n);
    }

    return centers;
}

void processFrame(cv::Mat frame,Car_mgr& mgr) {
    std::vector<float> centers;
    cv::Mat image = frame.clone();
    cv::Mat hsv;
    cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);

    Car* cur = mgr.origin_head;
    while (cur != nullptr) {
        Car* next = cur->next;
        delete cur;
        cur = next;
    }
    mgr.origin_head = mgr.head = nullptr;

    cv::Scalar lower_purple(150, 80, 80);
    cv::Scalar upper_purple(170, 255, 255);
    cv::Mat purple_mask;
    cv::inRange(hsv, lower_purple, upper_purple, purple_mask);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT,cv::Size(3, 3));
    // cv::morphologyEx(purple_mask, purple_mask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(purple_mask, purple_mask, cv::MORPH_CLOSE, kernel);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(purple_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    if (contours.empty()) {
        std::cerr << "没有找到紫色区域。" << std::endl;
        Car* cur = mgr.origin_head;
        while(cur != nullptr) {
            Car* next = cur->next;
            delete cur;
            cur = next;
        }
        mgr.origin_head = mgr.head = nullptr;
        return;
    }

    auto max_contour = *std::max_element(contours.begin(), contours.end(),
        [](const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
            return cv::contourArea(a) < cv::contourArea(b);
        });

    cv::Rect rect = cv::boundingRect(max_contour);
    cv::Mat cropped = purple_mask(rect);

    std::vector<int> column_mean(cropped.cols, 0);
    int threshold = 0;
    int mean_sum = 255;

    for (int x = 0; x < cropped.cols; ++x) {
        int y_start = -1, y_end = -1;

        for (int y = 0; y < cropped.rows; ++y) {
            if (cropped.at<uchar>(y, x)) {
                y_start = y;
                break;
            }
        }

        for (int y = cropped.rows - 1; y >= 0; --y) {
            if (cropped.at<uchar>(y, x)) {
                y_end = y;
                break;
            }
        }
        if (y_start >= 0 && y_end >= y_start) {
            int sum = 0;
            for (int y = y_start; y <= y_end; ++y) {
                sum += cropped.at<uchar>(y, x);
            }
            column_mean[x] = sum / (y_end - y_start + 1);
            threshold = mean_sum / (x + 1);
            mean_sum += column_mean[x];
            // std::cout << "x: " << x << ", column_mean: " << column_mean[x] << ", threshold: " << threshold << std::endl;
            if (column_mean[x] >= threshold) {
                column_mean[x] = 1;
            } else column_mean[x] = 0;
        }
    }
    // for (size_t i = 0; i < column_mean.size(); ++i) {
    //     std::cout << column_mean[i] << " ";
    // }
    //
    centers.clear();
    centers = calculateZeroSegmentCenters(column_mean);

    // for (size_t i = 0; i < centers.size(); ++i) {
    //     std::cout << "Center " << i << ": " << centers[i] << std::endl;
    // }
    for (float center : centers) {
        Car* node = new Car();
        node->center = center;
        node->prev = mgr.head;
        node->next = nullptr;

        if (mgr.head) {
            mgr.head->next = node;
            mgr.head = node;
        } else {
            mgr.origin_head = mgr.head = node;
        }
    }
}

void processVideo(const std::string& videoPath, Car_mgr& mgr) {
    cv::VideoCapture cap(0,cv::CAP_V4L2);
    if (videoPath == "0") cap.open(0,cv::CAP_V4L2);
    else cap.open(videoPath,cv::CAP_V4L2);

    if (!cap.isOpened()) {
        std::cerr << "无法打开视频源" << std::endl;
        return;
    }

    cv::Mat frame;
    while (cap.read(frame)) {
        processFrame(frame, mgr);
        sleep(1);
    }
    cap.release();
}

void* car_update(void* arg) {
    (void)arg;
    car_mgr->origin_head = car_mgr->head = nullptr;
    while (1) {
        processVideo("/dev/video0",*car_mgr);
    }
}

