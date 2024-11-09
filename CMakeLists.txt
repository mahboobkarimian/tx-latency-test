cmake_minimum_required(VERSION 3.10)
project(TimestampVideo)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Find FFmpeg Libraries
find_package(PkgConfig REQUIRED)

# Manually specify FFmpeg libraries and include directories
find_library(AVFORMAT_LIB avformat REQUIRED)
find_library(AVCODEC_LIB avcodec REQUIRED)
find_library(AVUTIL_LIB avutil REQUIRED)
find_library(SWSCALE_LIB swscale REQUIRED)

# Specify FFmpeg include directory explicitly if needed
include_directories(/usr/include)

pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale)
include_directories(${FFMPEG_INCLUDE_DIRS})

# Add executable
add_executable(timestamp_video b.cpp)

# Link libraries
target_link_libraries(timestamp_video ${OpenCV_LIBS} ${FFMPEG_LIBRARIES})
