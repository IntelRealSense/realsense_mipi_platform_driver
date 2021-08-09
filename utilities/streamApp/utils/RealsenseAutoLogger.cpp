/*
 * RealsenseAutoLogger.cpp
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
#include "RealsenseLogger.h"
#include "RealsenseAutoLogger.h"

#include <string>

using namespace std;

namespace realsense {
namespace utils {

RealsenseAutoLogger::RealsenseAutoLogger(const int level,
        const char *func,
        const int lineNumber,
        const char *fmt,
        ...)
: mLevel {level}, mFunc {func}, mLineNumber {lineNumber}, mFormat {fmt} {
    mStartTime = chrono::high_resolution_clock::now();
    va_list args;
    va_start(args, fmt);
    const string enterPrefix {"[enter]"};
    RealsenseLogger::log(mLevel, mFunc, mLineNumber, enterPrefix.c_str(), mFormat, args);
    va_end(args);
}

RealsenseAutoLogger::~RealsenseAutoLogger() {
    // This is for time profiling, it will calc the time spent in each function
    auto endTime = chrono::high_resolution_clock::now();
    chrono::duration<float> fs = endTime - mStartTime;
    chrono::milliseconds duration = std::chrono::duration_cast<chrono::milliseconds>(fs);

    va_list args;
    const string exitPrefix {"[exit] - " + to_string(duration.count()) + "ms"};
    RealsenseLogger::log(mLevel, mFunc, mLineNumber, exitPrefix.c_str(), mFormat, args);
}

} // namespace utils
} // namespace realsense
