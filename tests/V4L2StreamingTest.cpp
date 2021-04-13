/*
 * Copyright © 2020 Intel Corporation. All rights reserved.
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
#include <chrono>
#include <ctime>
#include <sys/mman.h>
#include <array>

#include "MetaData.h"

using namespace std;

//based on open source implementation:
//license verified by Yanir Lubetkin

//quote of the license:

/* Copyright (C) 1986 Gary S. Brown.  You may use this program, or
   code or tables extracted from it, as desired without restriction.*/

//original source resides in:
//http://stackoverflow.com/questions/302914/crc32-c-or-c-implementation
//http://web.archive.org/web/20080303093609/http://c.snippets.org/snip_lister.php?fname=crc.h
//http://web.archive.org/web/20100818224146/http://c.snippets.org/snip_lister.php?fname=crc_32.c

//#include "crc.h"

#define UPDC32(octet, crc) (crc_32_tab[((crc) ^ (octet)) & 0xff] ^ ((crc) >> 8))

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

static const uint32_t crc_32_tab[] __attribute__((section(".rodata"))) = { /* CRC polynomial 0xedb88320 */
0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};



uint32_t crc32buf(uint8_t *buf, int len)
{
    uint32_t oldcrc32 = 0xFFFFFFFF;
    for ( ; len; --len, ++buf)
        oldcrc32 = UPDC32(*buf, oldcrc32);
    return ~oldcrc32;
}

static uint32_t s_crcval = 0;

void crc32init(void)
{
    s_crcval = 0xFFFFFFFF;
}

uint32_t crc32chunk(uint8_t *buf, int len)
{
    uint32_t oldcrc32 = s_crcval;
    for ( ; len; --len, ++buf)
        oldcrc32 = UPDC32(*buf, oldcrc32);
    s_crcval = oldcrc32;
    return s_crcval;
}

uint32_t crc32complete(void)
{
    return ~s_crcval;
}


#define DS5_STREAM_CONFIG_0                  0x4000
#define DS5_CAMERA_CID_BASE                 (V4L2_CTRL_CLASS_CAMERA | DS5_STREAM_CONFIG_0)
#define DS5_CAMERA_CID_LASER_POWER          (DS5_CAMERA_CID_BASE+1)
#define DS5_CAMERA_CID_MANUAL_LASER_POWER   (DS5_CAMERA_CID_BASE+2)

class V4L2StreamTest : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

public:
    void setFmt(uint8_t fd, uint32_t pxlFmt, uint32_t w, uint32_t h)
    {
        v4l2_format v4l2Format;
        v4l2Format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2Format.fmt.pix.pixelformat = pxlFmt;
        v4l2Format.fmt.pix.width = w;
        v4l2Format.fmt.pix.height = h;
        int ret = ioctl(fd, VIDIOC_S_FMT, &v4l2Format);
        ASSERT_TRUE(0 == ret);
    }

    void setFPS(uint8_t fd, uint32_t fps)
    {
        struct v4l2_streamparm setFps {0};
        setFps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        setFps.parm.capture.timeperframe.numerator= 1;
        setFps.parm.capture.timeperframe.denominator = fps;
        int ret = ioctl(fd, VIDIOC_S_PARM, &setFps);
        ASSERT_TRUE(0 == ret);
    }

    void requestBuffers(uint8_t fd, uint32_t type, uint32_t memory, uint32_t count)
    {
        struct v4l2_requestbuffers v4L2ReqBufferrs {0};
        v4L2ReqBufferrs.type = type ;
        v4L2ReqBufferrs.memory = memory;
        v4L2ReqBufferrs.count = count;
        int ret = ioctl(fd, VIDIOC_REQBUFS, &v4L2ReqBufferrs);
        ASSERT_TRUE(0 == ret);
        ASSERT_TRUE(0 < v4L2ReqBufferrs.count);

    }

    void* queryMapQueueBuf(uint8_t fd, uint32_t type, uint32_t memory, uint8_t index, uint32_t size)
    {
        struct v4l2_buffer v4l2Buffer {0};
        v4l2Buffer.type = type;
        v4l2Buffer.memory = memory;
        v4l2Buffer.index = index;
        int ret = ioctl(fd, VIDIOC_QUERYBUF, &v4l2Buffer);
        if (ret)
            return nullptr;

        void *buffer = (struct buffer*)mmap(nullptr,
                                            size,
                                            PROT_READ | PROT_WRITE,
                                            MAP_SHARED,
                                            fd,
                                            v4l2Buffer.m.offset);
        if (nullptr == buffer)
            return nullptr;

        // queue buffers
        ret = ioctl(fd, VIDIOC_QBUF, &v4l2Buffer);
        if(ret)
            nullptr;

        return buffer;
    }
};

