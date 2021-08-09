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

#ifndef __CAMERA_CAPABILITIES_H__
#define __CAMERA_CAPABILITIES_H__

#include <unordered_map>
#include <stdexcept>
#include <string>

#include "RealsenseLog.h"

using namespace realsense::utils;

namespace realsense {
namespace camera_sub_system {

/**
 * this struct represents standard camera control capabilities.
 */
struct CameraControl {
    /**
     * specifies the control's supported modes.
     * if for example manual and auto is supported, you
     * need to bit wise or the below values.
     */
    enum class Mode {
        AUTO = 0x1,                     /**< auto mode supported */
        MANUAL = 0x2                    /**< manual mode supported */
    };                            /**< control mode field */

    /**
     * specifies the control's values range
     */
    struct range {
        int min{0};                     /**< minimal allowed value */
        int max{0};                     /**< maximal allowed value */
        int def{0};                     /**< default value */
        int res{0};                     /**< AKA step of change in value */

        /**
         * @return true if valid range, false otherwise
         */
        bool isValid()const;
    } mRange;                           /**< control range field */

    /**
     * control basic information
     */
    struct info {
        bool isReadOnly{true};          /**< is GET/SET supported */
        bool isCached{false};           /**< do we cache control values */
        bool isAsync{false};            /**< is it an async control */
        Mode mode{Mode::AUTO};          /**< @see Mode */
    } mInfo;                            /**< control info field */

    ///////////////////////////////////////////////////////////////////////////
    //      public methods...
    ///////////////////////////////////////////////////////////////////////////

    /**
     * returns control's range data
     * @see range
     */
    range getRange() const;

    /**
     * @return false if sync or true if async
     */
    bool isAsync() const;

    /**
     * @return false if non-cachable, true otherwise
     */
    bool isCached() const;

    /**
     * @return the mode of this control (auto, manual etc.)
     */
    Mode getMode() const;
};

/**
 * this class represents per camera capabilities information.
 * the various capabilities are managed as "key-value" pairs in which
 * the key is represented as string and value is the specific control's
 * information.
 *
 * this class is initialized/populated by another class i.e. it doesn't have
 * constructor or setters. most probably Camera class will access device
 * and
 */
class CameraCapabilities {
public:
    /** shortcut typedef */
    using CameraCapabilitiesMap = std::unordered_map<std::string, CameraControl>;

    /**
     * constructor
     *
     * @param caps: the actual camera capabilities
     */
    CameraCapabilities(CameraCapabilitiesMap &&caps);

    /**
     * destructor for abstract class
     */
    virtual ~CameraCapabilities();

    /**
     * gets the control settings for specific capability
     *
     * @param tag: control to look for
     * @return the control if found
     * @throws std::invalid_argument if tag not found
     */
    const CameraControl& operator[](const std::string &tag) throw(std::invalid_argument);

    /**
     * returns the control map
     */
    CameraCapabilitiesMap getControls();

protected:
    CameraCapabilitiesMap mCapabilitiesMap; /**< capabilities container */

private:
    CameraCapabilities(const CameraCapabilities&) = delete;
    CameraCapabilities& operator = (const CameraCapabilities&) = delete;
    CameraCapabilities(CameraCapabilities&&) = delete;
    CameraCapabilities&& operator = (CameraCapabilities&&) = delete;
};

} // namespace camera_sub_system
} // namespace realsense

#endif // __CAMERA_CAPABILITIES_H__
