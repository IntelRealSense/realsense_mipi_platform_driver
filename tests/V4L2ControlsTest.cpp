/*
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

#include <gtest/gtest.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <errno.h>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <array>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

using namespace std;

#define DS5_STREAM_CONFIG_0                  0x4000
#define DS5_CAMERA_CID_BASE                 (V4L2_CTRL_CLASS_CAMERA | DS5_STREAM_CONFIG_0)
#define DS5_CAMERA_CID_LASER_POWER          (DS5_CAMERA_CID_BASE+1)
#define DS5_CAMERA_CID_MANUAL_LASER_POWER   (DS5_CAMERA_CID_BASE+2)
#define DS5_CAMERA_DEPTH_CALIBRATION_TABLE_GET  (DS5_CAMERA_CID_BASE+3)
#define DS5_CAMERA_DEPTH_CALIBRATION_TABLE_SET  (DS5_CAMERA_CID_BASE+4)
#define DS5_CAMERA_COEFF_CALIBRATION_TABLE_GET  (DS5_CAMERA_CID_BASE+5)
#define DS5_CAMERA_COEFF_CALIBRATION_TABLE_SET  (DS5_CAMERA_CID_BASE+6)
#define DS5_CAMERA_CID_FW_VERSION               (DS5_CAMERA_CID_BASE+7)
#define DS5_CAMERA_CID_GVD                      (DS5_CAMERA_CID_BASE+8)
#define DS5_CAMERA_CID_AE_ROI_GET               (DS5_CAMERA_CID_BASE+9)
#define DS5_CAMERA_CID_AE_ROI_SET               (DS5_CAMERA_CID_BASE+10)
#define DS5_CAMERA_CID_AE_SETPOINT_GET          (DS5_CAMERA_CID_BASE+11)
#define DS5_CAMERA_CID_AE_SETPOINT_SET          (DS5_CAMERA_CID_BASE+12)
#define DS5_CAMERA_CID_ERB                      (DS5_CAMERA_CID_BASE+13)
#define DS5_CAMERA_CID_EWB                      (DS5_CAMERA_CID_BASE+14)
#define DS5_CAMERA_CID_HWMC                     (DS5_CAMERA_CID_BASE+15)

struct Resolution {
    uint32_t width;
    uint32_t height;
    vector<uint8_t> frameRates;
};

class V4L2BasicTest : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

    string fourCCToString(unsigned int val) const
    {
        // The inverse of
        // #define v4l2_fourcc(a,b,c,d)
        // (((__u32)(a)<<0)|((__u32)(b)<<8)|((__u32)(c)<<16)|((__u32)(d)<<24))
        std::string s;
        s += val & 0x7f;
        s += (val >> 8) & 0x7f;
        s += (val >> 16) & 0x7f;
        s += (val >> 24) & 0x7f;
        if (val & (1 << 31))
            s += "-BE";
        return s;
    }

    const vector<uint32_t> ExpectedDepthMBusFormats { {
        MEDIA_BUS_FMT_UYVY8_1X16, // for Z16
        MEDIA_BUS_FMT_Y8_1X8,     // for Y8/Grey
        MEDIA_BUS_FMT_RGB888_1X24 // for Y12I

    } };

    const vector<uint32_t> ExpectedDepthFormats { {
        V4L2_PIX_FMT_GREY,
        V4L2_PIX_FMT_Z16,
        V4L2_PIX_FMT_Y12I
    } };

    const vector<uint8_t> ExpectedDepthFrameRates { { 5, 30 } };

    /*const vector<Resolution> ExpectedDepthResolutions { {
        {
            .width = 1280,
            .height = 960,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 2),
        }, {
            .width = 1280,
            .height = 720,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 2),
        },{
            .width =  640,
            .height = 480,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 1),
        }, {
            .width =  1280,
            .height = 800,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 1),
        }, {
            .width =  1600,
            .height = 1300,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 1),
        }
    } };*/

    const vector<Resolution> ExpectedDepthResolutions { {
        {
            .width = 1280,
            .height = 720,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 2),
        }, {
            .width = 848,
            .height = 480,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 2),
        },{
            .width =  848,
            .height = 100,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 1),
        }, {
            .width =  640,
            .height = 480,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 1),
        }, {
            .width =  640,
            .height = 360,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 1),
        }, {
            .width =  480,
            .height = 270,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 1),
        }, {
            .width =  424,
            .height = 240,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 1),
        }, {
            .width =  256,
            .height = 144,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 1),
        }, {
            .width =  1280,
            .height = 800,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 1),
        }
    } };

    const vector<uint32_t> ExpectedColorMBusFormats { {
        MEDIA_BUS_FMT_Y8_1X8,
        MEDIA_BUS_FMT_RGB888_1X24
    } };

    const vector<uint32_t> ExpectedColorFormats { {
        V4L2_PIX_FMT_Y8,
        V4L2_PIX_FMT_Y12I,
    } };

    const vector<uint32_t> ExpectedRgbMBusFormats { {
        MEDIA_BUS_FMT_YUYV8_2X8,
    } };

    const vector<uint32_t> ExpectedRgbFormats { {
        V4L2_PIX_FMT_YUYV,
    } };

    /*const vector<Resolution> ExpectedRgbResolutions { {
        {
            .width = 2048,
            .height = 1536,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 2),
        }, {
            .width = 1920,
            .height = 1080,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 2),
        }
    } };*/

    const vector<Resolution> ExpectedRgbResolutions { {
        {
            .width = 1280,
            .height = 800,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 2),
        }, {
            .width = 1280,
            .height = 720,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 2),
        }, {
            .width = 848,
            .height = 480,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 2),
        }, {
            .width = 640,
            .height = 480,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 2),
        }, {
            .width = 640,
            .height = 360,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 2),
        }, {
            .width = 480,
            .height = 270,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 2),
        }, {
            .width = 424,
            .height = 240,
            .frameRates = vector<uint8_t>(ExpectedDepthFrameRates.begin(), ExpectedDepthFrameRates.end() - 2),
        }
    } };

};

// Enum media bus formats(see Media Bus Pixel Codes and struct v4l2_mbus_framefmt)
// on depth sub device. This sub device includes only one output pad (0).
TEST_F(V4L2BasicTest, EnumDepthMBusFormats) {
    string depthVideoNode = {"/dev/v4l-subdev2"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);
    struct v4l2_subdev_mbus_code_enum mbusCode = {0};

    mbusCode.pad = 0;
    mbusCode.which = V4L2_SUBDEV_FORMAT_TRY;
    while (ioctl(fd, VIDIOC_SUBDEV_ENUM_MBUS_CODE , &mbusCode) == 0) {
        ASSERT_TRUE(mbusCode.code == ExpectedDepthMBusFormats[mbusCode.index]);
        mbusCode.index++;
    }
    ASSERT_TRUE(mbusCode.index == ExpectedDepthMBusFormats.size());

    close(fd);
}