TEST_F(V4L2StreamTest, StreamDepth_640x480_5FPS) {
    string mdVideoNode = {"/dev/video0"};
    int depth_fd = open(mdVideoNode.c_str(), O_RDWR);
    ASSERT_TRUE(0 < depth_fd);

    // set format
    uint32_t width = 640;
    uint32_t height = 480;
    setFmt(depth_fd, V4L2_PIX_FMT_Z16, width, height);

    // set FPS
    setFPS(depth_fd, 5);

    // request buffers
    requestBuffers(depth_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP, 8);

    // query/map/queue buffers
    std::array<void*, 8> buffers {};
    for (int i = 0; i < buffers.size(); ++i) {
        buffers[i] = queryMapQueueBuf(depth_fd,
                                      V4L2_BUF_TYPE_VIDEO_CAPTURE,
                                      V4L2_MEMORY_MMAP,
                                      i,
                                      2 * width * height);
        ASSERT_TRUE(nullptr != buffers[i]);
    }

    // VIDIOC_STREAMON
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
    int ret = ioctl(depth_fd, VIDIOC_STREAMON, &type);
    ASSERT_TRUE(0 == ret);

    // dqueue/queue buffers
    long lastFrameTS {0};
    for (int i = 0; i < 15; ++i) {
        struct v4l2_buffer v4l2Buffer {0};
        v4l2Buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
        v4l2Buffer.memory = V4L2_MEMORY_MMAP;
        int ret = ioctl(depth_fd, VIDIOC_DQBUF, &v4l2Buffer);
        ASSERT_TRUE(0 == ret);
        ASSERT_TRUE(v4l2Buffer.type == V4L2_BUF_TYPE_VIDEO_CAPTURE);
        ASSERT_TRUE(v4l2Buffer.length == 2 * width * height);
        ASSERT_TRUE(v4l2Buffer.bytesused == 2 * width * height);
        ASSERT_TRUE(i == v4l2Buffer.sequence);

        if (i < 14) {
            ioctl(depth_fd, VIDIOC_QBUF, &v4l2Buffer);
            ASSERT_TRUE(0 == ret);
        }
    }

    // VIDIOC_STREAMOFF
    ret = ioctl(depth_fd, VIDIOC_STREAMOFF, &type);
    ASSERT_TRUE(0 == ret);

    close(depth_fd);
}

TEST_F(V4L2StreamTest, StreamDepth_640x480_30FPS) {
    string mdVideoNode = {"/dev/video0"};
    int depth_fd = open(mdVideoNode.c_str(), O_RDWR);
    ASSERT_TRUE(0 < depth_fd);


    // set format
    uint32_t width = 640;
    uint32_t height = 480;
    setFmt(depth_fd, V4L2_PIX_FMT_Z16, width, height);

    // set FPS
    setFPS(depth_fd, 30);

    // request buffers
    requestBuffers(depth_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP, 8);

    // query/map/queue buffers
    std::array<void*, 8> buffers {};
    for (int i = 0; i < buffers.size(); ++i) {
        buffers[i] = queryMapQueueBuf(depth_fd,
                                      V4L2_BUF_TYPE_VIDEO_CAPTURE,
                                      V4L2_MEMORY_MMAP,
                                      i,
                                      2 * width * height);
        ASSERT_TRUE(nullptr != buffers[i]);
    }

    // VIDIOC_STREAMON
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
    int ret = ioctl(depth_fd, VIDIOC_STREAMON, &type);
    ASSERT_TRUE(0 == ret);

    // dqueue/queue buffers
    long lastFrameTS {0};
    for (int i = 0; i < 15; ++i) {
        struct v4l2_buffer v4l2Buffer {0};
        v4l2Buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
        v4l2Buffer.memory = V4L2_MEMORY_MMAP;
        int ret = ioctl(depth_fd, VIDIOC_DQBUF, &v4l2Buffer);
        ASSERT_TRUE(0 == ret);
        ASSERT_TRUE(v4l2Buffer.type == V4L2_BUF_TYPE_VIDEO_CAPTURE);
        ASSERT_TRUE(v4l2Buffer.length == 2 * width * height);
        ASSERT_TRUE(v4l2Buffer.bytesused == 2 * width * height);
        ASSERT_TRUE(i == v4l2Buffer.sequence);

        if (i < 14) {
            ioctl(depth_fd, VIDIOC_QBUF, &v4l2Buffer);
            ASSERT_TRUE(0 == ret);
        }
    }

    // VIDIOC_STREAMOFF
    ret = ioctl(depth_fd, VIDIOC_STREAMOFF, &type);
    ASSERT_TRUE(0 == ret);

    close(depth_fd);
}

