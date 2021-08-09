/*
 * Stream.h
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

#ifndef __STREAM_H__
#define __STREAM_H__

#include <list>
#include <mutex>
#include <thread>
#include <functional>
#include <unordered_map>

#include <linux/videodev2.h>

#include "CSSTypes.h"

namespace realsense {
namespace camera_sub_system {

/**
 * Stream class
 *
 * TODO: add documentation, still not clear how to best describe it
 */

class Stream {
public:
    /**
     * Constructor
     *
     * @param nodeNumber streaming node number X in /dev/videoX
     */
    explicit Stream(std::uint8_t nodeNumber);

    /**
     * Destructor
     */
    ~Stream();

    /**
     * dump: This function prints supported formats and frame sizes,
     *       e.g., UYVY : 424x240
     */
    void dump() const;

    /**
     * User must configure Stream format, width, height and fps before
     * starting stream
     */
    int configure(realsense::camera_sub_system::Format);

    /**
     *
     */
    int start(std::vector<realsense::camera_sub_system::RsBuffer*>,
              std::function<void(uint8_t)>,
              uint32_t memoryType);

    /**
     *
     */
    int proccesCaptureRequest(realsense::camera_sub_system::RsBuffer*);

    /**
     *
     */
    int stop();

    /**
     *
     */
    int setAE(bool value);
    int setExposure (int value);
    int setLaserMode (bool value);
    int setLaserValue (int value);

private:
    // The number X of /dev/videoX node that this stream is associated with.
    // This node must be streaming node and not meta data node.
    const std::uint8_t mNodeNumber;

    // Unordered map of key-value that stores the supported formats and
    // resolutions, frames sizes.
    // The keys are the formats
    // The values are list of resolutions, pairs of width and height, that are
    // associated with the format
    std::unordered_map<uint32_t, std::list<std::pair<std::uint32_t, std::uint32_t>>> mFormatsMap;

    int mFileDescriptor {-1};
    std::thread mStreamingThread;
    std::mutex mStreamingMutex {};
    bool mIsStreaming {false};
    std::function<void(uint8_t)> mProccesCaptureResult;
    uint32_t mBufferLength;
    uint32_t mMemoryType;
    struct v4l2_buffer mV4l2Buffer {0};

    /**
     * Need to set format by user before starting stream
     */
    realsense::camera_sub_system::Format mFormat{0,0,0,0};

    // TODO: maybe not needed, saved in StreamView, dirty
    std::vector<realsense::camera_sub_system::RsBuffer*> mRsBuffersPtrs;


    /**
     * use the event file descriptor to stop the streaming thread.
     * @see eventfd
     */
    int mStopEventFD {-1};
    const static uint64_t StopEventFdValue {0xDEAD};

    int initCapabilities();
    int enumerateFormats();
    int enumFrameSizes(unsigned int, std::uint32_t);
    int enumFrameIntervals(unsigned int, std::uint32_t, std::uint32_t, std::uint32_t);
    int initMmap(std::vector<RsBuffer*> rsBuffers);
    int unInitMmap();
    void streamingThreadLoop();
    bool isStopEvent();

    std::string fourCCToString(unsigned int val) const;

    Stream(const Stream&) = delete;
    Stream& operator = (const Stream&) = delete;

}; // class Stream

} // namespace camera_sub_system
} // namespace realsense

#endif // __STREAM_H__