// Enum media bus formats on color (y8) sub device.
// This sub device includes only one output pad (0).
/*TEST_F(V4L2BasicTest, EnumColorSMBusFormats) {
    string depthVideoNode = {"/dev/v4l-subdev3"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);
    struct v4l2_subdev_mbus_code_enum mbusCode = {0};

    mbusCode.pad = 0;
    mbusCode.which = V4L2_SUBDEV_FORMAT_TRY;
    while (ioctl(fd, VIDIOC_SUBDEV_ENUM_MBUS_CODE , &mbusCode) >= 0) {
        ASSERT_TRUE(mbusCode.code == ExpectedColorMBusFormats[mbusCode.index]);
        mbusCode.index++;
    }
    ASSERT_TRUE(mbusCode.index == ExpectedColorMBusFormats.size());

    close(fd);
}*/

// Enum media bus formats on rgb (yuyv) sub device.
// This sub device includes only one output pad (0).
/*TEST_F(V4L2BasicTest, EnumRgbSMBusFormats) {
    string depthVideoNode = {"/dev/v4l-subdev4"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);
    struct v4l2_subdev_mbus_code_enum mbusCode = {0};

    mbusCode.pad = 0;
    mbusCode.which = V4L2_SUBDEV_FORMAT_TRY;
    while (ioctl(fd, VIDIOC_SUBDEV_ENUM_MBUS_CODE , &mbusCode) >= 0) {
        ASSERT_TRUE(mbusCode.code == ExpectedRgbMBusFormats[mbusCode.index]);
        mbusCode.index++;
    }
    ASSERT_TRUE(mbusCode.index == ExpectedRgbMBusFormats.size());

    close(fd);
}*/

// Enum media bus formats on mux sub device.
// This sub device includes only one output pad (0),
// and three input pads (1, 2, 3).
// TODO: currently, the driver has two instances
/*TEST_F(V4L2BasicTest, EnumMuxMbusFormats) {
    string depthVideoNode = {"/dev/v4l-subdev5"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);
    struct v4l2_subdev_mbus_code_enum mbusCode = {0};

    mbusCode.pad = 0;
    mbusCode.which = V4L2_SUBDEV_FORMAT_TRY;
    while (ioctl(fd, VIDIOC_SUBDEV_ENUM_MBUS_CODE , &mbusCode) >= 0) {
        mbusCode.index++;
    }
    ASSERT_TRUE(mbusCode.index ==
               (ExpectedDepthMBusFormats.size() +
                ExpectedColorMBusFormats.size()));

    mbusCode.pad = 1;
    mbusCode.index = 0;
    while (ioctl(fd, VIDIOC_SUBDEV_ENUM_MBUS_CODE , &mbusCode) >= 0) {
        ASSERT_TRUE(mbusCode.code == ExpectedDepthMBusFormats[mbusCode.index]);
        mbusCode.index++;
    }
    ASSERT_TRUE(mbusCode.index == ExpectedDepthMBusFormats.size());

    mbusCode.pad = 2;
    mbusCode.index = 0;
    while (ioctl(fd, VIDIOC_SUBDEV_ENUM_MBUS_CODE , &mbusCode) >= 0) {
        ASSERT_TRUE(mbusCode.code == ExpectedColorMBusFormats[mbusCode.index]);
        mbusCode.index++;
    }
    ASSERT_TRUE(mbusCode.index == ExpectedColorMBusFormats.size());

    mbusCode.pad = 3;
    mbusCode.index = 0;
    while (ioctl(fd, VIDIOC_SUBDEV_ENUM_MBUS_CODE , &mbusCode) >= 0) {
        ASSERT_TRUE(mbusCode.code == ExpectedRgbMBusFormats[mbusCode.index]);
        mbusCode.index++;
    }
    ASSERT_TRUE(mbusCode.index == ExpectedRgbMBusFormats.size());

    close(fd);
}*/

// Get media bus format on depth sub device.
// This sub device includes only one output pad (0).
/*TEST_F(V4L2BasicTest, GetDepthMBusFormat) {
    string depthVideoNode = {"/dev/v4l-subdev2"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);
    struct v4l2_subdev_format subdevFormat = {0};

    subdevFormat.pad = 0;
    subdevFormat.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    int ret = ioctl(fd, VIDIOC_SUBDEV_G_FMT , &subdevFormat);
    ASSERT_TRUE(0 == ret);
    ASSERT_TRUE(subdevFormat.format.code == ExpectedDepthMBusFormats[0]);

    close(fd);
}*/

// Set media bus format on depth sub device.
// This sub device includes only one output pad (0).
// TODO: need to improve this test, very basic
/*TEST_F(V4L2BasicTest, SetDepthMBusFormat) {
    string depthVideoNode = {"/dev/v4l-subdev2"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);
    struct v4l2_subdev_format subdevFormat = {0};

    // set format
    subdevFormat.pad = 0;
    subdevFormat.format.code = ExpectedDepthMBusFormats[0];
    subdevFormat.format.width = 640;
    subdevFormat.format.height = 480;
    subdevFormat.format.field = 1;
    subdevFormat.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    int ret = ioctl(fd, VIDIOC_SUBDEV_S_FMT , &subdevFormat);
    ASSERT_TRUE(0 == ret);

    // get format
    memset(&subdevFormat, 0, sizeof(subdevFormat));
    subdevFormat.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    ret = ioctl(fd, VIDIOC_SUBDEV_G_FMT , &subdevFormat);
    ASSERT_TRUE(0 == ret);
    ASSERT_TRUE(subdevFormat.format.code == ExpectedDepthMBusFormats[0]);

    close(fd);
}*/

// Get media bus format on color(y8) sub device.
// This sub device includes only one output pad (0).
/*TEST_F(V4L2BasicTest, GetColorMBusFormat) {
    string depthVideoNode = {"/dev/v4l-subdev3"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);
    struct v4l2_subdev_format subdevFormat = {0};

    subdevFormat.pad = 0;
    subdevFormat.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    int ret = ioctl(fd, VIDIOC_SUBDEV_G_FMT , &subdevFormat);
    ASSERT_TRUE(0 == ret);
    ASSERT_TRUE(ExpectedColorMBusFormats.end() !=
                find (ExpectedColorMBusFormats.begin(),
                      ExpectedColorMBusFormats.end(),
                      subdevFormat.format.code));

    close(fd);
}*/

// Set media bus format on color(y8) sub device.
// This sub device includes only one output pad (0)
// TODO: need to improve this test, very basic
/*TEST_F(V4L2BasicTest, SetColorMBusFormat) {
    string depthVideoNode = {"/dev/v4l-subdev3"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);
    struct v4l2_subdev_format subdevFormat = {0};

    // set format
    subdevFormat.pad = 0;
    subdevFormat.format.code = ExpectedColorMBusFormats[0];
    subdevFormat.format.width = 640;
    subdevFormat.format.height = 480;
    subdevFormat.format.field = 1;
    subdevFormat.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    int ret = ioctl(fd, VIDIOC_SUBDEV_S_FMT , &subdevFormat);
    ASSERT_TRUE(0 == ret);

    // get format
    memset(&subdevFormat, 0, sizeof(subdevFormat));
    subdevFormat.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    ret = ioctl(fd, VIDIOC_SUBDEV_G_FMT , &subdevFormat);
    ASSERT_TRUE(0 == ret);
    ASSERT_TRUE(subdevFormat.format.code == ExpectedColorMBusFormats[0]);

    close(fd);
}*/