TEST_F(V4L2StreamTest, StreamDepth_640x480_5FPS_MetaData) {
    string mdVideoNode = {"/dev/video0"};
    int depth_fd = open(mdVideoNode.c_str(), O_RDWR);
    ASSERT_TRUE(0 < depth_fd);
    mdVideoNode = "/dev/video1";
    int md_fd = open(mdVideoNode.c_str(), O_RDWR);
    ASSERT_TRUE(0 < md_fd);

    // set format
    uint32_t width = 640;
    uint32_t height = 480;
    setFmt(depth_fd, V4L2_PIX_FMT_Z16, width, height);

    // set FPS
    setFPS(depth_fd, 5);

    // request buffers
    requestBuffers(depth_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP, 8);
    requestBuffers(md_fd, V4L2_BUF_TYPE_META_CAPTURE, V4L2_MEMORY_MMAP, 8);

    std::array<void*, 8> depthBuffers {};
    std::array<void*, 8> metaDataBuffers {};
    for (int i = 0; i < depthBuffers.size(); ++i) {
        depthBuffers[i] = queryMapQueueBuf(depth_fd,
                                           V4L2_BUF_TYPE_VIDEO_CAPTURE,
                                           V4L2_MEMORY_MMAP,
                                           i,
                                           2 * width * height);
        metaDataBuffers[i] = queryMapQueueBuf(md_fd,
                                              V4L2_BUF_TYPE_META_CAPTURE,
                                              V4L2_MEMORY_MMAP,
                                              i,
                                              4096);
        ASSERT_TRUE(nullptr != depthBuffers[i]);
        ASSERT_TRUE(nullptr != metaDataBuffers[i]);
    }

    // VIDIOC_STREAMON
    enum v4l2_buf_type vType = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
    enum v4l2_buf_type mdType = V4L2_BUF_TYPE_META_CAPTURE ;
    int ret = ioctl(md_fd, VIDIOC_STREAMON, &mdType);
    ret = ioctl(depth_fd, VIDIOC_STREAMON, &vType);
    ASSERT_TRUE(0 == ret);

    for (int i = 0; i < 100; ++i) {
        struct v4l2_buffer depthV4l2Buffer {0};
        depthV4l2Buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
        depthV4l2Buffer.memory = V4L2_MEMORY_MMAP;

        struct v4l2_buffer mdV4l2Buffer {0};
        mdV4l2Buffer.type = V4L2_BUF_TYPE_META_CAPTURE ;
        mdV4l2Buffer.memory = V4L2_MEMORY_MMAP;

        int ret = ioctl(depth_fd, VIDIOC_DQBUF, &depthV4l2Buffer);
        ret = ioctl(md_fd, VIDIOC_DQBUF, &mdV4l2Buffer);
        cout << "depth bytes used " << depthV4l2Buffer.bytesused << endl;
        cout << "depth sequence " << depthV4l2Buffer.sequence << endl;
        cout << "depth v4l2 ts " << depthV4l2Buffer.timestamp.tv_sec << "." <<
                depthV4l2Buffer.timestamp.tv_usec << endl;
        cout << "meta v4l2 bytes used " << mdV4l2Buffer.bytesused << endl;
        cout << "meta v4l2 sequence " << mdV4l2Buffer.sequence << endl;
        cout << "meta v4l2 ts " << mdV4l2Buffer.timestamp.tv_sec << "." <<
                mdV4l2Buffer.timestamp.tv_usec << endl;
        STMetaDataDepthYNormalMode *ptr = static_cast<STMetaDataDepthYNormalMode*>(
                metaDataBuffers[mdV4l2Buffer.index]);
        cout << "meta data hw ts "<< dec << ptr->captureStats.hwTimestamp << endl;
        cout << "meta data exposure time "<< dec << ptr->captureStats.ExposureTime << endl;
        cout << "meta data gain "<< dec << ptr->intelDepthControl.manualGain << endl;
        cout << "meta data projector mode "<< dec << (uint16_t)ptr->intelDepthControl.projectorMode << endl;
        cout << "meta data projector value "<< dec << ptr->intelDepthControl.laserPower << endl;
        cout << "meta data frame counter "<< dec << ptr->intelCaptureTiming.frameCounter << endl;
        cout << "meta data crc32 "<< dec << ptr->crc32 << endl;
        uint32_t crc = crc32buf(static_cast<uint8_t*>(metaDataBuffers[mdV4l2Buffer.index]), sizeof(STMetaDataDepthYNormalMode) - 4);
        ASSERT_TRUE(crc == ptr->crc32);

        if (i < 99) {
            ioctl(depth_fd, VIDIOC_QBUF, &depthV4l2Buffer);
            ioctl(md_fd, VIDIOC_QBUF, &mdV4l2Buffer);
        }
    }

    // VIDIOC_STREAMOFF
    ret = ioctl(md_fd, VIDIOC_STREAMOFF, &mdType);
    ret = ioctl(depth_fd, VIDIOC_STREAMOFF, &vType);

    close(md_fd);
    close(depth_fd);
}

