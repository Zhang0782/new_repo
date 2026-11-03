#include <opencv2/opencv.hpp>
#include <iostream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <string>

namespace fs = std::filesystem;

// 角点排序：左下、右下、右上、左上（基于 OpenCV 图像坐标系：原点左上，y 向下）
std::vector<cv::Point2f> orderCorners(const cv::Point2f pts[4]) {
    std::vector<cv::Point2f> v(pts, pts + 4);
    // 按 y 坐标降序：y 越大越靠下
    std::sort(v.begin(), v.end(), [](const cv::Point2f& a, const cv::Point2f& b) {
        return a.y > b.y;
    });
    // v[0], v[1] 是下方两点；v[2], v[3] 是上方两点
    cv::Point2f bottomLeft, bottomRight, topLeft, topRight;
    if (v[0].x < v[1].x) {
        bottomLeft = v[0]; bottomRight = v[1];
    } else {
        bottomLeft = v[1]; bottomRight = v[0];
    }
    if (v[2].x < v[3].x) {
        topLeft = v[2]; topRight = v[3];
    } else {
        topLeft = v[3]; topRight = v[2];
    }
    return {bottomLeft, bottomRight, topRight, topLeft};
}

// 将 OpenCV 坐标（原点左上，y 向下）转换为考核坐标（原点左下，y 向上）
cv::Point2i toExamCoord(const cv::Point2f& p, int imgHeight) {
    return cv::Point2i(cvRound(p.x), imgHeight - cvRound(p.y));
}

int main() {
    std::string inputDir = "/input/";
    std::string outputDir = "/out/";

    // 创建输出目录（若不存在）
    if (!fs::exists(outputDir)) {
        fs::create_directories(outputDir);
    }

    // 收集输入目录中的 PNG 文件
    std::vector<fs::path> pngFiles;
    for (const auto& entry : fs::directory_iterator(inputDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".png") {
            pngFiles.push_back(entry.path());
        }
    }
    // 按文件名升序排序
    std::sort(pngFiles.begin(), pngFiles.end(), [](const fs::path& a, const fs::path& b) {
        return a.filename().string() < b.filename().string();
    });

    for (const auto& filePath : pngFiles) {
        std::string filename = filePath.filename().string();
        cv::Mat img = cv::imread(filePath.string(), cv::IMREAD_COLOR);
        if (img.empty()) {
            std::cerr << "无法读取: " << filename << std::endl;
            continue;
        }

        int imgHeight = img.rows;
        cv::Mat gray, binary;
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

        // 自适应阈值，适应明暗变化
        cv::adaptiveThreshold(gray, binary, 255,
                              cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                              cv::THRESH_BINARY_INV, 11, 2);

        // 形态学闭运算，连接断裂边缘
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, kernel);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        std::vector<std::vector<cv::Point2i>> detectedRects;

        for (const auto& contour : contours) {
            // 面积过滤：过滤太小或太大的噪声
            double area = cv::contourArea(contour);
            if (area < 500) continue;  // 可根据实际图片调整阈值

            // 拟合旋转矩形
            cv::RotatedRect rotRect = cv::minAreaRect(contour);

            // 长宽比过滤：排除过于细长的形状
            float w = rotRect.size.width;
            float h = rotRect.size.height;
            float ratio = std::max(w, h) / std::min(w, h);
            if (ratio > 10.0f) continue;

            cv::Point2f pts[4];
            rotRect.points(pts);

            // 按统一规则排序
            auto ordered = orderCorners(pts);

            // 转换为考核坐标
            std::vector<cv::Point2i> examPts;
            for (const auto& p : ordered) {
                examPts.push_back(toExamCoord(p, imgHeight));
            }

            // 绘制绿色边框（在原始 OpenCV 图像上直接绘制，不需要坐标转换）
            for (int j = 0; j < 4; ++j) {
                cv::line(img, ordered[j], ordered[(j + 1) % 4],
                         cv::Scalar(0, 255, 0), 2);
            }

            detectedRects.push_back(examPts);
        }

        // 保存输出图片
        std::string outPath = outputDir + filename;
        cv::imwrite(outPath, img);

        // 终端输出
        std::cout << filename << ": ";
        if (detectedRects.empty()) {
            std::cout << "null";
        } else {
            for (size_t i = 0; i < detectedRects.size(); ++i) {
                if (i > 0) std::cout << " | ";
                std::cout << "rect" << (i + 1) << " ";
                for (int j = 0; j < 4; ++j) {
                    std::cout << "(" << detectedRects[i][j].x << ", "
                              << detectedRects[i][j].y << ")";
                    if (j < 3) std::cout << "; ";
                }
            }
        }
        std::cout << std::endl;
    }

    return 0;
}