// TODO: VIDIOC_ENUM_FMT tests fail
TEST_F(V4L2BasicTest, EnumDepthFormats) {
    string colorVideoNode = {"/dev/video0"};
    int fd = open(colorVideoNode.c_str(), O_RDWR);
    struct v4l2_fmtdesc fmt = {0};

    vector<uint32_t> expectedFormats {ExpectedDepthFormats};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) >= 0) {
        ASSERT_TRUE(0 == fmt.flags);
        auto it = find(expectedFormats.begin(), expectedFormats.end(), fmt.pixelformat);
        ASSERT_TRUE(it != expectedFormats.end());
        expectedFormats.erase(it);
        fmt.index++;
    }

    ASSERT_TRUE(expectedFormats.empty());
    close(fd);
}

TEST_F(V4L2BasicTest, EnumDepthFramesizes) {
    string colorVideoNode = {"/dev/video0"};
    int fd = open(colorVideoNode.c_str(), O_RDWR);

    vector<Resolution> expectedDepthResolutions { ExpectedDepthResolutions };
    for (uint32_t fmt : ExpectedDepthFormats) {

        v4l2_frmsizeenum frameSize
        {
            .index = 0,
            .pixel_format = fmt
        };

        while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frameSize) == 0) {
            ASSERT_TRUE(V4L2_FRMSIZE_TYPE_DISCRETE == frameSize.type);
            Resolution tmpRes = {
                .width = frameSize.discrete.width,
                .height = frameSize.discrete.height
            };

            bool found = false;
            vector<Resolution>::iterator it = expectedDepthResolutions.begin();
            while (it != expectedDepthResolutions.end()) {
                if (tmpRes.width == it->width && tmpRes.height == it->height) {
                    found = true;
                    break;
                } else
                    ++it;
            }
            //ASSERT_TRUE(found);

            frameSize.index++;
        }
    }

    close(fd);
}
// TODO: dpends on camera/ 43x or 46x
/*TEST_F(V4L2BasicTest, EnumDepthFrameIntervals) {
    string colorVideoNode = {"/dev/video0"};
    int fd = open(colorVideoNode.c_str(), O_RDWR);


    for (uint32_t fmt : ExpectedDepthFormats) {
        for (Resolution resolution : ExpectedDepthResolutions) {
            struct v4l2_frmivalenum frameIntvl {
                .index = 0,
                .pixel_format = fmt,
                .width = resolution.width,
                .height = resolution.height
            };

            vector<uint8_t> expectedDepthFrameRates { resolution.frameRates };
            while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frameIntvl) == 0) {
                ASSERT_TRUE(V4L2_FRMIVAL_TYPE_DISCRETE == frameIntvl.type);

                vector<uint8_t>::iterator it = expectedDepthFrameRates.begin();
                while (it != expectedDepthFrameRates.end()) {
                    if (frameIntvl.discrete.denominator == *it)
                        it = expectedDepthFrameRates.erase(it);
                    else
                        ++it;
                }
                frameIntvl.index++;
            }
            ASSERT_TRUE(expectedDepthFrameRates.empty());
        }
    }
    close(fd);
}*/


// TODO: Fix this test after handling VIDIOC_ENUM_FMT
TEST_F(V4L2BasicTest, EnumRgbFormats) {
    string rgbVideoNode = {"/dev/video2"};
    int fd = open(rgbVideoNode.c_str(), O_RDWR);
    struct v4l2_fmtdesc fmt = {0};

    vector<uint32_t> expectedFormats {ExpectedRgbFormats};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) >= 0) {
        auto it = find(expectedFormats.begin(), expectedFormats.end(), fmt.pixelformat);
        ASSERT_TRUE(it != expectedFormats.end());
        expectedFormats.erase(it);
        fmt.index++;
    }

    ASSERT_TRUE(expectedFormats.empty());
    close(fd);
}

TEST_F(V4L2BasicTest, EnumRgbFramesizes) {
    string colorVideoNode = {"/dev/video2"};
    int fd = open(colorVideoNode.c_str(), O_RDWR);

    vector<Resolution> expectedRgbResolutions { ExpectedRgbResolutions };
    for (uint32_t fmt : ExpectedRgbFormats) {

        v4l2_frmsizeenum frameSize
        {
            .index = 0,
            .pixel_format = V4L2_PIX_FMT_YUYV
        };

        int ret = 0;
        while ( 0 == ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frameSize)) {
            ASSERT_TRUE(V4L2_FRMSIZE_TYPE_DISCRETE == frameSize.type);
            Resolution tmpRes = {
                .width = frameSize.discrete.width,
                .height = frameSize.discrete.height
            };

            bool found = false;
            vector<Resolution>::iterator it = expectedRgbResolutions.begin();
            while (it != expectedRgbResolutions.end()) {
                if (tmpRes.width == it->width && tmpRes.height == it->height) {
                    found = true;
                    break;
                } else
                    ++it;
            }
            ASSERT_TRUE(found);

            frameSize.index++;
        }
    }

    close(fd);
}

// TODO: need to debug this test
TEST_F(V4L2BasicTest, EnumMetaDataFormats) {
    string mdVideoNode1 = {"/dev/video1"};
    int dev_fd1 = open(mdVideoNode1.c_str(), O_RDWR);
    string mdVideoNode3 = {"/dev/video3"};
    int dev_fd3 = open(mdVideoNode3.c_str(), O_RDWR);
    struct v4l2_fmtdesc fmt = {0};
    struct v4l2_capability cap1 {0};
    struct v4l2_capability cap3 {0};

    int ret = ioctl(dev_fd1, VIDIOC_QUERYCAP, &cap1);
    ASSERT_TRUE(0 == ret);
    ASSERT_TRUE(0 != (cap1.device_caps & V4L2_CAP_META_CAPTURE));

    ret = ioctl(dev_fd3, VIDIOC_QUERYCAP, &cap3);
    ASSERT_TRUE(0 == ret);
    ASSERT_TRUE(0 != (cap3.device_caps & V4L2_CAP_META_CAPTURE));

    fmt.index = 0;
    fmt.type = V4L2_BUF_TYPE_META_CAPTURE;
    ret = ioctl(dev_fd1, VIDIOC_ENUM_FMT, &fmt);
    ASSERT_TRUE(0 == ret);
    ASSERT_TRUE( V4L2_BUF_TYPE_META_CAPTURE == fmt.type);
    ASSERT_TRUE(V4L2_META_FMT_D4XX_CSI2 == fmt.pixelformat);

    ret = ioctl(dev_fd3, VIDIOC_ENUM_FMT, &fmt);
    ASSERT_TRUE(0 == ret);
    ASSERT_TRUE( V4L2_BUF_TYPE_META_CAPTURE == fmt.type);
    ASSERT_TRUE(V4L2_META_FMT_D4XX_CSI2 == fmt.pixelformat);

    close(dev_fd1);
    close(dev_fd3);
}