TEST_F(V4L2StreamTest, StreamDepth_640x480_30FPS_MetaData) {
    string mdVideoNode = {"/dev/video0"};
    int depth_fd = open(mdVideoNode.c_str(), O_RDWR);
    ASSERT_TRUE(0 < depth_fd);
    mdVideoNode = "/dev/video1";
    int md_fd = open(mdVideoNode.c_str(), O_RDWR);
    ASSERT_TRUE(0 < md_fd);

    // set format
    uint32_t width = 640;
    uint32_t height = 480;
    setFmt(depth_fd, V4L2_PIX_FMT_Z16, width, height);

    // set FPS
    setFPS(depth_fd, 30);

    // request buffers
    requestBuffers(depth_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP, 8);
    requestBuffers(md_fd, V4L2_BUF_TYPE_META_CAPTURE, V4L2_MEMORY_MMAP, 8);

    std::array<void*, 8> depthBuffers {};
    std::array<void*, 8> metaDataBuffers {};
    for (int i = 0; i < depthBuffers.size(); ++i) {
        depthBuffers[i] = queryMapQueueBuf(depth_fd,
                                           V4L2_BUF_TYPE_VIDEO_CAPTURE,
                                           V4L2_MEMORY_MMAP,
                                           i,
                                           2 * width * height);
        metaDataBuffers[i] = queryMapQueueBuf(md_fd,
                                              V4L2_BUF_TYPE_META_CAPTURE,
                                              V4L2_MEMORY_MMAP,
                                              i,
                                              4096);
        ASSERT_TRUE(nullptr != depthBuffers[i]);
        ASSERT_TRUE(nullptr != metaDataBuffers[i]);
    }

    // VIDIOC_STREAMON
    enum v4l2_buf_type vType = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
    enum v4l2_buf_type mdType = V4L2_BUF_TYPE_META_CAPTURE ;
    int ret = ioctl(md_fd, VIDIOC_STREAMON, &mdType);
    ret = ioctl(depth_fd, VIDIOC_STREAMON, &vType);
    ASSERT_TRUE(0 == ret);

    for (int i = 0; i < 30000; ++i) {
        struct v4l2_buffer depthV4l2Buffer {0};
        depthV4l2Buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
        depthV4l2Buffer.memory = V4L2_MEMORY_MMAP;

        struct v4l2_buffer mdV4l2Buffer {0};
        mdV4l2Buffer.type = V4L2_BUF_TYPE_META_CAPTURE ;
        mdV4l2Buffer.memory = V4L2_MEMORY_MMAP;

        int ret = ioctl(depth_fd, VIDIOC_DQBUF, &depthV4l2Buffer);
        ret = ioctl(md_fd, VIDIOC_DQBUF, &mdV4l2Buffer);
        cout << "depth bytes used " << depthV4l2Buffer.bytesused << endl;
        cout << "depth sequence " << depthV4l2Buffer.sequence << endl;
        cout << "depth v4l2 ts " << depthV4l2Buffer.timestamp.tv_sec << "." <<
                depthV4l2Buffer.timestamp.tv_usec << endl;
        cout << "meta v4l2 bytes used " << mdV4l2Buffer.bytesused << endl;
        cout << "meta v4l2 sequence " << mdV4l2Buffer.sequence << endl;
        cout << "meta v4l2 ts " << mdV4l2Buffer.timestamp.tv_sec << "." <<
                mdV4l2Buffer.timestamp.tv_usec << endl;
        STMetaDataDepthYNormalMode *ptr = static_cast<STMetaDataDepthYNormalMode*>(
                metaDataBuffers[mdV4l2Buffer.index]);
        cout << "meta data hw ts "<< dec << ptr->captureStats.hwTimestamp << endl;
        cout << "meta data exposure time "<< dec << ptr->captureStats.ExposureTime << endl;
        cout << "meta data gain "<< dec << ptr->intelDepthControl.manualGain << endl;
        cout << "meta data projector mode "<< dec << (uint16_t)ptr->intelDepthControl.projectorMode << endl;
        cout << "meta data projector value "<< dec << ptr->intelDepthControl.laserPower << endl;
        cout << "meta data frame counter "<< dec << ptr->intelCaptureTiming.frameCounter << endl;
        cout << "meta data crc32 "<< dec << ptr->crc32 << endl;
        uint32_t crc = crc32buf(static_cast<uint8_t*>(metaDataBuffers[mdV4l2Buffer.index]), sizeof(STMetaDataDepthYNormalMode) - 4);
        ASSERT_TRUE(crc == ptr->crc32);

        if (i < 29999) {
            ioctl(depth_fd, VIDIOC_QBUF, &depthV4l2Buffer);
            ioctl(md_fd, VIDIOC_QBUF, &mdV4l2Buffer);
        }
    }

    // VIDIOC_STREAMOFF
    ret = ioctl(md_fd, VIDIOC_STREAMOFF, &mdType);
    ret = ioctl(depth_fd, VIDIOC_STREAMOFF, &vType);

    close(md_fd);
    close(depth_fd);
}

