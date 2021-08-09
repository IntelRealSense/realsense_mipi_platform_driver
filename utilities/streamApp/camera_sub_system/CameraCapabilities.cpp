/**
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
 */

#include "CameraCapabilities.h"

namespace realsense {
namespace camera_sub_system {

///////////////////////////////////////////////////////////////////////////////
//   CameraControl Implementation...
///////////////////////////////////////////////////////////////////////////////
CameraControl::range CameraControl::getRange() const {
    return mRange;
}

bool CameraControl::isAsync() const {
    return mInfo.isAsync;
}

bool CameraControl::isCached() const {
    return mInfo.isCached;
}

CameraControl::Mode CameraControl::getMode() const {
    return mInfo.mode;
}

bool CameraControl::range::isValid() const {
    if (min > max || def < min || def > max || res <= 0)
        return false;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
//  CameraCapabilities Implementation...
///////////////////////////////////////////////////////////////////////////////
CameraCapabilities::CameraCapabilities(CameraCapabilitiesMap &&caps):mCapabilitiesMap(caps)
{
}

CameraCapabilities::~CameraCapabilities() {
    mCapabilitiesMap.clear();
}

const CameraControl&
CameraCapabilities::operator[](const std::string &tag) throw(std::invalid_argument) {
    // make sure key exists, if not throw
    auto itr = mCapabilitiesMap.find(tag);
    if (itr == mCapabilitiesMap.end()) {
        throw std::invalid_argument("no capability for: " + tag);
    }

    return itr->second;
}

std::unordered_map<std::string, CameraControl> CameraCapabilities::getControls()
{
    return mCapabilitiesMap;
}

} // namespace camera_sub_system
} // namespace realsense