TEST_F(V4L2BasicTest, SetGetDepthFmt) {
    string depthVideoNode = {"/dev/video0"};
    int dev_fd = open(depthVideoNode.c_str(), O_RDWR);
    struct v4l2_format orgFormat = {0};

    // Save original format
    orgFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret = ioctl(dev_fd, VIDIOC_G_FMT, &orgFormat);
    ASSERT_TRUE(0 == ret);

    // Set new format
    struct v4l2_format sFormat = {0};
    sFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    sFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_Z16;
    sFormat.fmt.pix.width = 1280;
    sFormat.fmt.pix.height = 720;
    ret = ioctl(dev_fd, VIDIOC_S_FMT, &sFormat);
    ASSERT_TRUE(0 == ret);

    // Get format and verify it is equal to the set one
    struct v4l2_format gFormat = {0};
    gFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(dev_fd, VIDIOC_G_FMT, &gFormat);
    ASSERT_TRUE(0 == ret);
    ASSERT_TRUE(gFormat.fmt.pix.pixelformat == sFormat.fmt.pix.pixelformat);
    ASSERT_TRUE(gFormat.fmt.pix.width == sFormat.fmt.pix.width);
    ASSERT_TRUE(gFormat.fmt.pix.height == sFormat.fmt.pix.height);

    // Restore original format
    ret = ioctl(dev_fd, VIDIOC_S_FMT, &orgFormat);
    ASSERT_TRUE(0 == ret);

    close(dev_fd);
}

TEST_F(V4L2BasicTest, GetMetaDataFmt) {
    string mdVideoNode = {"/dev/video1"};
    int dev_fd = open(mdVideoNode.c_str(), O_RDWR);
    struct v4l2_format fmt = {0};

    fmt.type = V4L2_BUF_TYPE_META_CAPTURE;
    int ret = ioctl(dev_fd, VIDIOC_G_FMT, &fmt);
    ASSERT_TRUE(0 == ret);
    ASSERT_TRUE(V4L2_BUF_TYPE_META_CAPTURE == fmt.type);
    ASSERT_TRUE(V4L2_META_FMT_D4XX_CSI2 == fmt.fmt.meta.dataformat);
    close(dev_fd);

    mdVideoNode = {"/dev/video3"};
    dev_fd = open(mdVideoNode.c_str(), O_RDWR);
    ret = ioctl(dev_fd, VIDIOC_G_FMT, &fmt);
    ASSERT_TRUE(0 == ret);
    ASSERT_TRUE(V4L2_BUF_TYPE_META_CAPTURE == fmt.type);
    ASSERT_TRUE(V4L2_META_FMT_D4XX_CSI2 == fmt.fmt.meta.dataformat);
    close(dev_fd);

}

TEST_F(V4L2BasicTest, SetGetAutoExpMode) {
    string videoNode = {"/dev/video0"};
    int fd = open(videoNode.c_str(), O_RDWR);

    struct v4l2_ext_control aeModeCtrl {0};
    aeModeCtrl.id = V4L2_CID_EXPOSURE_AUTO;
    aeModeCtrl.size = 0;

    const int aeMin = 0;
    const int aeMax = 1;
    const int aeStep = 1;

    for (aeModeCtrl.value = aeMin;
         aeModeCtrl.value < aeMax + 1;
         aeModeCtrl.value += aeStep) {

        // set auto exposure mode
        struct v4l2_ext_controls ext {0};
        ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
        ext.controls = &aeModeCtrl;
        ext.count = 1;
        int ret = ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext);
        ASSERT_TRUE(0 == ret);

        // get auto exposure mode
        struct v4l2_ext_control currAeCtrl {0};
        currAeCtrl.id = V4L2_CID_EXPOSURE_AUTO;
        ext.controls = &currAeCtrl;
        ext.count = 1;
        ret = ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext);
        ASSERT_TRUE(0 == ret);
        ASSERT_TRUE(currAeCtrl.value == aeModeCtrl.value);
    }

    close(fd);
}

TEST_F(V4L2BasicTest, SetGetExposure) {
    string videoNode = {"/dev/video0"};
    int fd = open(videoNode.c_str(), O_RDWR);

    // set ae off
    struct v4l2_ext_control aeModeCtrl {0};
    aeModeCtrl.id = V4L2_CID_EXPOSURE_AUTO;
    aeModeCtrl.size = 0;
    aeModeCtrl.value = 0;

    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &aeModeCtrl;
    ext.count = 1;
    int ret = ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext);
    ASSERT_TRUE(0 == ret);

    struct v4l2_ext_control expCtrl {0};
    expCtrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
    expCtrl.size = 0;

    const int expMin = 10;
    const int expMax = 1660;
    const int expStep = 20;

    for (expCtrl.value = expMin;
         expCtrl.value < expMax + 1;
         expCtrl.value += expStep*10) {

        // set exposure
        struct v4l2_ext_controls ext {0};
        ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
        ext.controls = &expCtrl;
        ext.count = 1;
        int ret = ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext);
        ASSERT_TRUE(0 == ret);

        // get exposure
        struct v4l2_ext_control currExpCtrl {0};
        currExpCtrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
        ext.controls = &currExpCtrl;
        ext.count = 1;
        ret = ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext);
        ASSERT_TRUE(0 == ret);
        ASSERT_TRUE(currExpCtrl.value == expCtrl.value);
    }

    close(fd);
}

TEST_F(V4L2BasicTest, SetGetLaserPower) {
    string videoNode = {"/dev/video0"};
    int fd = open(videoNode.c_str(), O_RDWR);

    struct v4l2_ext_control laserCtrl {0};
    laserCtrl.id = DS5_CAMERA_CID_LASER_POWER;
    laserCtrl.size = 0;

    const int laserValueMin = 0;
    const int laserValueMax = 1;
    const int laserValueStep = 1;

    for (laserCtrl.value = laserValueMin;
         laserCtrl.value < laserValueMax + 1;
         laserCtrl.value += laserValueStep) {

        // set laser power
        struct v4l2_ext_controls ext {0};
        ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
        ext.controls = &laserCtrl;
        ext.count = 1;
        int ret = ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext);
        ASSERT_TRUE(0 == ret);

        // get laser power
        struct v4l2_ext_control currLaserCtrl {0};
        currLaserCtrl.id = DS5_CAMERA_CID_LASER_POWER;
        ext.controls = &currLaserCtrl;
        ext.count = 1;
        ret = ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext);
        ASSERT_TRUE(0 == ret);
        ASSERT_TRUE(currLaserCtrl.value == laserCtrl.value);
    }

    close(fd);
}

