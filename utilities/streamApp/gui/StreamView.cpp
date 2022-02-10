/*
 * StreamView.cpp
 *
 * Copyright © 2019 Intel Corporation. All rights reserved.
 *
 * The source code contained or described herein and all documents related to the source code
 * ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the
 * Material remains with Intel Corporation or its suppliers and licensors. The Material may
 * contain trade secrets and proprietary and confidential information of Intel Corporation
 * and its suppliers and licensors, and is protected by worldwide copyright and trade secret
 * laws and treaty provisions. No part of the Material may be used, copied, reproduced,
 * modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way
 * without Intel’s prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual property right
 * is granted to or conferred upon you by disclosure or delivery of the Materials, either
 * expressly, by implication, inducement, estoppel or otherwise. Any license under such
 * intellectual property rights must be express and approved by Intel in writing.
 *
 */

#include <vector>
#include <string>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "StreamView.h"
#include "Stream.h"
#include "RealsenseLog.h"

using namespace std;
using namespace cv;
using namespace realsense::camera_sub_system;
using namespace realsense::utils;

namespace realsense {
namespace camera_viewer {

StreamView::StreamView(uint8_t nodeNumber,
                       realsense::camera_sub_system::Stream &stream,
                       V4L2Utils::StreamUtils::StreamType st) :
        mNodeNumber{nodeNumber},
        mStream{stream},
        mFormat{V4L2_PIX_FMT_Z16, 1280, 720, 30},
        mStreamType {st}
{
    RS_AUTOLOG();
    mNodeStr = {"/dev/video"};
    mNodeStr += to_string(mNodeNumber);
}

void StreamView::draw()
{
    RS_AUTOLOG();

    namedWindow(mNodeStr.c_str(), WINDOW_FREERATIO);
    mCtrlWinName = mNodeStr + " control";
    namedWindow(mCtrlWinName.c_str(), WINDOW_NORMAL);
    createTrackbar("stream on/off", mCtrlWinName.c_str(), &mStartStopTBVlaue, 1, StreamView::startStopTrackbarCB, static_cast<void*>(this));
    switch(mStreamType) {
    case V4L2Utils::StreamUtils::StreamType::RS_Y8_STREAM:
    case V4L2Utils::StreamUtils::StreamType::RS_Y12I_STREAM:
    case V4L2Utils::StreamUtils::StreamType::RS_DEPTH_STREAM:
        {
            // Get current values of ctrls
            bool laserMode;
            mStream.getLaserMode(&laserMode);

            bool ae;
            mStream.getAE(&ae);

            int exp;
            mStream.getExposure(&exp);
            RS_LOGI("exposure %d ++++++++++++++++++++++++++++++", exp);

            int val;
            val = static_cast<int>(ae);
            createTrackbar("ae", mCtrlWinName.c_str(), &val, 1, StreamView::aETrackbarCB, static_cast<void*>(this));
            createTrackbar("exposure", mCtrlWinName.c_str(), &exp, 2000, StreamView::exposureTrackbarCB, static_cast<void*>(this));
            val = static_cast<int>(laserMode);
            createTrackbar("laser power mode", mCtrlWinName.c_str(), &val, 1, StreamView::laserModeTrackbarCB, static_cast<void*>(this));
            createTrackbar("laser power value", mCtrlWinName.c_str(), nullptr, 10, StreamView::laserValueTrackbarCB, static_cast<void*>(this));
        }
        break;
    case V4L2Utils::StreamUtils::StreamType::RS_RGB_STREAM:
        {
            // Get current values of ctrls
            bool ae;
            mStream.getAE(&ae);

            int exp;
            mStream.getExposure(&exp);
            RS_LOGI("exposure %d ++++++++++++++++++++++++++++++", exp);

            int val;
            val = static_cast<int>(ae);
            createTrackbar("ae", mCtrlWinName.c_str(), &val, 1, StreamView::aETrackbarCB, static_cast<void*>(this));
            createTrackbar("exposure", mCtrlWinName.c_str(), &exp, 10000, StreamView::exposureTrackbarCB, static_cast<void*>(this));
        }
        break;
    }

    //cv::waitKey();
    start(V4L2_MEMORY_MMAP);
}

StreamView::~StreamView()
{
}

void StreamView::startStopTrackbarCB (int pos, void *userData) {
    RS_LOGI("pos %d, userdata %p", pos, userData);
    StreamView* sv = static_cast<StreamView*>(userData);
    switch (pos) {
    case 0:
        //sv->stop();
        break;
    case 1:
        //sv->start(V4L2_MEMORY_MMAP);
        break;
    default:
        RS_LOGE("Invalid value %d", pos);
    }
}

void StreamView::aETrackbarCB (int pos, void *userData) {
    RS_LOGI("pos %d, userdata %p", pos, userData);
    StreamView* sv = static_cast<StreamView*>(userData);
    sv->setAE(pos);
    if (!pos)
        sv->setExposure(getTrackbarPos("exposure", sv->mCtrlWinName.c_str()));
}

void StreamView::exposureTrackbarCB (int pos, void *userData) {
    RS_LOGI("pos %d, userdata %p", pos, userData);
    StreamView* sv = static_cast<StreamView*>(userData);
    if (getTrackbarPos("ae", sv->mCtrlWinName.c_str()))
        setTrackbarPos("ae", sv->mCtrlWinName.c_str(), 0);
    else
        sv->setExposure(pos);
}

void StreamView::laserModeTrackbarCB (int pos, void *userData) {
    RS_LOGI("pos %d, userdata %p", pos, userData);
    StreamView* sv = static_cast<StreamView*>(userData);
    sv->setLaserMode(pos);
}

void StreamView::laserValueTrackbarCB (int pos, void *userData) {
    RS_LOGI("pos %d, userdata %p", pos, userData);
    StreamView* sv = static_cast<StreamView*>(userData);
    sv->setLaserValue(pos);
}

void StreamView::fpsTrackbarCB (int pos, void *userData) {
    RS_LOGI("pos %d, userdata %p", pos, userData);
    StreamView* sv = static_cast<StreamView*>(userData);
    sv->setFPS(pos);
}

void StreamView::slaveModeTrackbarCB (int pos, void *userData) {
    RS_LOGI("pos %d, userdata %p", pos, userData);
    StreamView* sv = static_cast<StreamView*>(userData);
    sv->setSlaveMode(pos);
}

void StreamView::start(uint32_t memoryType) {
    RS_AUTOLOG();

    mMemoryType = memoryType;

    // Allocate Buffers
    uint32_t bufferLength;
    switch(mStreamType) {
    case V4L2Utils::StreamUtils::StreamType::RS_RGB_STREAM:
        mFormat.v4l2Format = V4L2_PIX_FMT_YUYV;
        break;
    case V4L2Utils::StreamUtils::StreamType::RS_DEPTH_STREAM:
        mFormat.v4l2Format = V4L2_PIX_FMT_Z16;
        break;
    case V4L2Utils::StreamUtils::StreamType::RS_Y8_STREAM:
        mFormat.v4l2Format = V4L2_PIX_FMT_GREY;
        break;
    case V4L2Utils::StreamUtils::StreamType::RS_Y12I_STREAM:
        mFormat.v4l2Format = V4L2_PIX_FMT_Y12I;
        break;
    default:
        break;
    }
    bufferLength = mFormat.calc64BytesAlignedStride() * mFormat.height;

    mStream.setSlaveMode(mSlaveMode);

    if (mStream.configure(mFormat) < 0)
        RS_LOGE("Configure failed");
        //return;

    switch (memoryType) {
    case V4L2_MEMORY_MMAP:
        for (int i = 0; i < mBuffersCount; i++) {
            // TODO: use resolution
            mRsBuffers.emplace_back(nullptr, bufferLength, i);
        }
        break;
    case V4L2_MEMORY_USERPTR:
        for (int i = 0; i < mBuffersCount; i++) {
            // TODO: use resolution
            mRsBuffers.emplace_back(malloc(bufferLength), bufferLength, i);
        }
        break;
    default:
        RS_LOGE("Non supported memory type %d", memoryType);
    }

    for (int i = 0; i < mBuffersCount; i++) {
        RS_LOGI("pushing ptr to buffer %d", mRsBuffers[i].index);
        mRsBuffersPtrs.push_back(&mRsBuffers[i]);
    }

    mStream.start(mRsBuffersPtrs,
                  std::bind(&StreamView::proccesCaptureResult,
                            this,
                            std::placeholders::_1),
                  memoryType);

    //mStartStopTBVlaue = 1;
}

void StreamView::stop() {
    RS_AUTOLOG();
    mStream.stop();
    if (V4L2_MEMORY_USERPTR == mMemoryType)
        for (RsBuffer rsBuf : mRsBuffers)
            if (nullptr != rsBuf.buffer)
                free(rsBuf.buffer);
    mRsBuffers.clear();
    mRsBuffersPtrs.clear();
}

void StreamView::proccesCaptureResult(uint8_t index)
{
    int i = -1;
    for (i = 0; i < mBuffersCount; i++)
        if (mRsBuffersPtrs[i]->index == index)
            break;

    double min;
    double max;
    cv::Mat map;
    cv::Mat rgbMap;
    cv::Mat colorMap;
    cv::Mat histogramOptimizedMap;
    float scale;
    char* ptr;
    int cnt = 0;
    int flag = 1;
    char* left;
    char* right;
    char image[mFormat.bytesperline * mFormat.height];
    //lock_guard<mutex> lock(mMutex);
    switch(mStreamType) {
    case V4L2Utils::StreamUtils::StreamType::RS_DEPTH_STREAM:
        map = cv::Mat(mFormat.height, mFormat.width, CV_16UC1, (void*)mRsBuffersPtrs[i]->buffer, mFormat.bytesperline);
        cv::minMaxIdx(map, &min, &max);
        scale = 255 / (max-min);
        map.convertTo(histogramOptimizedMap, CV_8UC1, scale, -min*scale);
        cv::applyColorMap(histogramOptimizedMap, colorMap, cv::COLORMAP_JET);
        cv::imshow(mNodeStr.c_str(), colorMap);
        break;
    case V4L2Utils::StreamUtils::StreamType::RS_Y8_STREAM:
        map = cv::Mat(mFormat.height, mFormat.width, CV_8UC1, (void*)mRsBuffersPtrs[i]->buffer, mFormat.bytesperline);
        cv::imshow(mNodeStr.c_str(), map);
        break;
    case V4L2Utils::StreamUtils::StreamType::RS_Y12I_STREAM:
        ptr = (char*)(mRsBuffersPtrs[i]->buffer);
        left = image;
        right = image + mFormat.width;
        for(;ptr < (char*)(mRsBuffersPtrs[i]->buffer) + mRsBuffersPtrs[i]->length - 4; ptr+=4,cnt+=4) {
            *right++ = *ptr;
            *left = *(ptr+1) >> 4;
            *left |= *(ptr+2) << 4;
            left++;
            if(0 == (ptr - (char*)(mRsBuffersPtrs[i]->buffer)) % (mFormat.width*4)) {
                if(cnt == mFormat.width*4){
                    cnt = 0;
                    left += mFormat.width;
                    right += mFormat.width;
                }
            }
        }

        map = cv::Mat(mFormat.height, mFormat.width*2, CV_8UC1, image);
        //map = cv::Mat(mFormat.height, mFormat.width, CV_8UC1, image);
        cv::imshow(mNodeStr.c_str(), map);
        break;
    case V4L2Utils::StreamUtils::StreamType::RS_RGB_STREAM:
        map = cv::Mat(mFormat.height, mFormat.width, CV_8UC2, (void*)mRsBuffersPtrs[i]->buffer, mFormat.bytesperline);
        cv::cvtColor(map, rgbMap, COLOR_YUV2BGR_UYVY);
        cv::imshow(mNodeStr.c_str(), rgbMap);
        break;
    default:
        break;
    }

    waitKey(1);

    //mStream.proccesCaptureRequest(mRsBuffersPtrs[i]);
}

int StreamView::setAE(bool value) {
    RS_AUTOLOG();
    mStream.setAE(value);
}

int StreamView::setExposure(int value) {
    RS_AUTOLOG();
    mStream.setExposure(value);
}

int StreamView::setLaserMode(bool value) {
    RS_AUTOLOG();
    mStream.setLaserMode(value);
}

int StreamView::setLaserValue(int value) {
    RS_AUTOLOG();
    mStream.setLaserValue(value);
}

int StreamView::setFPS(int value) {
    RS_AUTOLOG();
    // TODO: check valid values?
    // New FPS will take effect in the next stream start
    //if (5 == value || 30 == value)
        mFormat.fps = value;
}

int StreamView::setResolution(uint32_t width, uint32_t height)
{
    mFormat.width = width;
    mFormat.height = height;
}

int StreamView::setSlaveMode(bool value){
    // New slave mode will take effect in the next stream start
    mSlaveMode = value;
}


} // namespace camera_viewer
} // namespace realsense
