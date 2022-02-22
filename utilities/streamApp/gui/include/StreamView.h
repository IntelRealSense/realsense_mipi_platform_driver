/*
 * StreamView.h
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

#ifndef __STREAM_VIEW_H__
#define __STREAM_VIEW_H__

#include <mutex>

#include "CSSTypes.h"
#include "StreamUtils.h"

// Forward declaration from camer_sub_system namespace
namespace realsense {
namespace camera_sub_system {
class Stream;
}
}

namespace realsense {
namespace camera_viewer {

/**
 * StreamView class
 *
 */

class StreamView {
public:
    /**
     * Constructor
     *
     * @param nodeNumber streaming node number X in /dev/videoX
     */
    explicit StreamView(std::uint8_t nodeNumber,
                        realsense::camera_sub_system::Stream &stream,
                        realsense::utils::V4L2Utils::StreamUtils::StreamType st =
                                realsense::utils::V4L2Utils::StreamUtils::StreamType::RS_DEPTH_STREAM);

    /**
     * Destructor
     */
    ~StreamView();

    /**
     *
     */
    void draw();

    /**
     *
     */
    int setFPS(int value);

    /**
     *
     */
    int setResolution(uint32_t width, uint32_t height);

    /**
     *
    */
    int setSlaveMode(bool value);

    // Callback functions to handle GUI events
    // TODO: Consider interface
    static void startStopTrackbarCB (int pos, void *userData);
    static void aETrackbarCB (int pos, void *userData);
    static void exposureTrackbarCB (int pos, void *userData);
    static void laserModeTrackbarCB (int pos, void *userData);
    static void laserValueTrackbarCB (int pos, void *userData);
    static void fpsTrackbarCB (int pos, void *userData);
    static void slaveModeTrackbarCB (int pos, void *userData);

private:

    // The number X of /dev/videoX node that this stream view is associated with.
    // This node must be streaming node and not meta data node.
    const std::uint8_t mNodeNumber;
    std::string mNodeStr {""};
    std::string mCtrlWinName {};
    realsense::utils::V4L2Utils::StreamUtils::StreamType mStreamType {realsense::utils::V4L2Utils::StreamUtils::StreamType::RS_DEPTH_STREAM};
    std::mutex mMutex {};


    realsense::camera_sub_system::Stream &mStream;
    realsense::camera_sub_system::Format mFormat;
    bool mSlaveMode;

    // Number of buffers for streaming
    const std::uint8_t mBuffersCount {4};
    uint32_t mMemoryType;

    std::vector<realsense::camera_sub_system::RsBuffer> mRsBuffers;
    std::vector<realsense::camera_sub_system::RsBuffer*> mRsBuffersPtrs;

    // Track bars GUI values
    int mStartStopTBValue {1};

    void start(uint32_t memoryType);
    void stop();
    int setAE(bool value);
    int setExposure(int value);
    int setLaserMode(bool value);
    int setLaserValue(int value);
    void proccesCaptureResult(uint8_t);

    StreamView(const StreamView&) = delete;
    StreamView& operator = (const StreamView&) = delete;

}; // class StreamView

} // namespace camera_viewer
} // namespace realsense

#endif // __STREAM_VIEW_H__