TEST_F(V4L2BasicTest, SetGetManualLaserPower) {
    string videoNode = {"/dev/video0"};
    int fd = open(videoNode.c_str(), O_RDWR);

    struct v4l2_ext_control manualLaserCtrl {0};
    manualLaserCtrl.id = DS5_CAMERA_CID_MANUAL_LASER_POWER;
    manualLaserCtrl.size = 0;

    const int manualLaserPowerMin = 0;
    const int manualLaserPowerMax = 360;
    const int manualLaserPowerStep = 30;

    for (manualLaserCtrl.value = manualLaserPowerMin;
         manualLaserCtrl.value < manualLaserPowerMax + 1;
         manualLaserCtrl.value += manualLaserPowerStep) {

        // set laser power
        struct v4l2_ext_controls ext {0};
        ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
        ext.controls = &manualLaserCtrl;
        ext.count = 1;
        int ret = ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext);
        ASSERT_TRUE(0 == ret);

        // get laser power
        struct v4l2_ext_control currManualLaserCtrl {0};
        currManualLaserCtrl.id = DS5_CAMERA_CID_MANUAL_LASER_POWER;
        ext.controls = &currManualLaserCtrl;
        ext.count = 1;
        ret = ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext);
        ASSERT_TRUE(0 == ret);
        ASSERT_TRUE(currManualLaserCtrl.value == manualLaserCtrl.value);
    }

    close(fd);
}

// Get FW version
TEST_F(V4L2BasicTest, GetFwVersion) {
    string depthVideoNode = {"/dev/video0"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);

    uint32_t fwVersion {0};
    struct v4l2_ext_control ctrl {0};
    ctrl.id = DS5_CAMERA_CID_FW_VERSION;
    ctrl.size = sizeof(fwVersion);
    ctrl.p_u32 = &fwVersion;

    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &ctrl;
    ext.count = 1;

    ASSERT_TRUE(0 ==  ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext));
    ASSERT_TRUE(0 != fwVersion);

    close(fd);
}

// Get FW version
TEST_F(V4L2BasicTest, GetGVD) {
    string depthVideoNode = {"/dev/video0"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);

    uint8_t gvd[276];
    struct v4l2_ext_control ctrl {0};
    ctrl.id = DS5_CAMERA_CID_GVD;
    ctrl.size = sizeof(gvd);
    ctrl.p_u8 = gvd;

    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &ctrl;
    ext.count = 1;

    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext));

    close(fd);
}

// Get depth calibration table
TEST_F(V4L2BasicTest, GetDepthCalib) {
    string depthVideoNode = {"/dev/video0"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);

    uint8_t depth[256];
    struct v4l2_ext_control ctrl {0};
    ctrl.id = DS5_CAMERA_DEPTH_CALIBRATION_TABLE_GET;
    ctrl.size = sizeof(depth);
    ctrl.p_u8 = depth;

    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &ctrl;
    ext.count = 1;

    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext));
    ASSERT_TRUE(0x1f == depth[2]);

    close(fd);
}

// Set depth calibration table
TEST_F(V4L2BasicTest, SetDepthCalib) {
    string depthVideoNode = {"/dev/video0"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);

    // Read origianl depth calib table
    uint8_t orgDepth[256];
    struct v4l2_ext_control ctrl {0};
    ctrl.id = DS5_CAMERA_DEPTH_CALIBRATION_TABLE_GET;
    ctrl.size = sizeof(orgDepth);
    ctrl.p_u8 = orgDepth;

    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &ctrl;
    ext.count = 1;

    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext));
    ASSERT_TRUE(0x1f == orgDepth[2]);

    uint8_t depth[256] = {
            0x03, 0x00, 0x1F, 0x00, 0xF0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x17, 0x74, 0x4C, 0xBC,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    };

    ctrl.id = DS5_CAMERA_DEPTH_CALIBRATION_TABLE_SET;
    ctrl.p_u8 = depth;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));

    // Read table and verify it was written
    uint8_t tmpDepth[256];
    ctrl.id = DS5_CAMERA_DEPTH_CALIBRATION_TABLE_GET;
    ctrl.p_u8 = tmpDepth;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext));
    ASSERT_TRUE(0 == memcmp( tmpDepth, depth, sizeof(tmpDepth)));

    // Restore original table
    ctrl.id = DS5_CAMERA_DEPTH_CALIBRATION_TABLE_SET;
    ctrl.p_u8 = orgDepth;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));

    // Write depth table
    ctrl.id = DS5_CAMERA_DEPTH_CALIBRATION_TABLE_SET;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));

    close(fd);
}


// Get coeff calibration table
TEST_F(V4L2BasicTest, GetCoeffCalib) {
    string depthVideoNode = {"/dev/video0"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);

    uint8_t coeff[512];
    struct v4l2_ext_control ctrl {0};
    ctrl.id = DS5_CAMERA_COEFF_CALIBRATION_TABLE_GET;
    ctrl.size = sizeof(coeff);
    ctrl.p_u8 = coeff;

    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &ctrl;
    ext.count = 1;

    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext));
    ASSERT_TRUE(0x19 == coeff[2]);

    close(fd);
}

