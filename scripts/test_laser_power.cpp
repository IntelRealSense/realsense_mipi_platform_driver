#include <iostream>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/videodev2.h>

using namespace std;

#define DS5_STREAM_CONFIG_0                  0x4000
#define DS5_CAMERA_CID_BASE                 (V4L2_CTRL_CLASS_CAMERA | DS5_STREAM_CONFIG_0)
#define DS5_CAMERA_CID_LASER_POWER          (DS5_CAMERA_CID_BASE+1)
#define DS5_CAMERA_CID_MANUAL_LASER_POWER   (DS5_CAMERA_CID_BASE+2)

/*
takes 2 arguments:
1. /dev/videoX
2. 360 (laser value)
Example: ./test_laser_power /dev/video0 360
*/

int main(int argc, char** argv) {

    if (argc != 3) {
        cout << "Please enter 2 inputs" << endl;
        return 0;
    }

    int laserPowerValue = 0;
    char* end;

    int fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        cout << "failed to open " << argv[1] << endl;
        return 0;
    }

    struct v4l2_ext_control manualLaserCtrl { 0 };
    manualLaserCtrl.id = DS5_CAMERA_CID_MANUAL_LASER_POWER;
    manualLaserCtrl.value = strtol(argv[2], &end, 10);

    cout << "trying to set laser power to " << manualLaserCtrl.value << endl;
    // set laser power = 360mW
    struct v4l2_ext_controls ext { 0 };
    ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ext.controls = &manualLaserCtrl;
    ext.count = 1;
    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext) != 0) {
        cout << "VIDIOC_S_EXT_CTRLS failed" << endl;
    }

    // get laser power
    struct v4l2_ext_control currManualLaserCtrl {0};
    currManualLaserCtrl.id = DS5_CAMERA_CID_MANUAL_LASER_POWER;
    ext.controls = &currManualLaserCtrl;
    ext.count = 1;
    if (ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext) != 0) {
        cout << "VIDIOC_G_EXT_CTRLS failed" << endl;
    }

    if (currManualLaserCtrl.value != manualLaserCtrl.value) {
        cout << "Laser power set failed" << endl;
        cout << "cameras laser value: " << currManualLaserCtrl.value << endl;
        close(fd);
        return 0;
    }

    cout << "success - laser power was set to " << currManualLaserCtrl.value << endl;
    close(fd);
}