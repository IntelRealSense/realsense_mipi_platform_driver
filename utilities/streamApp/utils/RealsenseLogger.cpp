/*
 * RealsenseLogger.cpp
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

#include <cstdio>
#include <iostream>

#include "RealsenseLogger.h"

using namespace std;

#undef LOG_TAG
#define LOG_TAG "RS_CSS"

// TODO: remove in release mode
#undef LOG_NDEBUG
#define LOG_NDEBUG 0

namespace realsense {
namespace utils {

void RealsenseLogger::rsVsnprintf(char* pdst,
        unsigned int size,
        const char* pfmt,
        va_list argptr) {

    pdst[0] = '\0';
    int num_chars_written = vsnprintf(pdst, size, pfmt, argptr);

    if ((num_chars_written >= (int)size) && (size > 0)) {
        // Message length exceeds the buffer limit size
        num_chars_written = size - 1;
        pdst[size - 1] = '\0';
    }
}

void RealsenseLogger::log(const int level,
        const char *func,
        const int line,
        const char *fmt,
        ...) {

    const int max_log_buf_size {1024};
    char str_buffer[max_log_buf_size];
    va_list args;

    va_start(args, fmt);
    rsVsnprintf(str_buffer, sizeof(str_buffer), fmt, args);
    va_end(args);

    switch (level) {
    case RS_LOG_VERBOSE:
#ifdef __ANDROID__
        ALOGV(" %s: %d: %s", func, line, str_buffer);
#endif
#ifdef __linux__
        cout <<"[VERBOSE] " << " " << func << ": " << line << ": " << str_buffer << endl;
#endif
        break;
    case RS_LOG_DEBUG:
#ifdef __ANDROID__
        ALOGD(" %s: %d: %s", func, line, str_buffer);
#endif
#ifdef __linux__
        cout <<"[DEBUG] " << " " << func << ": " << line << ": " << str_buffer << endl;
#endif
        break;
    case RS_LOG_INFO:
#ifdef __ANDROID__
        ALOGI(" %s: %d: %s", func, line, str_buffer);
#endif
#ifdef __linux__
        cout <<"[INFO] " << " " << func << ": " << line << ": " << str_buffer << endl;
#endif
        break;
    case RS_LOG_WARN:
#ifdef __ANDROID__
        ALOGW(" %s: %d: %s", func, line, str_buffer);
#endif
#ifdef __linux__
        cout <<"[WARN] " << " " << func << ": " << line << ": " << str_buffer << endl;
#endif
        break;
    case RS_LOG_ERROR:
#ifdef __ANDROID__
        ALOGE(" %s: %d: %s", func, line, str_buffer);
#endif
#ifdef __linux__
        cout <<"[ERROR] " << " " << func << ": " << line << ": " << str_buffer << endl;
#endif
        break;
    default:
#ifdef __ANDROID__
        ALOGD(" %s: %d: %s", func, line, str_buffer);
#endif
#ifdef __linux__
        cout <<"[DEBUG] " << " " << func << ": " << line << ": " << str_buffer << endl;
#endif
    }
}

} // namespace utils
} // namespace realsense