#pragma pack(push, 1)
struct HWMC {
    uint16_t header;
    uint16_t magic_word;
    uint32_t opcode;
    uint32_t params[4];
};

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

static int stream30Frames(int depth_fd, int md_fd, std::array<void*, 8> metaDataBuffers, bool lastStream)
{
    for (int i = 0; i < 30; ++i) {
        struct v4l2_buffer depthV4l2Buffer {0};
        depthV4l2Buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
        depthV4l2Buffer.memory = V4L2_MEMORY_MMAP;

        struct v4l2_buffer mdV4l2Buffer {0};
        mdV4l2Buffer.type = V4L2_BUF_TYPE_META_CAPTURE ;
        mdV4l2Buffer.memory = V4L2_MEMORY_MMAP;

        int ret = ioctl(depth_fd, VIDIOC_DQBUF, &depthV4l2Buffer);
        ret = ioctl(md_fd, VIDIOC_DQBUF, &mdV4l2Buffer);
        cout << "depth sequence " << depthV4l2Buffer.sequence << endl;
        cout << "meta v4l2 sequence " << mdV4l2Buffer.sequence << endl;

        STMetaDataDepthYNormalMode *ptr = static_cast<STMetaDataDepthYNormalMode*>(
                metaDataBuffers[mdV4l2Buffer.index]);
        cout << "meta data exposure time "<< dec << ptr->captureStats.ExposureTime << endl;
        cout << "meta data frame counter "<< dec << ptr->intelCaptureTiming.frameCounter << endl;

        ioctl(depth_fd, VIDIOC_QBUF, &depthV4l2Buffer);
        ioctl(md_fd, VIDIOC_QBUF, &mdV4l2Buffer);

	// don't queue the last buffer on last stream
	if (lastStream && i < 29) {
            ioctl(depth_fd, VIDIOC_QBUF, &depthV4l2Buffer);
            ioctl(md_fd, VIDIOC_QBUF, &mdV4l2Buffer);
        }
    }

    return 0;
}

static int setHDR(int fd, uint32_t *expValues, uint8_t numOfExposures)
{
    struct v4l2_ext_control aeModeCtrl {0};
    aeModeCtrl.id = V4L2_CID_EXPOSURE_AUTO;
    aeModeCtrl.size = 0;
    aeModeCtrl.value = 0;

    // set auto exposure mode off
    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &aeModeCtrl;
    ext.count = 1;
    int ret = ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext);
    
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

    struct SubPresetControl HDRs[10];
    for (int i = 0; i < numOfExposures; i++) {
        HDRs[i].controlId = 1;
        HDRs[i].controlValue = *(expValues + i);
    }

    struct SubPresetItemHeader itemHeader = {
            .headerSize = 0x4,
            .iterations = 0x01,
            .numOfControls = 0x1
    };

    struct SubPresetHeader subPresetHeader = {
            .headerSize = 0x5,
            .id = 0x1,
            .iterations = 0x0, // infinite loop
            .numOfItems = numOfExposures // number of required HDRs, each item include one exposure value
    };

    size_t offset = 0;
    offset += sizeof(struct HWMC);

    memcpy(hwmc + offset, &subPresetHeader, sizeof(struct SubPresetHeader));
    offset += sizeof(struct SubPresetHeader);

    for (int i = 0; i < numOfExposures; i++) {
        memcpy(hwmc + offset, &itemHeader, sizeof(struct SubPresetItemHeader));
        offset += sizeof(struct SubPresetItemHeader);
        memcpy(hwmc + offset, &HDRs[i], sizeof(struct SubPresetControl));
        offset += sizeof(struct SubPresetControl);
    }

    /*
	currently, when HWMC is of size odd, then the driver fails.
	in order to fix this, we check if the size is odd and add one byte to the total size
	of the command (its initialed zeros) to make it even.
	this will be fixed in the driver later.
    */
    setSubPreset.header += offset - sizeof(struct HWMC);
    if (setSubPreset.header%2 != 0)
	setSubPreset.header++;

    setSubPreset.params[0] = offset - sizeof(struct HWMC);
    memcpy(hwmc, &setSubPreset, sizeof(struct HWMC));

    if (0 != ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext)) {
	cout << "VIDIOC_S_EXT_CTRLS failed!" << endl;    
    }

    return 0;
}

