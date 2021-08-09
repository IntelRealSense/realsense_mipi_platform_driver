#include <iostream>
#include <stdlib.h>  // for strtol

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

int main(int argc, char** argv) {

    if (argc != 2) {
        cout << "Please enter laser power value" << endl;
        return 0;
    }
    
    int laserPowerValue = 0;
    char* end;

    int fd = open("/dev/video0", O_RDWR);
    struct v4l2_ext_control manualLaserCtrl { 0 };
    manualLaserCtrl.id = DS5_CAMERA_CID_MANUAL_LASER_POWER;
    manualLaserCtrl.value = strtol(argv[0], &end, 10);

    // set laser power = 360mW
    struct v4l2_ext_controls ext { 0 };
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &manualLaserCtrl;
    ext.count = 1;
    ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext);     
    

    // get laser power
    struct v4l2_ext_control currManualLaserCtrl {0};
    currManualLaserCtrl.id = DS5_CAMERA_CID_MANUAL_LASER_POWER;
    ext.controls = &currManualLaserCtrl;
    ext.count = 1;
    ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext);

    if (currManualLaserCtrl.value != manualLaserCtrl.value)
        cout << "Laser power set failed" << endl;

    cout << "success" << endl;
    close(fd);
}