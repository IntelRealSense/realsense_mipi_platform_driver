/*
 * ScopedFileDescriptor.h
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

#include <mutex>
#include <string>

#ifndef __SCOPED_FILE_DESCRIPTOR_H__
#define __SCOPED_FILE_DESCRIPTOR_H__

namespace realsense {
namespace utils {

/**
 * ScopedFileDescriptor class wraps OS file descriptor for auto open/close operations.
 * File is opened at construction time and closed at destruction time.
 *
 */
class ScopedFileDescriptor {

public:
    /**
     * Constructor:  opens a file
     *
     * @param nodeNum: node number to open
     *
     */
    explicit ScopedFileDescriptor(unsigned nodeNum);

    /**
     * Destructor
     */
    ~ScopedFileDescriptor();

    /**
     * returns the file descriptor
     */
    int get() const;

    /**
     * bool operator
     *
     * @return true if file is successfully opened
     *
     */
    explicit operator bool() const;

private:

    // The file descriptor returned by open() after opening node /dev/video
    int mScopedFileDescriptor {-1};

    ScopedFileDescriptor(const ScopedFileDescriptor&) = delete;
    ScopedFileDescriptor& operator = (const ScopedFileDescriptor&) = delete;

}; // class ScopedFileDescriptor

} // namespace utils
} // namespace realsense

#endif // __SCOPED_FILE_DESCRIPTOR_H__
