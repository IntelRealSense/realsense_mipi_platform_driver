/*
 * RealsenseLog.h
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

#ifndef __REALSENSE_LOG_H__
#define __REALSENSE_LOG_H__


#include "RealsenseLogger.h"
#include "RealsenseAutoLogger.h"

namespace realsense {
namespace utils {

#undef RSAUTOLOG
#define RSAUTOLOG(fmt, ...) \
    RealsenseAutoLogger autoLogger(RealsenseLogger::RS_LOG_INFO, __PRETTY_FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \

#undef RS_AUTOLOG
#define RS_AUTOLOG(...) RSAUTOLOG("", ##__VA_ARGS__)

#undef RSLOGx
#define RSLOGx(level, fmt, ...) \
{\
    RealsenseLogger::log(level, __PRETTY_FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
}

#undef RS_LOGV
#define RS_LOGV(fmt, ...) \
    RSLOGx(RealsenseLogger::RS_LOG_VERBOSE, fmt, ##__VA_ARGS__)

#undef RS_LOGD
#define RS_LOGD(fmt, ...) \
    RSLOGx(RealsenseLogger::RS_LOG_DEBUG, fmt, ##__VA_ARGS__)

#undef RS_LOGI
#define RS_LOGI(fmt, ...) \
    RSLOGx(RealsenseLogger::RS_LOG_INFO, fmt, ##__VA_ARGS__)

#undef RS_LOGW
#define RS_LOGW(fmt, ...) \
    RSLOGx(RealsenseLogger::RS_LOG_WARN, fmt, ##__VA_ARGS__)

#undef RS_LOGE
#define RS_LOGE(fmt, ...) \
    RSLOGx(RealsenseLogger::RS_LOG_ERROR, fmt, ##__VA_ARGS__)

} // namespace utils
} // namespace realsense

#endif // __REALSENSE_LOG_H__