// Set coeff calibration table
TEST_F(V4L2BasicTest, SetCoeffCalib) {
    string depthVideoNode = {"/dev/video0"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);

    // Read origianl depth calib table
    uint8_t orgCoef[512];
    struct v4l2_ext_control ctrl {0};
    ctrl.id = DS5_CAMERA_COEFF_CALIBRATION_TABLE_GET;
    ctrl.size = sizeof(orgCoef);
    ctrl.p_u8 = orgCoef;

    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &ctrl;
    ext.count = 1;


    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext));
    ASSERT_TRUE(0x19 == orgCoef[2]);

    uint8_t newCoef[512] = {
            0x03, 0x00, 0x19, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x84, 0x0E, 0x2A, 0x04, 0x6D, 0x91, 0x5A, 0x78,
            0x65, 0xCC, 0xFF, 0x3E, 0x16, 0x0F, 0x4C, 0x3F, 0xE6, 0x46, 0x00, 0x3F, 0x9D, 0x3D, 0x01, 0x3F,
            0x88, 0x5C, 0x65, 0xBD, 0x47, 0x57, 0x86, 0x3D, 0xF6, 0x4C, 0x45, 0x3A, 0x4A, 0xE0, 0x0E, 0xBA,
            0x6D, 0x91, 0xAC, 0xBC, 0xD3, 0x08, 0x00, 0x3F, 0x26, 0x49, 0x4C, 0x3F, 0x72, 0x3F, 0x00, 0x3F,
            0x3F, 0xE6, 0x00, 0x3F, 0xDE, 0x18, 0x65, 0xBD, 0x4B, 0xEF, 0x83, 0x3D, 0x20, 0x17, 0x80, 0x39,
            0x7F, 0x94, 0x3A, 0xBA, 0x8E, 0x89, 0xA7, 0xBC, 0xA3, 0xFF, 0x7F, 0x3F, 0xCE, 0x16, 0x56, 0xBB,
            0xA1, 0xBB, 0x24, 0x3A, 0x7C, 0x0F, 0x56, 0x3B, 0xA2, 0xFF, 0x7F, 0x3F, 0x2A, 0xE9, 0x35, 0x3A,
            0x86, 0x53, 0x25, 0xBA, 0x29, 0x5F, 0x35, 0xBA, 0xF9, 0xFF, 0x7F, 0x3F, 0xE4, 0xFE, 0x7F, 0x3F,
            0x28, 0x72, 0xBA, 0xBB, 0xF9, 0x71, 0x9C, 0xBA, 0x35, 0x6B, 0xBA, 0x3B, 0xEC, 0xFE, 0x7F, 0x3F,
            0x46, 0x88, 0x36, 0xBA, 0x41, 0xF6, 0x9C, 0x3A, 0xCA, 0xBF, 0x34, 0x3A, 0xF0, 0xFF, 0x7F, 0x3F,
            0x79, 0xEF, 0x47, 0xC2, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x14, 0xFC, 0x70, 0x44, 0x14, 0xFC, 0x70, 0x44, 0xF4, 0xDA, 0x6F, 0x44, 0xF6, 0x67, 0x08, 0x44,
            0x0E, 0xA8, 0x20, 0x44, 0x0E, 0xA8, 0x20, 0x44, 0x4D, 0xE7, 0x1F, 0x44, 0xF3, 0xDF, 0xB5, 0x43,
            0xAA, 0xC9, 0xC0, 0x43, 0xAA, 0xC9, 0xC0, 0x43, 0x5D, 0xE2, 0x9F, 0x43, 0xF0, 0x3F, 0x72, 0x43,
            0xAC, 0xDE, 0xD4, 0x43, 0xAC, 0xDE, 0xD4, 0x43, 0x46, 0xDF, 0xD3, 0x43, 0xEF, 0x7B, 0x72, 0x43,
            0x0E, 0xA8, 0xA0, 0x43, 0x0E, 0xA8, 0xA0, 0x43, 0x4D, 0xE7, 0x9F, 0x43, 0xF3, 0xDF, 0x35, 0x43,
            0xAC, 0xDE, 0x54, 0x43, 0xAC, 0xDE, 0x54, 0x43, 0x46, 0xDF, 0x53, 0x43, 0xEF, 0x7B, 0xF2, 0x42,
            0xAA, 0xC9, 0x40, 0x43, 0xAA, 0xC9, 0x40, 0x43, 0x5D, 0xE2, 0x1F, 0x43, 0xF0, 0x3F, 0xF2, 0x42,
            0x14, 0xFC, 0x70, 0x43, 0x14, 0xFC, 0x70, 0x43, 0xF4, 0xDA, 0x6F, 0x43, 0xF6, 0x67, 0x08, 0x43,
            0x0E, 0xA8, 0x20, 0x44, 0x0E, 0xA8, 0x20, 0x44, 0x4D, 0xE7, 0x1F, 0x44, 0xF3, 0xDF, 0xC9, 0x43,
            0x14, 0xFC, 0xF0, 0x43, 0x14, 0xFC, 0xF0, 0x43, 0xF4, 0xDA, 0xEF, 0x43, 0xF6, 0x67, 0x88, 0x43,
            0x3F, 0x97, 0x10, 0x44, 0x3F, 0x97, 0x10, 0x44, 0x8B, 0xD3, 0xB3, 0x43, 0xF4, 0xAF, 0xB5, 0x43,
            0x66, 0x58, 0xE7, 0x43, 0x66, 0x58, 0xE7, 0x43, 0x6F, 0xDC, 0x8F, 0x43, 0x90, 0x59, 0x91, 0x43,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };

    // Write coeff table
    uint8_t newCoef1[512] = {
            0x03, 0x00, 0x19, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x60, 0x0C, 0x2F, 0x04, 0xF9, 0x30, 0x49, 0xFC,
            0x25, 0x67, 0x5C, 0x3F, 0xA6, 0x3C, 0x87, 0x3F, 0xAB, 0x5B, 0x01, 0x3F, 0xA9, 0x55, 0xFB, 0x3E,
            0xF9, 0x2D, 0xDC, 0xBE, 0x21, 0x1C, 0x53, 0x3E, 0xA9, 0xA6, 0xD2, 0xB9, 0xF2, 0x43, 0xBF, 0xB9,
            0xC0, 0x6A, 0x6E, 0xBD, 0x9A, 0x1E, 0x5C, 0x3F, 0x62, 0x1D, 0x87, 0x3F, 0x1F, 0x0F, 0x00, 0x3F,
            0x44, 0xF9, 0x04, 0x3F, 0xB8, 0x54, 0xDC, 0xBE, 0x73, 0x07, 0x56, 0x3E, 0xB9, 0xD4, 0x7B, 0x39,
            0x11, 0x05, 0x78, 0x38, 0x7A, 0xEE, 0x79, 0xBD, 0xF7, 0xFF, 0x7F, 0x3F, 0xB3, 0x75, 0x26, 0xBA,
            0xCC, 0x25, 0x5B, 0xBA, 0x43, 0x9F, 0x27, 0x3A, 0x10, 0xFF, 0x7F, 0x3F, 0x24, 0x24, 0xAE, 0x3B,
            0x88, 0x42, 0x5A, 0x3A, 0x9A, 0x28, 0xAE, 0xBB, 0x0D, 0xFF, 0x7F, 0x3F, 0xA7, 0xFF, 0x7F, 0x3F,
            0xDE, 0x02, 0x0C, 0xBB, 0x08, 0x04, 0x21, 0x3B, 0x6F, 0xDD, 0x0C, 0x3B, 0xED, 0xFE, 0x7F, 0x3F,
            0x49, 0x10, 0xAE, 0xBB, 0xF5, 0x44, 0x20, 0xBB, 0x59, 0x3C, 0xAE, 0x3B, 0xE1, 0xFE, 0x7F, 0x3F,
            0x52, 0xBD, 0x01, 0xC3, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x2E, 0x8F, 0xBD, 0x44, 0x2E, 0x8F, 0xBD, 0x44, 0x39, 0xB6, 0x72, 0x44, 0x6B, 0x64, 0x0A, 0x44,
            0xE7, 0xBE, 0x7C, 0x44, 0xE7, 0xBE, 0x7C, 0x44, 0xD0, 0xCE, 0x21, 0x44, 0xE4, 0x85, 0xB8, 0x43,
            0xE7, 0xBE, 0xFC, 0x43, 0xE7, 0xBE, 0xFC, 0x43, 0xD0, 0xCE, 0xA1, 0x43, 0xE4, 0x85, 0x74, 0x43,
            0xAD, 0x71, 0x27, 0x44, 0xAD, 0x71, 0x27, 0x44, 0x3B, 0x65, 0xD6, 0x43, 0x34, 0xFE, 0x75, 0x43,
            0xE7, 0xBE, 0xFC, 0x43, 0xE7, 0xBE, 0xFC, 0x43, 0xD0, 0xCE, 0xA1, 0x43, 0xE4, 0x85, 0x38, 0x43,
            0xAD, 0x71, 0xA7, 0x43, 0xAD, 0x71, 0xA7, 0x43, 0x3B, 0x65, 0x56, 0x43, 0x34, 0xFE, 0xF5, 0x42,
            0xE7, 0xBE, 0x7C, 0x43, 0xE7, 0xBE, 0x7C, 0x43, 0xD0, 0xCE, 0x21, 0x43, 0xE4, 0x85, 0xF4, 0x42,
            0x2E, 0x8F, 0xBD, 0x43, 0x2E, 0x8F, 0xBD, 0x43, 0x39, 0xB6, 0x72, 0x43, 0x6B, 0x64, 0x0A, 0x43,
            0xE7, 0xBE, 0x7C, 0x44, 0xE7, 0xBE, 0x7C, 0x44, 0xD0, 0xCE, 0x21, 0x44, 0xE4, 0x85, 0xCC, 0x43,
            0x2E, 0x8F, 0x3D, 0x44, 0x2E, 0x8F, 0x3D, 0x44, 0x39, 0xB6, 0xF2, 0x43, 0x6B, 0x64, 0x8A, 0x43,
            0x51, 0xFA, 0x2E, 0x44, 0x51, 0xFA, 0x2E, 0x44, 0xD2, 0x80, 0xB6, 0x43, 0x9E, 0x21, 0xB7, 0x43,
            0x74, 0xFB, 0x0B, 0x44, 0x74, 0xFB, 0x0B, 0x44, 0xA8, 0x00, 0x92, 0x43, 0x4B, 0x81, 0x92, 0x43,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    ctrl.id = DS5_CAMERA_COEFF_CALIBRATION_TABLE_SET;
    ctrl.p_u8 = newCoef1;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));

    // Read table and verify it was written
    uint8_t tmpCoef[512];
    ctrl.id = DS5_CAMERA_COEFF_CALIBRATION_TABLE_GET;
    ctrl.p_u8 = tmpCoef;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext));
    ASSERT_TRUE(0 == memcmp( tmpCoef, newCoef1, sizeof(tmpCoef)));

    // Restore original table
    ctrl.id = DS5_CAMERA_COEFF_CALIBRATION_TABLE_SET;
    ctrl.p_u8 = orgCoef;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));

    close(fd);
}