TEST_F(V4L2StreamTest, StreamDepth_1280x960_30FPS_HDR_HMC) {
    string mdVideoNode = {"/dev/video0"};
    int depth_fd = open(mdVideoNode.c_str(), O_RDWR);
    ASSERT_TRUE(0 < depth_fd);
    mdVideoNode = "/dev/video1";
    int md_fd = open(mdVideoNode.c_str(), O_RDWR);
    ASSERT_TRUE(0 < md_fd);

    // set format
    uint32_t width = 1280;
    uint32_t height = 960;
    setFmt(depth_fd, V4L2_PIX_FMT_Z16, width, height);

    // set FPS
    setFPS(depth_fd, 30);

    // request buffers
    requestBuffers(depth_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP, 8);
    requestBuffers(md_fd, V4L2_BUF_TYPE_META_CAPTURE, V4L2_MEMORY_MMAP, 8);

    std::array<void*, 8> depthBuffers {};
    std::array<void*, 8> metaDataBuffers {};
    for (int i = 0; i < depthBuffers.size(); ++i) {
        depthBuffers[i] = queryMapQueueBuf(depth_fd,
                                           V4L2_BUF_TYPE_VIDEO_CAPTURE,
                                           V4L2_MEMORY_MMAP,
                                           i,
                                           2 * width * height);
        metaDataBuffers[i] = queryMapQueueBuf(md_fd,
                                              V4L2_BUF_TYPE_META_CAPTURE,
                                              V4L2_MEMORY_MMAP,
                                              i,
                                              4096);
        ASSERT_TRUE(nullptr != depthBuffers[i]);
        ASSERT_TRUE(nullptr != metaDataBuffers[i]);
    }

    // VIDIOC_STREAMON
    enum v4l2_buf_type vType = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
    enum v4l2_buf_type mdType = V4L2_BUF_TYPE_META_CAPTURE ;
    int ret = ioctl(md_fd, VIDIOC_STREAMON, &mdType);
    ret = ioctl(depth_fd, VIDIOC_STREAMON, &vType);
    ASSERT_TRUE(0 == ret);

    cout << endl << "trying HMC with 3 exposure values: 10, 510, 1660" << endl;

    uint32_t threeExpValues[3]= {10, 510, 1660};
    setHDR(depth_fd, threeExpValues, 3);

    stream30Frames(depth_fd, md_fd, metaDataBuffers, false);

    cout << endl << "HMC with 3 values finished, now trying with 2 values: 110, 1640" << endl;
    cout << "note, it takes about 3 frames to start the new HMC with the new values" << endl << endl;

    uint32_t twoExpValues[2]= {110, 1640};
    setHDR(depth_fd, twoExpValues, 2);

    stream30Frames(depth_fd, md_fd, metaDataBuffers, false);

    cout << endl << "HMC with 2 values finished, now trying with 4 values: 90, 130, 1600, 1620" << endl;
    cout << "note, it takes about 3 frames to start the new HMC with the new values" << endl << endl;

    uint32_t fourExpValues[4]= {90, 130, 1600, 1620};
    setHDR(depth_fd, fourExpValues, 4);

    stream30Frames(depth_fd, md_fd, metaDataBuffers, false);

    cout << endl << "HMC with 4 values finished, now trying with 6 values: 70, 150, 1540, 1560, 1580, 1660" << endl;
    cout << "note, it takes about 3 frames to start the new HMC with the new values" << endl << endl;

    uint32_t sixExpValues[6]= {70, 150, 1540, 1560, 1580, 1660};
    setHDR(depth_fd, sixExpValues, 6);

    stream30Frames(depth_fd, md_fd, metaDataBuffers, true);

    // VIDIOC_STREAMOFF
    ret = ioctl(md_fd, VIDIOC_STREAMOFF, &mdType);
    ret = ioctl(depth_fd, VIDIOC_STREAMOFF, &vType);

    close(md_fd);
    close(depth_fd);
}

