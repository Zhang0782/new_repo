cmake_minimum_required(VERSION 3.10)
project(RectangleDetector)

# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 寻找 OpenCV 库
find_package(OpenCV REQUIRED)

# 包含 OpenCV 头文件目录
include_directories(${OpenCV_INCLUDE_DIRS})

# 生成可执行文件，名为 detector，源文件是 main.cpp
add_executable(detector main.cpp)

# 链接 OpenCV 库
target_link_libraries(detector ${OpenCV_LIBS})