// Get/Set ae roi
TEST_F(V4L2BasicTest, GetSetAeRoi) {
    string depthVideoNode = {"/dev/video0"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);

    // Read ae roi
    uint16_t orgAeRoi[4];
    struct v4l2_ext_control ctrl {0};
    ctrl.id = DS5_CAMERA_CID_AE_ROI_GET;
    ctrl.size = sizeof(orgAeRoi);
    ctrl.p_u16 = orgAeRoi;

    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &ctrl;
    ext.count = 1;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext));

    uint16_t newAeRoi[4] = {0x6C, 0x274, 0xAA, 0x460};
    ctrl.id = DS5_CAMERA_CID_AE_ROI_SET;
    ctrl.p_u16 = newAeRoi;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));

    // Read value and verify it was written
    uint16_t tmpAeRoi[4];
    ctrl.id = DS5_CAMERA_CID_AE_ROI_GET;
    ctrl.p_u16 = tmpAeRoi;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext));
    ASSERT_TRUE(0 == memcmp( tmpAeRoi, newAeRoi, sizeof(tmpAeRoi)));

    // Restore original value
    ctrl.id = DS5_CAMERA_CID_AE_ROI_SET;
    ctrl.p_u16 = orgAeRoi;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));

    close(fd);
}

// Get/Set ae setpoint
TEST_F(V4L2BasicTest, GetSetAeSetpoint) {
    string depthVideoNode = {"/dev/video0"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);

    // Read ae roi
    uint32_t orgAeSetpoint {0};
    struct v4l2_ext_control ctrl {0};
    ctrl.id = DS5_CAMERA_CID_AE_SETPOINT_GET;
    ctrl.size = sizeof(uint32_t);

    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &ctrl;
    ext.count = 1;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext));
    orgAeSetpoint = ctrl.value;

    uint32_t newAeSetpoint = 4095;
    ctrl.id = DS5_CAMERA_CID_AE_SETPOINT_SET;
    ctrl.value = newAeSetpoint;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));

    // Read value and verify it was written
    ctrl.id = DS5_CAMERA_CID_AE_SETPOINT_GET;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext));
    ASSERT_TRUE(ctrl.value == newAeSetpoint);

    newAeSetpoint = 1000;
    ctrl.id = DS5_CAMERA_CID_AE_SETPOINT_SET;
    ctrl.value = newAeSetpoint;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));

    // Read value and verify it was written
    ctrl.id = DS5_CAMERA_CID_AE_SETPOINT_GET;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext));
    ASSERT_TRUE(ctrl.value == newAeSetpoint);

    // Restore original value
    ctrl.id = DS5_CAMERA_CID_AE_SETPOINT_SET;
    ctrl.value = orgAeSetpoint;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));

    close(fd);
}

// ERB
TEST_F(V4L2BasicTest, Erb) {
    string depthVideoNode = {"/dev/video0"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);

    // erb
    uint8_t erb[1020] {0};
    // erb[0] and erb[1] are for offset, in this sample it is 0x0000
    erb[0] = 0x00;
    erb[1] = 0x00;

    //read size 0x03F8
    erb[2] = 0x03;
    erb[3] = 0xF8;

    struct v4l2_ext_control ctrl {0};
    ctrl.id = DS5_CAMERA_CID_ERB;
    ctrl.size = sizeof(erb);
    ctrl.p_u8 = erb;

    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &ctrl;
    ext.count = 1;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));
    ASSERT_TRUE(0x00 == erb[0]);// verify offset
    ASSERT_TRUE(0x00 == erb[1]);
    ASSERT_TRUE(0x03 == erb[2]);// verify size
    ASSERT_TRUE(0xF8 == erb[3]);

    // erb[0] and erb[1] are for offset, in this sample it is 0x7D0, 2000
    erb[0] = 0x07;
    erb[1] = 0xD0;

    //read size 0x03F8
    erb[2] = 0x03;
    erb[3] = 0xF8;
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));
    ASSERT_TRUE(0x07 == erb[0]);// verify offset
    ASSERT_TRUE(0xD0 == erb[1]);
    ASSERT_TRUE(0x03 == erb[2]);// verify size
    ASSERT_TRUE(0xF8 == erb[3]);

    close(fd);
}

#pragma pack(push, 1)
struct HWMC {
    uint16_t header;
    uint16_t magic_word;
    uint32_t opcode;
    uint32_t params[4];
};
#pragma pack(pop)