TEST_F(V4L2StreamTest, StreamDepth_1280x960_30FPS_MetaData) {
    string mdVideoNode = {"/dev/video0"};
    int depth_fd = open(mdVideoNode.c_str(), O_RDWR);
    ASSERT_TRUE(0 < depth_fd);
    mdVideoNode = "/dev/video1";
    int md_fd = open(mdVideoNode.c_str(), O_RDWR);
    ASSERT_TRUE(0 < md_fd);

    // set format
    uint32_t width = 1280;
    uint32_t height = 960;
    setFmt(depth_fd, V4L2_PIX_FMT_Z16, width, height);

    // set FPS
    setFPS(depth_fd, 30);

    // request buffers
    requestBuffers(depth_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP, 8);
    requestBuffers(md_fd, V4L2_BUF_TYPE_META_CAPTURE, V4L2_MEMORY_MMAP, 8);

    std::array<void*, 8> depthBuffers {};
    std::array<void*, 8> metaDataBuffers {};
    for (int i = 0; i < depthBuffers.size(); ++i) {
        depthBuffers[i] = queryMapQueueBuf(depth_fd,
                                           V4L2_BUF_TYPE_VIDEO_CAPTURE,
                                           V4L2_MEMORY_MMAP,
                                           i,
                                           2 * width * height);
        metaDataBuffers[i] = queryMapQueueBuf(md_fd,
                                              V4L2_BUF_TYPE_META_CAPTURE,
                                              V4L2_MEMORY_MMAP,
                                              i,
                                              4096);
        ASSERT_TRUE(nullptr != depthBuffers[i]);
        ASSERT_TRUE(nullptr != metaDataBuffers[i]);
    }

    // VIDIOC_STREAMON
    enum v4l2_buf_type vType = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
    enum v4l2_buf_type mdType = V4L2_BUF_TYPE_META_CAPTURE ;
    int ret = ioctl(md_fd, VIDIOC_STREAMON, &mdType);
    ret = ioctl(depth_fd, VIDIOC_STREAMON, &vType);
    ASSERT_TRUE(0 == ret);

    for (int i = 0; i < 30000; ++i) {
        struct v4l2_buffer depthV4l2Buffer {0};
        depthV4l2Buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
        depthV4l2Buffer.memory = V4L2_MEMORY_MMAP;

        struct v4l2_buffer mdV4l2Buffer {0};
        mdV4l2Buffer.type = V4L2_BUF_TYPE_META_CAPTURE ;
        mdV4l2Buffer.memory = V4L2_MEMORY_MMAP;

        int ret = ioctl(depth_fd, VIDIOC_DQBUF, &depthV4l2Buffer);
        ret = ioctl(md_fd, VIDIOC_DQBUF, &mdV4l2Buffer);
        cout << "depth bytes used " << depthV4l2Buffer.bytesused << endl;
        cout << "depth sequence " << depthV4l2Buffer.sequence << endl;
        cout << "depth v4l2 ts " << depthV4l2Buffer.timestamp.tv_sec << "." <<
                depthV4l2Buffer.timestamp.tv_usec << endl;
        cout << "meta v4l2 bytes used " << mdV4l2Buffer.bytesused << endl;
        cout << "meta v4l2 sequence " << mdV4l2Buffer.sequence << endl;
        cout << "meta v4l2 ts " << mdV4l2Buffer.timestamp.tv_sec << "." <<
                mdV4l2Buffer.timestamp.tv_usec << endl;
        STMetaDataDepthYNormalMode *ptr = static_cast<STMetaDataDepthYNormalMode*>(
                metaDataBuffers[mdV4l2Buffer.index]);
        cout << "meta data hw ts "<< dec << ptr->captureStats.hwTimestamp << endl;
        cout << "meta data exposure time "<< dec << ptr->captureStats.ExposureTime << endl;
        cout << "meta data gain "<< dec << ptr->intelDepthControl.manualGain << endl;
        cout << "meta data projector mode "<< dec << (uint16_t)ptr->intelDepthControl.projectorMode << endl;
        cout << "meta data projector value "<< dec << ptr->intelDepthControl.laserPower << endl;
        cout << "meta data frame counter "<< dec << ptr->intelCaptureTiming.frameCounter << endl;
        cout << "meta data crc32 "<< dec << ptr->crc32 << endl;
        uint32_t crc = crc32buf(static_cast<uint8_t*>(metaDataBuffers[mdV4l2Buffer.index]), sizeof(STMetaDataDepthYNormalMode) - 4);
        ASSERT_TRUE(crc == ptr->crc32);

        if (i < 29999) {
            ioctl(depth_fd, VIDIOC_QBUF, &depthV4l2Buffer);
            ioctl(md_fd, VIDIOC_QBUF, &mdV4l2Buffer);
        }
    }

    // VIDIOC_STREAMOFF
    ret = ioctl(md_fd, VIDIOC_STREAMOFF, &mdType);
    ret = ioctl(depth_fd, VIDIOC_STREAMOFF, &vType);

    close(md_fd);
    close(depth_fd);
}

