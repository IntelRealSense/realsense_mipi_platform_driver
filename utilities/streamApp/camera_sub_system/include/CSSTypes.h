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

#ifndef __CSS_TYPES_H__
#define __CSS_TYPES_H__

#include <vector>
#include <map>
#include <string>
#include <functional>
#include <stdexcept>
#include <memory>
#include <linux/videodev2.h>

#include "CameraCapabilities.h"

namespace realsense {
namespace camera_sub_system {

// @todo move to seperate camera related types
#define UVC_CID_GENERIC_XU          (V4L2_CID_PRIVATE_BASE+15)
#define UVC_CID_LASER_POWER_MODE    (V4L2_CID_PRIVATE_BASE+16)
#define UVC_CID_MANUAL_EXPOSURE     (V4L2_CID_PRIVATE_BASE+17)
#define UVC_CID_LASER_POWER_LEVEL   (V4L2_CID_PRIVATE_BASE+18)
#define UVC_CID_EXPOSURE_MODE       (V4L2_CID_PRIVATE_BASE+19)
#define UVC_CID_WHITE_BALANCE_MODE  (V4L2_CID_PRIVATE_BASE+20)
#define UVC_CID_PRESET              (V4L2_CID_PRIVATE_BASE+21)

#define DS5_ASRC_PID 0x0AD3     /**< RS420 */
#define DS5U_ASR_PID 0x0B03
#define DS5_AWGC_PID 0x0B07
#define DS5_AWG_PID 0x0AD4      /**< RS430 */
#define DS5_PSR_PID 0x0AD1      /**< same as AWG but has no laser */

/**
 * ds5 xu mapping selectors
 */
enum ds5_xu_selector {
	DS5_XU_SELECTOR_GENERIC = 1,
	DS5_XU_SELECTOR_LASER_POWER_MODE = 2,
	DS5_XU_SELECTOR_MANUAL_EXPOSURE = 3,
	DS5_XU_SELECTOR_LASER_POWER_LEVEL = 4,
	DS5_XU_SELECTOR_PRESET = 6,
	DS5_XU_SELECTOR_ENABLE_AUTO_WHITE_BALANCE = 10,
	DS5_XU_SELECTOR_ENABLE_AUTO_EXPOSURE = 11,
	DS5_XU_SELECTOR_EXTRINSICS = 12,
	DS5_XU_SELECTOR_INTRINSICS = 13,
};

/**
 * CSS' return status for operations
 */
enum class CssStatus {
    CSS_STATUS_SUCCESS,                 /**< operation completed successfully  */
    CSS_STATUS_ERROR,                   /**< generic error occured             */
    CSS_STATUS_PENDING,                 /**< operation didn't complete (async) */
    CSS_STATUS_ILEGAL_FLOW,             /**< wrong flow order                  */
    CSS_STATUS_ILEGAL_PARAM,            /**< parameter to function is wrong    */
    CSS_STATUS_NOT_SUPPORTED
};

/**
 * The current status of the camera device
 */
enum class CameraStatus {
    CAMERA_STATUS_NOT_PRESENT = 0,
    CAMERA_STATUS_PRESENT = 1,
};

enum class ModuleStatus {MODULE_ADDED, MODULE_REMOVED};

/**
 * The cameras type
 */
enum class CameraType {
    CAMERA_TYPE_USB = 0x1,              /**< USB camera  */
    CAMERA_TYPE_MIPI = 0x2,             /**< MIPI camera */
};

typedef unsigned FourCC;

/**
 * RSBuffer
 */
struct RsBuffer {
    void *buffer;
    uint32_t length;
    uint8_t index;

    RsBuffer (void* b, uint32_t s, uint8_t i)
    : buffer(b), length(s), index(i) {}
};

/**
 * Format
 */

struct Format {
    uint32_t v4l2Format;
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    uint32_t bytesperline;

    Format (uint32_t f, uint32_t w, uint32_t h, uint32_t fps)
    : v4l2Format(f), width(w), height(h), fps(fps)
    {
        calc64BytesAlignedStride();
    }

    uint32_t calc64BytesAlignedStride(void)
    {
        bytesperline = ((width / 64) + ((width % 64) ? 1 : 0)) * 64;

        if (v4l2Format == V4L2_PIX_FMT_YUYV || v4l2Format == V4L2_PIX_FMT_Z16)
            bytesperline *= 2;
        else if (v4l2Format == V4L2_PIX_FMT_Y12I)
            bytesperline *= 4;

        return bytesperline;
    }
};

/**
 * Resolution
 */
struct Resolution {
public:

    struct FrameRate {
        uint32_t durationNumerator;   // frame duration numerator.   Ex: 1
        uint32_t durationDenominator; // frame duration denominator. Ex: 30
        double getDouble() const {
            return durationDenominator / static_cast<double>(durationNumerator);
        };     // FrameRate in double.        Ex: 30.0
    };

    Resolution(unsigned width, unsigned height) {
        if (height == 0 || width == 0) {
            throw std::invalid_argument("height or width can't be zero");
        }

        h = height;
        w = width;
        aspectRatio = (static_cast<float>(w) / static_cast<float>(h));
    }

    unsigned getHeight() const { return h; }
    unsigned getWidth() const { return w; }
    float getAspectRatio() const { return aspectRatio; }
    std::vector<double> getFrameRateList();

    void addFrameRate(FrameRate fr) {
        frameRates.push_back(fr);
    }
    bool operator == (const Resolution &other) { return other.h == h && other.w == w; }

private:
    unsigned h{0}, w{0};
    float aspectRatio{0.0};
    std::vector<FrameRate> frameRates;
};

/**
 * StreamInfo
 */
struct StreamInfo {
    unsigned id;                                                          /**< stream id */
    std::unordered_map<FourCC, std::vector<Resolution>> supportedFormats; /**< formats map, holds resolutions and frame rates */
};

/**
 * CameraInfo
 */
struct CameraInfo {
    unsigned numOfNodes {0};                        /**< number of nodes this camera has on /dev/videoX */
    std::string serialNumber;                       /**< unique module string */
    std::vector<StreamInfo> streams;                /**< all it's streams */
    std::shared_ptr<CameraCapabilities> controls;
};

/**
 * ModuleInfo
 */
struct ModuleInfo {
public:
    /**
     * prints the info in a readable way
     */
    void print();

    unsigned pid;                           /**< USB product ID             */
    std::string path;                       /**< module's USB path          */
    std::function<int(unsigned)> controlsFunction;
    std::vector<CameraInfo> camerasInfo;    /**< all its cameras at least 1 */
};

/**
 * Node
 **/
struct Node {
    unsigned nodeNum{0};    /**< node number X in /dev/videoX           */
    long index{0};          /**< node's internal index                  */
    int fd{0};              /**< node's file descriptor                 */
};

/**
 * struct to hold CSS' initialization parameters
 */
struct InitParams {
    /**
     * constructor
     *
     * @param cb: callback for camera status change
     */
    InitParams(std::function<void(ModuleInfo, ModuleStatus)> &&cb):callback(cb) {}

    // @TODO: must be ref type so it'll be initialized in ctor
    std::function<void(ModuleInfo, ModuleStatus)> callback;    /**< callback when module is added/removed */

    bool isValid() const {
        return callback != nullptr;
    }
};

} // namespace camera_sub_system
} // namespace realsense

#endif // __CSS_TYPES_H__