// HWMC
TEST_F(V4L2BasicTest, Hwmc) {
    string depthVideoNode = {"/dev/video0"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);

    uint8_t hwmc[1028] {0};

    struct v4l2_ext_control ctrl {0};
    ctrl.id = DS5_CAMERA_CID_HWMC;
    ctrl.size = sizeof(hwmc);
    ctrl.p_u8 = hwmc;

    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &ctrl;
    ext.count = 1;

    // Get org depth calibration table
    HWMC getDepthTable {0};
    getDepthTable.header = 0x14;
    getDepthTable.magic_word = 0xCDAB;
    getDepthTable.opcode = 0x15;
    getDepthTable.params[0] = 0x1f;

    uint8_t orgDepthTable[256] {0};
    memcpy(hwmc, &getDepthTable, sizeof(struct HWMC));
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));
    ASSERT_TRUE(getDepthTable.opcode == *(hwmc + sizeof(struct HWMC)));
    memcpy(orgDepthTable, hwmc + sizeof(struct HWMC) + 4, sizeof(orgDepthTable));

    // Set depth calibration table
    uint8_t newDepthTable[256] = {
            0x03, 0x00, 0x1F, 0x00, 0xF0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x17, 0x74, 0x4C, 0xBC,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    };
    HWMC setDepthTable {0};
    setDepthTable.header = 0x114;
    setDepthTable.magic_word = 0xCDAB;
    setDepthTable.opcode = 0x62;
    setDepthTable.params[0] = 0x1f;
    memcpy(hwmc, &setDepthTable, sizeof(struct HWMC));
    memcpy(hwmc + sizeof(struct HWMC), newDepthTable, sizeof(newDepthTable));
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));

    // Get new depth calibration table and verify it was written
    memcpy(hwmc, &getDepthTable, sizeof(struct HWMC));
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));
    ASSERT_TRUE(getDepthTable.opcode == *(hwmc + sizeof(struct HWMC)));
    memcmp(orgDepthTable, hwmc + sizeof(struct HWMC) + 4, sizeof(orgDepthTable));

    // Get the rec params for all supported resolutions
    HWMC resParamsGet {0};
    resParamsGet.header = 0x14;
    resParamsGet.magic_word = 0xCDAB;
    resParamsGet.opcode = 0x7E;

    memcpy(hwmc, &resParamsGet, sizeof(struct HWMC));
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));

    close(fd);
}

#pragma pack(push, 1)
typedef struct SubPresetHeader
{
    uint8_t  headerSize;
    uint8_t  id;
    uint16_t iterations;
    uint8_t  numOfItems;
}SubPresetHeader;

typedef struct SubPresetItemHeader
{
    uint8_t  headerSize;
    uint16_t iterations;
    uint8_t  numOfControls;
}SubPresetItemHeader;

typedef struct SubPresetControl
{
    uint8_t  controlId;
    uint32_t controlValue;
}SubPresetControl;
#pragma pack(pop)

// HDR
TEST_F(V4L2BasicTest, HDR) {
    string depthVideoNode = {"/dev/video0"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);

    struct v4l2_ext_control aeModeCtrl {0};
    aeModeCtrl.id = V4L2_CID_EXPOSURE_AUTO;
    aeModeCtrl.size = 0;
    aeModeCtrl.value = 0;

    // set auto exposure mode
    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &aeModeCtrl;
    ext.count = 1;
    int ret = ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext);
    ASSERT_TRUE(0 == ret);

    uint8_t hwmc[1028] {0};

    struct v4l2_ext_control ctrl {0};
    ctrl.id = DS5_CAMERA_CID_HWMC;
    ctrl.size = sizeof(hwmc);
    ctrl.p_u8 = hwmc;

    //struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &ctrl;
    ext.count = 1;

    // Set SETSUBPRESET HWMC
    HWMC setSubPreset {0};
    setSubPreset.header = 0x14;
    setSubPreset.magic_word = 0xCDAB;
    setSubPreset.opcode = 0x7B; // SETSUBPRESET opcode

    struct SubPresetControl HDRs[3] = {
            {
                    .controlId = 0x1,
                    .controlValue = 0xa
            },
            {
                    .controlId = 0x1,
                    .controlValue = 510
            },
            {
                    .controlId = 0x1,
                    .controlValue = 1660
            },
    };

    struct SubPresetItemHeader itemHeader = {
            .headerSize = 0x4,
            .iterations = 0x01,
            .numOfControls = 0x1
    };

    struct SubPresetHeader subPresetHeader = {
            .headerSize = 0x5,
            .id = 0x1,
            .iterations = 0x0, // infinite loop
            .numOfItems = sizeof(HDRs)/sizeof(struct SubPresetControl) // number of required HDRs, each item include one exposure value
    };

    size_t offset = 0;
    offset += sizeof(struct HWMC);

    memcpy(hwmc + offset, &subPresetHeader, sizeof(struct SubPresetHeader));
    offset += sizeof(struct SubPresetHeader);

    for (int i = 0; i < sizeof(HDRs)/sizeof(struct SubPresetControl); i++) {
        memcpy(hwmc + offset, &itemHeader, sizeof(struct SubPresetItemHeader));
        offset += sizeof(struct SubPresetItemHeader);
        memcpy(hwmc + offset, &HDRs[i], sizeof(struct SubPresetControl));
        offset += sizeof(struct SubPresetControl);
    }
  
    setSubPreset.header += offset - sizeof(struct HWMC);
    setSubPreset.params[0] = offset - sizeof(struct HWMC);
    memcpy(hwmc, &setSubPreset, sizeof(struct HWMC));
    printf("HDR preset size is %d, %d\n", offset - sizeof(struct HWMC), sizeof(struct HWMC));
 
    for(int i = 0; i < offset; i++) {
        printf(" %02x", hwmc[i]);
    }

    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));
for(int i = 0; i < offset; i++) {
        printf(" %02x", hwmc[i]);
    }

std::this_thread::sleep_for (std::chrono::seconds(1));
for(int i = 0; i < offset; i++) {
        printf(" %02x", hwmc[i]);
    }

    close(fd);
}

// Reset
/*TEST_F(V4L2BasicTest, RST) {
    string depthVideoNode = {"/dev/video0"};
    int fd = open(depthVideoNode.c_str(), O_RDWR);

    uint8_t hwmc[1028] {0};

    struct v4l2_ext_control ctrl {0};
    ctrl.id = DS5_CAMERA_CID_HWMC;
    ctrl.size = sizeof(hwmc);
    ctrl.p_u8 = hwmc;

    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &ctrl;
    ext.count = 1;

    // Reset HWMC
    HWMC reset {0};
    reset.header = 0x14;
    reset.magic_word = 0xCDAB;
    reset.opcode = 0x20; // RESET opcode
    reset.params[0] = 0;

    memcpy(hwmc, &reset, sizeof(struct HWMC));
    ASSERT_TRUE(0 == ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext));
    std::this_thread::sleep_for (std::chrono::seconds(1));
    close(fd);
}
*/