TEST_F(V4L2StreamTest, StreamRGB_1920x1080_5FPS) {
    string mdVideoNode = {"/dev/video2"};
    int rgb_fd = open(mdVideoNode.c_str(), O_RDWR);
    ASSERT_TRUE(0 < rgb_fd);

    // set format
    uint32_t width = 1920;
    uint32_t height = 1080;
    setFmt(rgb_fd, V4L2_PIX_FMT_YUYV, width, height);

    // set FPS
    setFPS(rgb_fd, 5);

    // request buffers
    requestBuffers(rgb_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP, 8);

    // query/map/queue buffers
    std::array<void*, 8> buffers {};
    for (int i = 0; i < buffers.size(); ++i) {
        buffers[i] = queryMapQueueBuf(rgb_fd,
                                      V4L2_BUF_TYPE_VIDEO_CAPTURE,
                                      V4L2_MEMORY_MMAP,
                                      i,
                                      2 * width * height);
        ASSERT_TRUE(nullptr != buffers[i]);
    }

    // VIDIOC_STREAMON
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
    int ret = ioctl(rgb_fd, VIDIOC_STREAMON, &type);
    ASSERT_TRUE(0 == ret);

    // dqueue/queue buffers
    long lastFrameTS {0};
    for (int i = 0; i < 15; ++i) {
            struct v4l2_buffer v4l2Buffer {0};
            v4l2Buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
            v4l2Buffer.memory = V4L2_MEMORY_MMAP;
            int ret = ioctl(rgb_fd, VIDIOC_DQBUF, &v4l2Buffer);
            ASSERT_TRUE(0 == ret);
            ASSERT_TRUE(v4l2Buffer.type == V4L2_BUF_TYPE_VIDEO_CAPTURE);
            ASSERT_TRUE(v4l2Buffer.length == 2 * width * height);
            ASSERT_TRUE(v4l2Buffer.bytesused == 2 * width * height);
            ASSERT_TRUE(i == v4l2Buffer.sequence);

            if (i < 14) {
                ioctl(rgb_fd, VIDIOC_QBUF, &v4l2Buffer);
                ASSERT_TRUE(0 == ret);
            }
    }

    // VIDIOC_STREAMOFF
    ret = ioctl(rgb_fd, VIDIOC_STREAMOFF, &type);
    ASSERT_TRUE(0 == ret);

    close(rgb_fd);
}

TEST_F(V4L2StreamTest, StreamRGB_1920x1080_30FPS) {
    string mdVideoNode = {"/dev/video2"};
    int rgb_fd = open(mdVideoNode.c_str(), O_RDWR);
    ASSERT_TRUE(0 < rgb_fd);

    // set format
    uint32_t width = 1920;
    uint32_t height = 1080;
    setFmt(rgb_fd, V4L2_PIX_FMT_YUYV, width, height);

    // set FPS
    setFPS(rgb_fd, 5);

    // request buffers
    requestBuffers(rgb_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP, 8);

    // query/map/queue buffers
    std::array<void*, 8> buffers {};
    for (int i = 0; i < buffers.size(); ++i) {
        buffers[i] = queryMapQueueBuf(rgb_fd,
                                      V4L2_BUF_TYPE_VIDEO_CAPTURE,
                                      V4L2_MEMORY_MMAP,
                                      i,
                                      2 * width * height);
        ASSERT_TRUE(nullptr != buffers[i]);
    }

    // VIDIOC_STREAMON
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
    int ret = ioctl(rgb_fd, VIDIOC_STREAMON, &type);
    ASSERT_TRUE(0 == ret);

    // dqueue/queue buffers
    long lastFrameTS {0};
    for (int i = 0; i < 1; ++i) {
            struct v4l2_buffer v4l2Buffer {0};
            v4l2Buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
            v4l2Buffer.memory = V4L2_MEMORY_MMAP;
            int ret = ioctl(rgb_fd, VIDIOC_DQBUF, &v4l2Buffer);
            ASSERT_TRUE(0 == ret);
            ASSERT_TRUE(v4l2Buffer.type == V4L2_BUF_TYPE_VIDEO_CAPTURE);
            ASSERT_TRUE(v4l2Buffer.length == 2 * width * height);
            ASSERT_TRUE(v4l2Buffer.bytesused == 2 * width * height);
            ASSERT_TRUE(i == v4l2Buffer.sequence);

            if (i < 14) {
                ioctl(rgb_fd, VIDIOC_QBUF, &v4l2Buffer);
                ASSERT_TRUE(0 == ret);
            }
    }

    // VIDIOC_STREAMOFF
    ret = ioctl(rgb_fd, VIDIOC_STREAMOFF, &type);
    ASSERT_TRUE(0 == ret);

    close(rgb_fd);
}
