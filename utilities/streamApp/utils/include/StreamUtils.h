/*
 * StreamUtils.h
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

#ifndef __STREAM_UTILS_H__
#define __STREAM_UTILS_H__

#include <unordered_map>

#include "CSSTypes.h"

namespace realsense {
namespace utils {
namespace V4L2Utils {

using namespace realsense::camera_sub_system;

class StreamUtils {
public:
    /**
     * enumerates over formats
     */
    static int getStreamSupportedFormats(unsigned nodeNum, std::unordered_map<FourCC, std::vector<Resolution>> &formatsMap);

    /**
     * enumerates over frame sizes
     */
    static int enumFrameSizes(std::unordered_map<FourCC, std::vector<Resolution>> &formatsMap,
                                unsigned int, std::uint32_t);

    /**
     * enumerates over frame rates
     */
    static int enumFrameIntervals(unsigned int fd,
                std::unordered_map<FourCC, std::vector<Resolution>>::iterator itr);

    /**
     * converts fourCC (int) to string
     */
    static std::string fourCCToString(unsigned int val);

    enum class StreamType { RS_DEPTH_STREAM, RS_Y8_STREAM, RS_RGB_STREAM , RS_Y12I_STREAM};

}; // class StreamUtils

} // namespace V4L2Utils
} // namespace utils
} // namespace realsense

#endif // __STREAM_UTILS_H__
