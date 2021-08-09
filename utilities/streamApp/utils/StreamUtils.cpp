/*
 * StreamUtils.cpp
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

#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "StreamUtils.h"
#include "RealsenseLog.h"
#include "ScopedFileDescriptor.h"

using namespace std;

namespace realsense {
namespace utils {
namespace V4L2Utils {

int StreamUtils::getStreamSupportedFormats(unsigned nodeNum, std::unordered_map<FourCC, std::vector<Resolution>> &formatsMap) {
    RS_AUTOLOG();

    ScopedFileDescriptor fd(nodeNum);
    if (!fd)
        return -1;

    struct v4l2_fmtdesc fmtdesc
    {
        .index = 0,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE
    };

    int ret = 0;
    do {
        ret = TEMP_FAILURE_RETRY(ioctl(fd.get(), VIDIOC_ENUM_FMT, &fmtdesc));
        // check if ioctl succeeded and the format is native to the device
        if (ret == 0 && !(fmtdesc.flags & V4L2_FMT_FLAG_EMULATED)) {

            // add format to map
            std::vector<Resolution> resolutions;
            bool inserted = formatsMap.insert({fmtdesc.pixelformat, resolutions}).second;
            if (!inserted) {
                RS_LOGW("Format %s already exists", fourCCToString(fmtdesc.pixelformat).c_str());
            }

            // loop over frame sizes
            int result = enumFrameSizes(formatsMap, fd.get(), fmtdesc.pixelformat);
            if (result != 0)
                break;
        }
        fmtdesc.index++;
    } while (ret == 0);

    return 0;
}

int StreamUtils::enumFrameSizes(std::unordered_map<FourCC, std::vector<Resolution>> &formatsMap, unsigned int fd, uint32_t format)
{
    RS_AUTOLOG();
    v4l2_frmsizeenum frameSize
    {
        .index = 0,
        .pixel_format = format
    };

    for (; TEMP_FAILURE_RETRY(ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frameSize)) == 0;
                    ++frameSize.index) {
        switch (frameSize.type) {
            case V4L2_FRMSIZE_TYPE_DISCRETE:
            {
                Resolution res = Resolution(frameSize.discrete.width, frameSize.discrete.height);
                auto itr = formatsMap.find(format);
                itr->second.push_back(res);

                // loop over frame sizes
                int ret = enumFrameIntervals(fd, itr);
                if (ret != 0)
                    break;
                break;
            }
            case V4L2_FRMSIZE_TYPE_STEPWISE:
            {
                RS_LOGW("Non handled frame size type %x", frameSize.type);
                break;
            }
            default:
                RS_LOGE("Un defined frame size type %x", frameSize.type);
        }
    }

    return 0;
}

int StreamUtils::enumFrameIntervals(unsigned int fd,
                std::unordered_map<FourCC, std::vector<Resolution>>::iterator itr)
{
    v4l2_frmivalenum frameInterval
    {
        .index = 0,
        .pixel_format = itr->first,
        .width = itr->second.back().getWidth(),
        .height = itr->second.back().getHeight(),
    };

    // add frame rates to format
    for (frameInterval.index = 0;
            TEMP_FAILURE_RETRY(ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frameInterval)) == 0;
            ++frameInterval.index) {
        if (frameInterval.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
            // ignore if frame rate is 0
            if (frameInterval.discrete.numerator != 0) {
                Resolution::FrameRate fr = {
                        frameInterval.discrete.numerator,
                        frameInterval.discrete.denominator
                };
                itr->second.back().addFrameRate(fr);
            }
        }
    }

    return 0;
}

std::string StreamUtils::fourCCToString(unsigned int val)
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

// TODO: add enumFrameSizes static
} // namespace V4L2Utils
} // namespace utils
} // namespace realsense
