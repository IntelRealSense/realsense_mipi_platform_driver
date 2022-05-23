/*
 * Stream.cpp
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
#include <iostream>
#include <chrono>
using namespace std::chrono_literals;

#include <string.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/eventfd.h>

#include <linux/v4l2-subdev.h>

#include "Stream.h"
#include "RealsenseLog.h"
#include "ScopedFileDescriptor.h"

using namespace std;
using namespace realsense::utils;

namespace realsense {
namespace camera_sub_system {

// TODO: const
#define DS5_STREAM_CONFIG_0                  0x4000
#define DS5_CAMERA_CID_BASE                 (V4L2_CTRL_CLASS_CAMERA | DS5_STREAM_CONFIG_0)
#define DS5_CAMERA_CID_LASER_POWER          (DS5_CAMERA_CID_BASE+1)
#define DS5_CAMERA_CID_MANUAL_LASER_POWER   (DS5_CAMERA_CID_BASE+2)
#define DS5_CAMERA_CID_HWMC                 (DS5_CAMERA_CID_BASE+15)

Stream::Stream(uint8_t nodeNumber) : mNodeNumber{nodeNumber}
{
    RS_AUTOLOG();
    RS_LOGI("node number %d", mNodeNumber);

    int res = initCapabilities();
    if (res < 0)
        throw runtime_error("initCapabilities failed");
}

Stream::~Stream()
{
}

int Stream::initCapabilities()
{
    RS_AUTOLOG();

    return enumerateFormats();
}

int Stream::enumerateFormats()
{
    RS_AUTOLOG();

    v4l2_fmtdesc fmt {};
    fmt.index = 0;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ScopedFileDescriptor fd(mNodeNumber);
    if (!fd)
        return -1;

    int res = 0;
    while (true) {
        res = ioctl(fd.get(), VIDIOC_ENUM_FMT, &fmt);
        if (res != 0) {
            if (res == -1 && errno == EINVAL) {
                // if reached here, this means we done enumerating formats.
                res = 0;
            } else {
                RS_LOGE("VIDIOC_ENUM_FMT failed, res = %d, errno = %d", res, errno);
            }
            break;
        }
        fmt.index++;

        // TODO: I get format 0 for DS5U cameras. Is this bug?
        if (0 == fmt.pixelformat)
            continue;

        list<pair<uint32_t, uint32_t>> frameSizesList;
        bool wasInserted = mFormatsMap.insert({fmt.pixelformat, frameSizesList}).second;
        if (!wasInserted) {
            RS_LOGW("Format %s already exists", fourCCToString(fmt.pixelformat).c_str());
        }

        RS_LOGW("Format %s ", fourCCToString(fmt.pixelformat).c_str());

        //string strFormat = fourCCToString(fmt.pixelformat);
        //if (std::string::npos != strFormat.find("Z16"))
        res = enumFrameSizes(fd.get(), fmt.pixelformat);
        if (res != 0)
            break;
    }

    return res;
}

int Stream::enumFrameSizes(unsigned int fd, uint32_t format)
{
    RS_AUTOLOG();

    v4l2_frmsizeenum frmSizeEnum;
    frmSizeEnum.index = 0;
    frmSizeEnum.pixel_format = format;

    int res = ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmSizeEnum);
    if (res != 0) {
        RS_LOGE("VIDIOC_ENUM_FRAMESIZES failed, res = %d, errno = %d", res, errno);
        return res;
    }

    switch (frmSizeEnum.type ) {
    case V4L2_FRMSIZE_TYPE_DISCRETE:
    {
        do {
            list<pair<uint32_t, uint32_t>> &frameSizesList = mFormatsMap.find(format)->second;
            frameSizesList.push_back(make_pair(static_cast<uint32_t>(frmSizeEnum.discrete.width),
                    static_cast<uint32_t>(frmSizeEnum.discrete.height)));

            int res = enumFrameIntervals(fd, format,
                    frmSizeEnum.discrete.width,
                    frmSizeEnum.discrete.height);
            if (0 != res) {
                RS_LOGE("enumFrameIntervals failed for format %s, framesize = %dx%d, res = %d, errno = %d",
                        fourCCToString(format).c_str(),
                        frmSizeEnum.discrete.width,
                        frmSizeEnum.discrete.height,
                        res,
                        errno);

                return res;
            }

            // Only V4L2_FRMSIZE_TYPE_DISCRETE should increase index.
            frmSizeEnum.index++;

            res = ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmSizeEnum);
            if (res != 0) {
                if (res == -1 && errno == EINVAL) {
                    // if reached here, this means we done enumerating formats.
                    res = 0;
                } else {
                    RS_LOGE("VIDIOC_ENUM_FRAMESIZES failed, res = %d, errno = %d", res, errno);
                }
                break;
            }
        } while (true);

        break;
    }
    // TODO: handle other types:
    case V4L2_FRMSIZE_TYPE_STEPWISE:
    {
        RS_LOGW("Non handled frame size type %x", frmSizeEnum.type);
        break;
    }
    default:
        RS_LOGE("Un defined frame size type %x", frmSizeEnum.type);
    }

    return res;
}

// currently we only print FPS values, they are not stored in data structure.
int Stream::enumFrameIntervals(unsigned int fd, uint32_t format, uint32_t width, uint32_t height)
{
    RS_AUTOLOG();

    v4l2_frmivalenum frmIntervals;
    frmIntervals.index = 0;
    frmIntervals.pixel_format = format;
    frmIntervals.width = width;
    frmIntervals.height = height;

    while (true) {
        int res = ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmIntervals);
        if (0 != res) {
            return 0;
        }

        if (V4L2_FRMIVAL_TYPE_DISCRETE == frmIntervals.type) {
            RS_LOGD("            frame numerator  : %d", frmIntervals.discrete.numerator);
            RS_LOGD("            fps: %d", frmIntervals.discrete.denominator);
        }

        frmIntervals.index++;
    }

    return 0;
}

std::string Stream::fourCCToString(unsigned int val) const
{
    RS_AUTOLOG();

    // The inverse of
    //     #define v4l2_fourcc(a,b,c,d)
    //     (((__u32)(a)<<0)|((__u32)(b)<<8)|((__u32)(c)<<16)|((__u32)(d)<<24))
    std::string s;

    s += val & 0x7f;
    s += (val >> 8) & 0x7f;
    s += (val >> 16) & 0x7f;
    s += (val >> 24) & 0x7f;
    if (val & (1 << 31))
        s += "-BE";
    return s;
}

void Stream::dump() const
{
    // mFormatsMap is an unordered map of formats and frame sizes
    // The keys are the supported formats
    // The values are lists of frame sizes.
    const char* space = " ";
    for (auto itr : mFormatsMap) {
        uint32_t format = itr.first;
        const list<pair<uint32_t, uint32_t>> &frameSizesList = mFormatsMap.find(format)->second;
        for (auto pair : frameSizesList) {
            uint32_t width = pair.first;
            uint32_t height = pair.second;
            RS_LOGI("%24s %s : %dx%d", space, fourCCToString(format).c_str(), width, height);

        }
        RS_LOGI("");
    }
}

int Stream::configure(realsense::camera_sub_system::Format format)
{
    RS_AUTOLOG();

    ScopedFileDescriptor fd(mNodeNumber);
    if (!fd) {
        RS_LOGE("ScopedFileDescriptor failed");
        return -1;
    }

    mFormat = format;

    struct v4l2_format v4l2Format;
    v4l2Format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2Format.fmt.pix.pixelformat = mFormat.v4l2Format;
    v4l2Format.fmt.pix.width = mFormat.width;
    v4l2Format.fmt.pix.height = mFormat.height;
    int ret = ioctl(fd.get(), VIDIOC_S_FMT, &v4l2Format);
    if (ret < 0) {
        RS_LOGE("VIDIOC_S_FMT failed, errno %d", errno);
        return -1;
    } else {
        if (v4l2Format.fmt.pix.pixelformat != mFormat.v4l2Format ||
            v4l2Format.fmt.pix.width != mFormat.width ||
            v4l2Format.fmt.pix.height != mFormat.height) {
            RS_LOGE("VIDIOC_S_FMT fails to set to %s width(%d) height(%d) done",
                    fourCCToString(mFormat.v4l2Format).c_str(),
                    mFormat.width,
                    mFormat.height);
            return -1;
        }
        RS_LOGI("VIDIOC_S_FMT to %s width(%d) height(%d) done",
                fourCCToString(mFormat.v4l2Format).c_str(),
                mFormat.width,
                mFormat.height);
    }

    if (mFormat.fps == 0) {
        ret = getFPS(&mFormat.fps);
        if (ret < 0)
            return -1;
    } else {
        ret = setFPS(mFormat.fps);
        if (ret < 0)
            return -1;
    }

    return ret;
}

int Stream::start(vector<RsBuffer*> rsBuffers, uint32_t memoryType)
{
    RS_AUTOLOG();

    lock_guard<mutex> lock(mStreamingMutex);
    if (mIsStreaming) {
        RS_LOGW("Already streaming");
        return 0;
    }

    string vn {"/dev/video"};
    vn += to_string(mNodeNumber);
    mFileDescriptor = open(vn.c_str(), O_RDWR);
    if (mFileDescriptor < 0) {
        RS_LOGE("open %s failed, errno %d", vn.c_str(), errno);
        return -1;
    }

    mMemoryType = memoryType;

    // VIDIOC_REQBUFS
    struct v4l2_requestbuffers v4L2ReqBufferrs {0};
    v4L2ReqBufferrs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4L2ReqBufferrs.memory = memoryType;
    v4L2ReqBufferrs.count = rsBuffers.size();
    int ret = ioctl(mFileDescriptor, VIDIOC_REQBUFS, &v4L2ReqBufferrs);
    if (ret < 0) {
        RS_LOGE("VIDIOC_REQBUFS failed, errno %d", errno);
        close(mFileDescriptor);
        return -1;
    }
    if (v4L2ReqBufferrs.count < rsBuffers.size()) {
        RS_LOGE("VIDIOC_REQBUFS allocated only %d, requested size is %d",
                v4L2ReqBufferrs.count, rsBuffers.size());
        return -1;
    }

    for (unsigned int i = 0; i < rsBuffers.size(); ++i) {
        mV4l2Buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mV4l2Buffer.memory = memoryType;
        mV4l2Buffer.index = i;
        ret = ioctl(mFileDescriptor, VIDIOC_QUERYBUF, &mV4l2Buffer);
        if (ret < 0) {
            RS_LOGE("VIDIOC_REQBUFS failed, errno %d", errno);
            close(mFileDescriptor);
            return -1;
        }
    }

    switch (memoryType) {
    case V4L2_MEMORY_MMAP:
        ret = initMmap(rsBuffers);
        break;
    case V4L2_MEMORY_USERPTR:
        break;
    default:
        RS_LOGE("Non supported memory type");
        close(mFileDescriptor);
        return -1;
    }

    // Queue buffers
    for (RsBuffer *rsBuffer : rsBuffers) {

        mRsBuffersPtrs.push_back(rsBuffer); // TODO: maybe not needed

        mV4l2Buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mV4l2Buffer.memory = memoryType;
        mV4l2Buffer.index = rsBuffer->index;
        if (V4L2_MEMORY_USERPTR == memoryType) {
            mV4l2Buffer.m.userptr = (unsigned long long)rsBuffer->buffer;
            mV4l2Buffer.length = rsBuffer->length;
        }

        int ret = ioctl(mFileDescriptor, VIDIOC_QBUF, &mV4l2Buffer);
        if (ret < 0) {
            RS_LOGE("VIDIOC_QBUF failed for buffer %d, errno %d", mV4l2Buffer.index, errno);
            close(mFileDescriptor);
            return -1;
        }
    }

    // initialize event notification object for stopping the thread loop
    mStopEventFD = eventfd(0, 0);
    if (mStopEventFD < 0) {
        RS_LOGE("eventfd failed with errno %d", errno);
        close(mFileDescriptor);
        return -1;
    }

    try {
        mStreamingThread = std::thread(&Stream::streamingThreadLoop, this);
    } catch (...) {
        RS_LOGE("Failed to start streaming thread");
        close(mStopEventFD);
        close(mFileDescriptor);
        return -1;
    }

    // VIDIOC_STREAMON
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(mFileDescriptor, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        RS_LOGE("VIDIOC_STREAMON failed, errno %d", errno);
        cleanup();
        close(mStopEventFD);
        close(mFileDescriptor);
        return -1;
    }

    mIsStreaming = true;
    return 0;
}

int Stream::cleanup()
{
    // write an event to stop the streaming thread loop
    uint64_t stop {StopEventFdValue};
    ssize_t size = write(mStopEventFD, &stop, sizeof(uint64_t));
    if (size != sizeof(uint64_t)) {
        RS_LOGE("Failed to stop streaming thread %d", errno);
    }

    // a stop event was sent. wait for the streaming thread to end.
    if (mStreamingThread.joinable()) {
        mStreamingThread.join();
    } else {
        RS_LOGE("Failed joining streaming thread.");
    }

    if (V4L2_MEMORY_MMAP == mMemoryType)
        unInitMmap();

    return 0;
}

int Stream::stop()
{
    int ret;
    RS_AUTOLOG();

    lock_guard<mutex> lock(mStreamingMutex);

    if (!mIsStreaming) {
        RS_LOGW("Stream is stopped");
        return 0;
    }

    mIsStreaming =false;

    enum v4l2_buf_type type {V4L2_BUF_TYPE_VIDEO_CAPTURE};
    ret = ioctl(mFileDescriptor, VIDIOC_STREAMOFF, &type);
    if (ret < 0)
        RS_LOGE("VIDIOC_STREAMOFF failed, errno %d", errno);

    ret = cleanup();

    close(mFileDescriptor);
    close(mStopEventFD);

    return ret;
}

int Stream::setFPS(uint32_t value)
{
    ScopedFileDescriptor fd(mNodeNumber);
    if (!fd) {
        RS_LOGE("ScopedFileDescriptor failed");
        return -1;
    }

    struct v4l2_streamparm setFps {0};
    setFps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    setFps.parm.capture.timeperframe.numerator= 1;
    setFps.parm.capture.timeperframe.denominator = value;

    int ret = ioctl(fd.get(), VIDIOC_S_PARM, &setFps);
    if (ret < 0) {
        RS_LOGE("VIDIOC_S_PARM failed, errno %d", errno);
        return -1;
    } else {
        mFormat.fps = value;
        RS_LOGI("FPS set to to %d ", mFormat.fps);
    }
    return 0;
}

int Stream::getFPS(uint32_t* value)
{
    ScopedFileDescriptor fd(mNodeNumber);
    if (!fd) {
        RS_LOGE("ScopedFileDescriptor failed");
        return -1;
    }

    struct v4l2_streamparm getFps {0};
    getFps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    int ret = ioctl(fd.get(), VIDIOC_G_PARM, &getFps);
    if (ret < 0) {
        RS_LOGE("VIDIOC_G_PARM failed, errno %d", errno);
        return -1;
    } else {
        *value = getFps.parm.capture.timeperframe.denominator / getFps.parm.capture.timeperframe.numerator;
        RS_LOGI("FPS get %d ", *value);
        if (*value != mFormat.fps) {
            mFormat.fps = *value;
        }
    }
    return 0;
}

int Stream::setAE (bool value)
{
    struct v4l2_ext_control aeModeCtrl {0};
    aeModeCtrl.id = V4L2_CID_EXPOSURE_AUTO;
    aeModeCtrl.size = 0;
    aeModeCtrl.value = value ? V4L2_EXPOSURE_APERTURE_PRIORITY : V4L2_EXPOSURE_MANUAL;

    // set auto exposure mode
    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &aeModeCtrl;
    ext.count = 1;

    ScopedFileDescriptor fd(mNodeNumber);
    if (!fd)
       return -1;

    if (ioctl(fd.get(), VIDIOC_S_EXT_CTRLS, &ext) < 0) {
        RS_LOGE("VIDIOC_S_EXT_CTRLS AE failed with %d", errno);
        return -1;
    }

    return 0;
}

int Stream::getAE (bool* value)
{
    struct v4l2_ext_control aeModeCtrl {0};
    aeModeCtrl.id = V4L2_CID_EXPOSURE_AUTO;
    aeModeCtrl.size = 0;

    // get auto exposure mode
    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &aeModeCtrl;
    ext.count = 1;

    ScopedFileDescriptor fd(mNodeNumber);
    if (!fd)
       return -1;

    if (ioctl(fd.get(), VIDIOC_G_EXT_CTRLS, &ext) < 0) {
        RS_LOGE("VIDIOC_G_EXT_CTRLS AE failed with %d", errno);
        return -1;
    }

    *value = (aeModeCtrl.value != V4L2_EXPOSURE_MANUAL) ? true : false;

    return 0;
}

int Stream::setExposure (int value) {
    // set auto exposure mode off
    struct v4l2_ext_control aeModeCtrl {0};
    aeModeCtrl.id = V4L2_CID_EXPOSURE_AUTO;
    aeModeCtrl.size = 0;
    aeModeCtrl.value = V4L2_EXPOSURE_MANUAL;

    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &aeModeCtrl;
    ext.count = 1;

    ScopedFileDescriptor fd(mNodeNumber);
    if (!fd)
       return -1;

    int ret = ioctl(fd.get(), VIDIOC_S_EXT_CTRLS, &ext);
    if (ret < 0)
        RS_LOGE("VIDIOC_S_EXT_CTRLS AE failed with %d", errno);

    // set exposure
    struct v4l2_ext_control expCtrl {0};
    expCtrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
    expCtrl.size = 0;
    expCtrl.value = value;

    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.count = 1;
    ext.controls = &expCtrl;
    ret = ioctl(fd.get(), VIDIOC_S_EXT_CTRLS, &ext);
    if (ret < 0)
        RS_LOGE("VIDIOC_S_EXT_CTRLS exposure failed with %d", errno);

    return ret;
}

int Stream::getExposure (int* value) {
    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.count = 1;

    ScopedFileDescriptor fd(mNodeNumber);
    if (!fd)
       return -1;

    // get exposure
    struct v4l2_ext_control expCtrl {0};
    expCtrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
    expCtrl.size = 0;

    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.count = 1;
    ext.controls = &expCtrl;
    int ret = ioctl(fd.get(), VIDIOC_G_EXT_CTRLS, &ext);
    if (ret < 0)
        RS_LOGE("VIDIOC_G_EXT_CTRLS exposure failed with %d", errno);

    *value = expCtrl.value;

    return ret;
}

int Stream::setLaserMode (bool value)
{
    struct v4l2_ext_control laserCtrl {0};
    laserCtrl.id = DS5_CAMERA_CID_LASER_POWER;
    laserCtrl.size = 0;
    laserCtrl.value = value;

    // set laser power
    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &laserCtrl;
    ext.count = 1;

    ScopedFileDescriptor fd(mNodeNumber);
    if (!fd)
       return -1;

    int ret = ioctl(fd.get(), VIDIOC_S_EXT_CTRLS, &ext);
    if (ret < 0)
        RS_LOGE("VIDIOC_S_EXT_CTRLS laser mode failed with %d", errno);

    return ret;
}

int Stream::getLaserMode (bool* value)
{
    if (nullptr == value)
        return -1;

    struct v4l2_ext_control laserCtrl {0};
    laserCtrl.id = DS5_CAMERA_CID_LASER_POWER;
    laserCtrl.size = 0;

    // get laser power
    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &laserCtrl;
    ext.count = 1;

    ScopedFileDescriptor fd(mNodeNumber);
    if (!fd)
       return -1;

    int ret = ioctl(fd.get(), VIDIOC_G_EXT_CTRLS, &ext);
    if (ret < 0)
        RS_LOGE("VIDIOC_G_EXT_CTRLS laser mode failed with %d", errno);

    *value = static_cast<bool>(laserCtrl.value);

    return ret;
}


int Stream::setLaserValue (int value)
{
    struct v4l2_ext_control manualLaserCtrl {0};
    manualLaserCtrl.id = DS5_CAMERA_CID_MANUAL_LASER_POWER;
    manualLaserCtrl.size = 0;

    RS_LOGI("setting laser power to %d", value);
    manualLaserCtrl.value = value;

    // set laser power
    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &manualLaserCtrl;
    ext.count = 1;

    ScopedFileDescriptor fd(mNodeNumber);
    if (!fd)
            return -1;
    int ret = ioctl(fd.get(), VIDIOC_S_EXT_CTRLS, &ext);
    if (ret < 0)
        RS_LOGE("VIDIOC_S_EXT_CTRLS laser value failed with %d", errno);

	// get laser power
    struct v4l2_ext_control currManualLaserCtrl {0};
    currManualLaserCtrl.id = DS5_CAMERA_CID_MANUAL_LASER_POWER;
    ext.controls = &currManualLaserCtrl;
    ext.count = 1;
    ret = ioctl(fd.get(), VIDIOC_G_EXT_CTRLS, &ext);

    if (currManualLaserCtrl.value != manualLaserCtrl.value)
    RS_LOGE("VIDIOC_G_EXT_CTRLS laser value failed with %d", errno);

    return ret;
}

int Stream::getLaserValue (int* value)
{

    struct v4l2_ext_control manualLaserCtrl {0};
    manualLaserCtrl.id = DS5_CAMERA_CID_MANUAL_LASER_POWER;
    manualLaserCtrl.size = 0;

    RS_LOGI("getting laser power");

    // get laser power
    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &manualLaserCtrl;
    ext.count = 1;

    ScopedFileDescriptor fd(mNodeNumber);
    if (!fd)
            return -1;
    int ret = ioctl(fd.get(), VIDIOC_G_EXT_CTRLS, &ext);
    if (ret < 0)
            RS_LOGE("VIDIOC_G_EXT_CTRLS laser value failed with %d", errno);

    *value = manualLaserCtrl.value;

    return ret;
}

struct HWMC {
    uint16_t header;
    uint16_t magic_word;
    uint32_t opcode;
    uint32_t params[4];
};

int Stream::setSlaveMode (bool value)
{
    uint8_t hwmc[1028] {0};

    struct v4l2_ext_control ctrl {0};
    ctrl.id = DS5_CAMERA_CID_HWMC;
    ctrl.size = sizeof(hwmc);
    ctrl.p_u8 = hwmc;

    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &ctrl;
    ext.count = 1;

    // Get org depth calibration table
    HWMC slaveMode {0};
    slaveMode.header = 0x14;
    slaveMode.magic_word = 0xCDAB;
    slaveMode.opcode = 0x69;
    if (value)
        slaveMode.params[0] = 3;
    else
        slaveMode.params[0] = 0;

    memcpy(hwmc, &slaveMode, sizeof(struct HWMC));

    ScopedFileDescriptor fd(mNodeNumber);
    if (!fd)
       return -1;

    int ret = ioctl(fd.get(), VIDIOC_S_EXT_CTRLS, &ext);
    if (ret < 0)
        RS_LOGE("VIDIOC_S_EXT_CTRLS slave mode failed with %d", errno);

    return ret;
}

int Stream::initMmap(vector<RsBuffer*> rsBuffers)
{
    RS_AUTOLOG();

    // Map buffers
    for (RsBuffer *rsBuffer : rsBuffers) {
        mV4l2Buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mV4l2Buffer.memory = V4L2_MEMORY_MMAP;
        mV4l2Buffer.index = rsBuffer->index;
        int ret = ioctl(mFileDescriptor, VIDIOC_QUERYBUF, &mV4l2Buffer);
        if (ret < 0) {
            RS_LOGE("VIDIOC_QUERYBUF failed with errno %d", errno);
            return -1;
        }

        rsBuffer->buffer = (struct buffer*)mmap(nullptr,
                                                mV4l2Buffer.length,
                                                PROT_READ | PROT_WRITE,
                                                MAP_SHARED,
                                                mFileDescriptor,
                                                mV4l2Buffer.m.offset);
        RS_LOGI("mmap retruned %p errno %d", rsBuffer->buffer, errno);
        memset(rsBuffer->buffer, 0, mV4l2Buffer.length);
    }
    return 0;
}

int Stream::unInitMmap()
{
    RS_AUTOLOG();

    for (RsBuffer *rsBuffer : mRsBuffersPtrs) {
        munmap(rsBuffer->buffer, rsBuffer->length);
    }

    mRsBuffersPtrs.clear();
    return 0;
}

void Stream::streamingThreadLoop()
{
    RS_AUTOLOG();

    // First file descriptor receives stop events.
    // Second file descriptor receives buffer ready events.
    struct pollfd pollFDs[2] = {
        {
            .fd = mStopEventFD,
            .events = POLLIN,
        }, {
            .fd = mFileDescriptor,
            .events = POLLIN | POLLERR,
        },
    };

    string vn {"/dev/video"};
    vn += to_string(mNodeNumber);

    // run endless loop till we're signaled to stop via eventfd
    while (true) {
        // poll on buffers ready file descriptor to get
        // events related ready buffers
        // poll on the events file descriptor to know
        // when to stop the thread loop
        if (poll(pollFDs, 2, -1) < 0) { // TODO: change -1 to FPS and handle frame drops
            if (errno == EINTR)
                continue;

            // poll interrupted and we need to stop loop
            break;
        }

        // First file descriptor listens to stop events.
        auto stopEvents = pollFDs[0].revents;
        // Second file descriptor listens to buffers ready events.
        auto bufferReadyEvents = pollFDs[1].revents;

        if (stopEvents & POLLIN) {
            if (isStopEvent())
                break;
        } else if (bufferReadyEvents & POLLIN) {
            mV4l2Buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            mV4l2Buffer.memory = mMemoryType;
            int ret = ioctl(mFileDescriptor, VIDIOC_DQBUF, &mV4l2Buffer);
            if (ret < 0) {
                RS_LOGE("VIDIOC_DQBUF failed %d", errno);
                ret = ioctl(mFileDescriptor, VIDIOC_QBUF, &mV4l2Buffer);
                if (ret < 0)
                    RS_LOGE("VIDIOC_QBUF after DQBUF failure failed %d", errno);
            } else {
                {
                    std::lock_guard<std::mutex> lock(mDisplayMutex);
                    mDisplayIndex.push_back(mV4l2Buffer.index);
                }
                mDisplayCV.notify_one();
            }
        } else {
            RS_LOGE("poll error, %d", errno);
            this_thread::sleep_for(5ms);
            //break;
        }
    }

    // thread loop done, notify we're stopped (stop method is waiting)
    RS_LOGI("streamingThreadLoop done...");
}

bool Stream::isStopEvent()
{
    RS_AUTOLOG();
    uint64_t value = 0;
    ssize_t size =  read(mStopEventFD, static_cast<void *>(&value), sizeof(uint64_t));
    if (sizeof(uint64_t) == size && StopEventFdValue == value) {
        RS_LOGI("Stopping streaming thread");
        return true;
    } else {
        RS_LOGW("Failed to stop streaming thread, size(%d), value(%x)", size, value);
    }
    return false;
}

uint32_t Stream::getBufferIndex()
{
    std::unique_lock<std::mutex> lock(mDisplayMutex);
    mDisplayCV.wait_for(lock, 5ms, [&]{ return !mDisplayIndex.empty(); });
    if (mDisplayIndex.empty())
        return UINT32_MAX;
    else
        return mDisplayIndex.front();
}

void Stream::returnBufferIndex()
{
    std::lock_guard<std::mutex> lock(mDisplayMutex);
    auto index = mDisplayIndex.front();
    mDisplayIndex.pop_front();

    struct v4l2_buffer buf {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = mMemoryType;
    buf.index = index;
    int ret = ioctl(mFileDescriptor, VIDIOC_QBUF, &buf);
    if (ret < 0) {
        RS_LOGE("VIDIOC_QBUF failed %d", errno);
        return;
    }
}

} // namespace camera_sub_system
} // namespace realsense
