/*
 * RealsenseLogger.h
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

#ifndef __REALSENSE_LOGGER_H__
#define __REALSENSE_LOGGER_H__

#include <cstdarg>

#ifdef __ANDROID__
#include <utils/Log.h>
#endif

namespace realsense {
namespace utils {

/**
 * RealsenseLogger is helper class that wraps the logging mechanism of the OS.
 *
 * For example, in the case of Android, it helps to redefine:
 *     ALOGV,
 *     ALOGD,
 *     ALOGI,
 *     ALOGW,
 *     and ALOGE
 *
 * This class shouldn't be used directly.
 * For logging purposes, use LOGx redefined in "RealsensLog.h".
 */
class RealsenseLogger {
public:

    enum {
        RS_LOG_UNKNOWN = 0,
        RS_LOG_DEFAULT,

        RS_LOG_VERBOSE,
        RS_LOG_DEBUG,
        RS_LOG_INFO,
        RS_LOG_WARN,
        RS_LOG_ERROR,
        RS_LOG_FATAL,

        RS_LOG_SILENT,     /* must be last */
    };

    /**
     * log: Helper logging method. Only used in "RealsenseLog.h"
     *      and "RealsenseAutoLogger.cpp"
     *
     * @param level: logging level
     * @param func:  caller function name
     * @param line:  caller line number
     * @param fmt:   log message formatting string
    **/
    static void log(const int level,
            const char *func,
            const int line,
            const char *fmt,
            ...);

private:

    /**
     * rsVsnprintf: Produces formatted string from variable argument list.
     *
     * @param pdst:   destination buffer pointer
     * @param size:   destination buffer size
     * @param pfmt:   string format
     * @param argptr: variable argument list length
    **/
    static void rsVsnprintf(char* pdst,
            unsigned int size,
            const char* pfmt,
            std::va_list argptr);

    RealsenseLogger(){};
    ~RealsenseLogger(){};
    RealsenseLogger(const RealsenseLogger&) = delete;
    RealsenseLogger& operator =(const RealsenseLogger&) = delete;

}; // class RealsenseLogger

} // namespace utils
} // namespace realsense

#endif // __REALSENSE_LOGGER__
