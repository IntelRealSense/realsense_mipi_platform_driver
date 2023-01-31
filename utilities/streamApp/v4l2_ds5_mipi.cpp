
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

#include <array>
#include <string>
#include <thread>
#include <iostream>
#include <getopt.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>

#include "RealsenseLog.h"
#include "StreamView.h"
#include "Stream.h"
#include "StreamUtils.h"

using namespace std;
using namespace realsense::utils;
using namespace realsense::camera_viewer;
using namespace realsense::camera_sub_system;

static void usage(const char *argv0)
{
    cout << "Usage: " << argv0 << " [options]" << endl;
    cout << "Supported options:\n" << endl;
    cout << "-d        number of the /dev/videoX, default 0" << endl;
    cout << "-t        stream type \"depth\", \"rgb\", \"y8\", \"y12i\", \"y8i\", default depth" << endl;
    cout << "-w        stream width, default 1280" << endl;
    cout << "-h        stream height, default 720" << endl;
    cout << "-f        stream fps, default 30" << endl;
    cout << "-s        slave mode, default false" << endl;
    cout << "--help    show this help screen" << endl;
}

static struct option opts[] = {
    {"device", 1, 0, 'd'},
    {"stream-type", 1, 0, 't'},
    {"fps", 1, 0, 'f'},
    {"width", 1, 0, 'w'},
    {"height", 1, 0, 'h'},
    {"slave-mode", 1, 0, 's'},
    {0, 0, 0, 0}
};

int main(int argc, char **argv)
{
    int c;
    string sOptArg{""};
    uint8_t nodeNumber {0};
    V4L2Utils::StreamUtils::StreamType streamType {V4L2Utils::StreamUtils::StreamType::RS_DEPTH_STREAM};
    uint32_t fps {30};
    uint32_t width{1280};
    uint32_t height{720};
    bool slaveMode{false};
    opterr = 0;

    if (argc == 2 && string(argv[1]) == "--help") {
        usage(argv[0]);
        return 0;
    }

    while ((c = getopt_long(argc, argv, "d:t:f:r:w:h:s:", opts, NULL)) != -1) {
        switch (c) {
        case 'd':
            sOptArg = string(optarg);
            nodeNumber = std::stoi(sOptArg);
            break;
        case 't':
            sOptArg = string(optarg);
            if ("depth" == sOptArg)
                streamType = V4L2Utils::StreamUtils::StreamType::RS_DEPTH_STREAM;
            if ("y8" == sOptArg)
                 streamType = V4L2Utils::StreamUtils::StreamType::RS_Y8_STREAM;
            if ("y8i" == sOptArg)
                 streamType = V4L2Utils::StreamUtils::StreamType::RS_Y8I_STREAM;
            if ("y12i" == sOptArg)
                streamType = V4L2Utils::StreamUtils::StreamType::RS_Y12I_STREAM;
            if ("rgb" == sOptArg)
                streamType = V4L2Utils::StreamUtils::StreamType::RS_RGB_STREAM;
            break;
        case 'f':
            sOptArg = string(optarg);
            fps = std::stoi(sOptArg);
            cout << "fps " << fps << endl;
            break;
        case 'w':
            sOptArg = string(optarg);
            width = std::stoi(sOptArg);
            cout << "width " << width << endl;
            break;
        case 'h':
            sOptArg = string(optarg);
            height = std::stoi(sOptArg);
            cout << "height " << height << endl;
            break;
        case 's':
            sOptArg = string(optarg);
            if ("on" == sOptArg)
                slaveMode = true;
            if ("off" == sOptArg)
                slaveMode = false;
            cout << "slave mode " << slaveMode << endl;
            break;
        default:
            cout << "Invalid option -" << c << endl;
            cout << "Run " << argv[0] << " --help for help" << endl;
            return 1;
        }
    }

    Stream* stream;
    StreamView* streamView;

    stream = new Stream{nodeNumber};
    streamView = new StreamView {nodeNumber, *stream, streamType};
    streamView->setResolution(width, height);
    streamView->setFPS(fps);
    streamView->setSlaveMode(slaveMode);
    streamView->draw();

    delete streamView;
    delete stream;
}
