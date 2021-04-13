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

#include <fstream>
#include <iterator>
#include <vector>

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

#define PRESET_CFG	    0x6F
#define PRESET_APPLY	0x70
#define PRESET_QUERY	0x71
#define PRESET_GET  	0x72

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

#pragma pack(push, 1)
struct HWMC {
    uint16_t header;
    uint16_t magic_word;
    uint32_t opcode;
    uint32_t params[4];
};
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

/**
 * function that creates the hardware monitor command according to the opcode and sends it
 * fd: file descriptor
 * opcode: the opcode to the hwmc (CFG, APPLY, QUERY, GET)
 * param1: first parameter in command header (if 0 then its not needed)
**/

static int sendHwmc(int fd, uint32_t opcode, uint32_t param1)
{
    uint8_t hwmc[1028] {0};

    struct v4l2_ext_control ctrl {0};
    ctrl.id = DS5_CAMERA_CID_HWMC;
    ctrl.size = sizeof(hwmc);
    ctrl.p_u8 = hwmc;

    struct v4l2_ext_controls ext {0};
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &ctrl;
    ext.count = 1;

    // Set HWMC
    HWMC setPreset {0};
    setPreset.header = 0x14;
    setPreset.magic_word = 0xCDAB;
    setPreset.opcode = opcode;

    size_t offset = 0;
    offset += sizeof(struct HWMC);

    switch (opcode) {
    case PRESET_CFG:
	{
	std::ifstream input( "/home/nvidia/tools/src/tests/AmazonExample.bin", std::ifstream::binary );
	input.seekg(0, input.end);
	int length = input.tellg();
	input.seekg(0, input.beg);

	char* buffer = new char[length];

	input.read(buffer, length);

	memcpy(hwmc + offset, buffer, length);
	offset += length;
	break;
	}
    default: break;
    }

    /*
	currently, when HWMC is of size odd, then the driver fails.
	in order to fix this, we check if the size is odd and add one byte to the total size
	of the command (its initialed zeros) to make it even.
	this will be fixed in the driver later.
    */
    setPreset.header += offset - sizeof(struct HWMC);
    if (setPreset.header%2 != 0)
	setPreset.header++;

    if (param1 != 0) {
        setPreset.params[0] = param1;
    }

    memcpy(hwmc, &setPreset, sizeof(struct HWMC));

    switch (opcode) {
    case PRESET_CFG:
	{
	cout << "sending PRESET CFG"<< endl;
	if (0 != ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext)) {
	    cout << "VIDIOC_S_EXT_CTRLS failed!" << endl;    
	}
	
	break;
	}
    case PRESET_APPLY:
	{
	cout << "sending PRESET APPLY"<< endl;
	if (0 != ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext)) {
	    cout << "VIDIOC_S_EXT_CTRLS failed!" << endl;    
	}
	
	break;
	}
    case PRESET_QUERY:
	{
	cout << "sending PRESET QUERY"<< endl;
	if (0 != ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext)) {
	    cout << "VIDIOC_G_EXT_CTRLS failed!" << endl;    
	}

	auto myfile = std::fstream("query.binary", std::ios::out | std::ios::binary);
	myfile.write((char*)ctrl.p_u8, 1028);

	break;
	}
    case PRESET_GET:
	{
	cout << "sending PRESET GET"<< endl;
	if (0 != ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext)) {
	    cout << "VIDIOC_G_EXT_CTRLS failed!" << endl;    
	}

	auto myfile = std::fstream("get.binary", std::ios::out | std::ios::binary);
	myfile.write((char*)ctrl.p_u8, 1028);

	break;
	}
    default: break;
    }

    return 0;
}

/**
 * 1. configures preset at index 101 and 102
 * 2. queries all presets
 * 3. gets the buffer of preset at index 102
 * 4. applies preset at index 101
**/

TEST_F(V4L2StreamTest, AmazonTest) {
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

    cout << endl << "configuring preset at index 101 & 102" << endl << endl;
    sendHwmc(depth_fd, PRESET_CFG, 101);
    sendHwmc(depth_fd, PRESET_CFG, 102);

    cout << endl << "quering all presets" << endl << endl;
    sendHwmc(depth_fd, PRESET_QUERY, 0);

    cout << endl << "getting buffer for preset at index 102" << endl << endl;
    sendHwmc(depth_fd, PRESET_GET, 102);

    cout << endl << "applying preset at index 101" << endl << endl;
    sendHwmc(depth_fd, PRESET_APPLY, 101);

    stream30Frames(depth_fd, md_fd, metaDataBuffers, false);

    // VIDIOC_STREAMOFF
    ret = ioctl(md_fd, VIDIOC_STREAMOFF, &mdType);
    ret = ioctl(depth_fd, VIDIOC_STREAMOFF, &vType);

    close(md_fd);
    close(depth_fd);
}


