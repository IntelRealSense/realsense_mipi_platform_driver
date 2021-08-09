/*
 * RealsenseAutoLogger.h
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

#ifndef __REALSENSE_AUTO_LOGGER_H__
#define __REALSENSE_AUTO_LOGGER_H__
#include <chrono>

namespace realsense {
namespace utils {

/**
 * RealsenseAutoLogger is helper class that enables auto logging at functions
 * entry/exit.
 *
 * This class shouldn't be used directly.
 * For logging purposes, use AUTOLOG defined in "RealsensLog.h".
 *
 */
class RealsenseAutoLogger {
public:
    /**
     * Constructor
     *
     * @param level: logging level
     * @param func: caller function name
     * @param line: caller line number
     * @param fmt: log message formatting string
     *
    **/
    RealsenseAutoLogger(const int level,
            const char *func,
            const int line,
            const char *fmt,
            ...);

    /**
     * Destructor
     **/
    ~RealsenseAutoLogger();

private:
    const int mLevel;
    const char *mFunc;
    const int mLineNumber;
    const char *mFormat;
    std::chrono::time_point<std::chrono::high_resolution_clock>  mStartTime;

    RealsenseAutoLogger(const RealsenseAutoLogger&) = delete;
    RealsenseAutoLogger& operator =(const RealsenseAutoLogger&) = delete;

}; // class RealsenseAutoLogger

} // namespace utils
} // namespace realsense

#endif // __REALSENSE_AUTO_LOGGER__
