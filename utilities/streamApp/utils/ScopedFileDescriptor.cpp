/*
 * ScopedFileDescriptor.cpp
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

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "ScopedFileDescriptor.h"
#include "RealsenseLog.h"

using namespace std;

namespace realsense {
namespace utils {

static std::string videoNodePathPrefix {"/dev/video"};

ScopedFileDescriptor::ScopedFileDescriptor(unsigned nodeNum) {

    RS_AUTOLOG();

    string videoNode(utils::videoNodePathPrefix);
    videoNode += to_string(nodeNum);

    mScopedFileDescriptor = open(videoNode.c_str(), O_RDWR);
    if (mScopedFileDescriptor < 0) {
        RS_LOGE("Failed opening %s, errno = %d",videoNode.c_str(), errno);
    }
}

ScopedFileDescriptor::~ScopedFileDescriptor() {

    RS_AUTOLOG();

    if(mScopedFileDescriptor != -1) {
        int res = close(mScopedFileDescriptor);
        if (res != 0) {
            RS_LOGE("Failed closing %d, errno = ", mScopedFileDescriptor, errno);
        } else {
            RS_LOGV("%d closed", mScopedFileDescriptor);
        }
    }
}

int ScopedFileDescriptor::get() const {

    RS_AUTOLOG();

    return mScopedFileDescriptor;
}

ScopedFileDescriptor::operator bool() const {

    RS_AUTOLOG();

    return (mScopedFileDescriptor > 0);
}

} // namespace utils
} // namespace realsense
