
# Realsense Firmware Log Parser

## About

This script can parse log next format:
```text
logger:   15,    0,    0,    0,  160,   33,   38,   16,    7,    0,  228,   33,  224,    1,   80,    3,    0,    0,    0,    0,   87,  124,  108,  139,  160,   35,   10,   16,   65,    2,  161,   49,    0,    0,    0,    0,    0,    0,    0,    0,  151,  130,  108,  139,  160,   33,   63,   16,    8,    0,  230,   64,   15,    0,  135,   25,   48,    3,    0,    0,  213,  208,  250,  139,  160,   35,   10,   16,   65,    2,  161,   81,    0,    0,    0,    0,    0,    0,    0,    0,  191,   33,  253,  139,  160
``` 

and provide parsed output from next format:

```text
Sequence  File name                     Group id  Thread name  Severity  Line  Timestamp      Δ timestamp  Description                        
2         AutoExposure.c                2         DEPTH        1         484   2339142743     0            Depth - ROI control set to left 480, right 848                         
3         HwConfig.c                    2         DEPTH        3         417   2339144343     0.016        HW config - OTF status inconsistent 0, 0, 0x0                       
4         Imager.c                      2         DEPTH        1         230   2348470485     93.2614      Sensor frame rate changed to 15 FPS, VTS 0x1987, HTS 0x330
5         HwConfig.c                    2         DEPTH        3         417   2348622271     1.51786      HW config - OTF status inconsistent 0, 0, 0x0
6         PWM.c                         2         DEPTH        1         809   2348622640     0.00369      PWM - RegCamPwmWindowCount 0xd5d4, RegCamPwmDelayCount 0x842a
```

## Usage

* Create log file:
```shell
v4l2-ctl -d /dev/video0 -C logger > firmware.log
```

The script can receive input from pipe or from file:
  * Pipe input:
  ```shell
  cat firmware.log | python .\firmware_log_parser.py -x HWLoggerEventsDS5.xml
  ```
  **Note:** -x HWLoggerEventsDS5.xml - a file with guides for parser
  
  * File input:
  ```shell
  python .\firmware_log_parser.py -x HWLoggerEventsDS5.xml -f firmware.log
  ```
  **Note:** -f _file.log_ - a file with firmware log <br />
  -x _file.xml_ - a file with guides for parser
  
## Settings

Find the `output_customisation` variable in the script and change the print value to False/True to purpose Hide/Show parameter in an output
```python
output_customisation = {'print_sequence_id': True,
                        'print_file_name': True,
                        'print_group_id': True,
                        'print_thread_name': True,
                        'print_severity': True,
                        'print_line_num': True,
                        'print_timestamp': True,
                        'print_delta_timestamp': True,
                        'print_description': True}
```