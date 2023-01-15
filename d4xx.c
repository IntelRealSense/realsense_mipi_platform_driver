/*
 * ds5.c - Intel(R) RealSense(TM) D4XX camera driver
 *
 * Copyright (c) 2017-2019, INTEL CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/media.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/videodev2.h>

#include <media/media-entity.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-mediabus.h>

//#define DS5_DRIVER_NAME "DS5 RealSense camera driver"
#define DS5_DRIVER_NAME "d4xx"
#define DS5_DRIVER_NAME_AWG "d4xx-awg"
#define DS5_DRIVER_NAME_ASR "d4xx-asr"
#define DS5_DRIVER_NAME_CLASS "d4xx-class"
#define DS5_DRIVER_NAME_DFU "d4xx-dfu"

#define DS5_MIPI_SUPPORT_LINES		0x0300
#define DS5_MIPI_SUPPORT_PHY		0x0304
#define DS5_MIPI_DATARATE_MIN		0x0308
#define DS5_MIPI_DATARATE_MAX		0x030A
#define DS5_FW_VERSION			0x030C
#define DS5_FW_BUILD			0x030E
#define DS5_DEVICE_TYPE			0x0310
#define DS5_DEVICE_TYPE_D45X		6
#define DS5_DEVICE_TYPE_D43X		5
#define DS5_DEVICE_TYPE_D46X		4

#define DS5_MIPI_LANE_NUMS		0x0400
#define DS5_MIPI_LANE_DATARATE		0x0402
#define DS5_MIPI_CONF_STATUS		0x0500

#define DS5_START_STOP_STREAM		0x1000
#define DS5_DEPTH_STREAM_STATUS		0x1004
#define DS5_RGB_STREAM_STATUS		0x1008
#define DS5_IMU_STREAM_STATUS		0x100C
#define DS5_IR_STREAM_STATUS		0x1014

#define DS5_STREAM_DEPTH		0x0
#define DS5_STREAM_RGB			0x1
#define DS5_STREAM_IMU			0x2
#define DS5_STREAM_IR			0x4
#define DS5_STREAM_STOP			0x100
#define DS5_STREAM_START		0x200
#define DS5_STREAM_IDLE			0x1
#define DS5_STREAM_STREAMING		0x2

#define DS5_DEPTH_STREAM_DT		0x4000
#define DS5_DEPTH_STREAM_MD		0x4002
#define DS5_DEPTH_RES_WIDTH		0x4004
#define DS5_DEPTH_RES_HEIGHT		0x4008
#define DS5_DEPTH_FPS			0x400C
#define DS5_DEPTH_OVERRIDE		0x401C

#define DS5_RGB_STREAM_DT		0x4020
#define DS5_RGB_STREAM_MD		0x4022
#define DS5_RGB_RES_WIDTH		0x4024
#define DS5_RGB_RES_HEIGHT		0x4028
#define DS5_RGB_FPS			0x402C

#define DS5_IMU_STREAM_DT		0x4040
#define DS5_IMU_STREAM_MD		0x4042
#define DS5_IMU_RES_WIDTH		0x4044
#define DS5_IMU_RES_HEIGHT		0x4048
#define DS5_IMU_FPS			0x404C

#define DS5_IR_STREAM_DT		0x4080
#define DS5_IR_STREAM_MD		0x4082
#define DS5_IR_RES_WIDTH		0x4084
#define DS5_IR_RES_HEIGHT		0x4088
#define DS5_IR_FPS			0x408C
#define DS5_IR_OVERRIDE			0x409C

#define DS5_DEPTH_CONTROL_BASE		0x4100
#define DS5_RGB_CONTROL_BASE		0x4200
#define DS5_MANUAL_EXPOSURE_LSB		0x0000
#define DS5_MANUAL_EXPOSURE_MSB		0x0002
#define DS5_MANUAL_GAIN			0x0004
#define DS5_LASER_POWER			0x0008
#define DS5_AUTO_EXPOSURE_MODE		0x000C
#define DS5_EXPOSURE_ROI_TOP		0x0010
#define DS5_EXPOSURE_ROI_LEFT		0x0014
#define DS5_EXPOSURE_ROI_BOTTOM		0x0018
#define DS5_EXPOSURE_ROI_RIGHT		0x001C
#define DS5_MANUAL_LASER_POWER		0x0024
#define DS5_PWM_FREQUENCY			0x0028

#define DS5_DEPTH_CONFIG_STATUS		0x4800
#define DS5_RGB_CONFIG_STATUS		0x4802
#define DS5_IMU_CONFIG_STATUS		0x4804
#define DS5_IR_CONFIG_STATUS		0x4808

#define DS5_STATUS_STREAMING		0x1
#define DS5_STATUS_INVALID_DT		0x2
#define DS5_STATUS_INVALID_RES		0x4
#define DS5_STATUS_INVALID_FPS		0x8

#define MIPI_LANE_RATE			1000

#define MAX_DEPTH_EXP			200000
#define MAX_RGB_EXP			10000
#define DEF_DEPTH_EXP			33000
#define DEF_RGB_EXP			1660

/* Currently both depth and IR use VC 0 */
#define DS5_DEPTH_VCHAN_N		0
#define DS5_MOTION_T_VCHAN_N		0
//#define DS5_DEBUG_VCHAN_N		1
//#define DS5_MOTION_T_VCHAN_N		2

enum ds5_mux_pad {
	DS5_MUX_PAD_EXTERNAL,
	DS5_MUX_PAD_DEPTH,
	DS5_MUX_PAD_MOTION_T,
	DS5_MUX_PAD_RGB,
	DS5_MUX_PAD_IMU,
	DS5_MUX_PAD_COUNT,
};

#define DS5_N_CONTROLS			8

#define CSI2_MAX_VIRTUAL_CHANNELS	4

#define DFU_WAIT_RET_LEN 6

#define DS5_START_POLL_TIME	10
#define DS5_START_MAX_TIME	1000
#define DS5_START_MAX_COUNT	(DS5_START_MAX_TIME / DS5_START_POLL_TIME)

/* DFU definition section */
#define DFU_MAGIC_NUMBER "/0x01/0x02/0x03/0x04"
#define DFU_BLOCK_SIZE 1024

#define DFU_I2C_STANDARD_MODE		100000
#define DFU_I2C_FAST_MODE			400000
#define DFU_I2C_BUS_CLK_RATE		DFU_I2C_FAST_MODE

#define ds5_read_with_check(state, addr, val) {\
	if (ds5_read(state, addr, val))	\
		return -EINVAL;}
#define ds5_raw_read_with_check(state, addr, buf, size)	{\
	if (ds5_raw_read(state, addr, buf, size))	\
		return -EINVAL;}
#define ds5_write_with_check(state, addr, val) {\
	if (ds5_write(state, addr, val)) 	\
		return -EINVAL;}
#define ds5_raw_write_with_check(state, addr, buf, size) {\
	if (ds5_raw_write(state, addr, buf, size)) 	\
		return -EINVAL;}
enum dfu_fw_state {
	appIDLE                = 0x0000,
	appDETACH              = 0x0001,
	dfuIDLE                = 0x0002,
	dfuDNLOAD_SYNC         = 0x0003,
	dfuDNBUSY              = 0x0004,
	dfuDNLOAD_IDLE         = 0x0005,
	dfuMANIFEST_SYNC       = 0x0006,
	dfuMANIFEST            = 0x0007,
	dfuMANIFEST_WAIT_RESET = 0x0008,
	dfuUPLOAD_IDLE         = 0x0009,
	dfuERROR               = 0x000a
};

enum dfu_state {
	DS5_DFU_IDLE = 0,
	DS5_DFU_RECOVERY,
	DS5_DFU_OPEN,
	DS5_DFU_IN_PROGRESS,
	DS5_DFU_DONE,
	DS5_DFU_ERROR
} dfu_state_t;

struct hwm_cmd {
	u16 header;
	u16 magic_word;
	u32 opcode;
	u32 param1;
	u32 param2;
	u32 param3;
	u32 param4;
	unsigned char Data[0];
};

static const struct hwm_cmd cmd_switch_to_dfu = {
	.header = 0x14,
	.magic_word = 0xCDAB,
	.opcode = 0x1e,
	.param1 = 0x01,
};

enum table_id {
	COEF_CALIBRATION_ID = 0x19,
	DEPTH_CALIBRATION_ID = 0x1f,
	RGB_CALIBRATION_ID = 0x20,
	IMU_CALIBRATION_ID = 0x22
} table_id_t;

static const struct hwm_cmd get_calib_data = {
	.header = 0x14,
	.magic_word = 0xCDAB,
	.opcode = 0x15,
	.param1 = 0x00,	//table_id
};

static const struct hwm_cmd set_calib_data = {
	.header = 0x0114,
	.magic_word = 0xCDAB,
	.opcode = 0x62,
	.param1 = 0x00,	//table_id
	.param2 = 0x02,	//region
};

static const struct hwm_cmd gvd = {
	.header = 0x14,
	.magic_word = 0xCDAB,
	.opcode = 0x10,
};

static const struct hwm_cmd set_ae_roi = {
	.header = 0x14,
	.magic_word = 0xCDAB,
	.opcode = 0x44,
};

static const struct hwm_cmd get_ae_roi = {
	.header = 0x014,
	.magic_word = 0xCDAB,
	.opcode = 0x45,
};

static const struct hwm_cmd set_ae_setpoint = {
	.header = 0x18,
	.magic_word = 0xCDAB,
	.opcode = 0x2B,
	.param1 = 0xa, // AE control
};

static const struct hwm_cmd get_ae_setpoint = {
	.header = 0x014,
	.magic_word = 0xCDAB,
	.opcode = 0x2C,
	.param1 = 0xa, // AE control
	.param2 = 0, // get current
};

static const struct hwm_cmd erb = {
	.header = 0x14,
	.magic_word = 0xCDAB,
	.opcode = 0x17,
};

static const struct hwm_cmd ewb = {
	.header = 0x14,
	.magic_word = 0xCDAB,
	.opcode = 0x18,
};

struct __fw_status {
	uint32_t	spare1;
	uint32_t	FW_lastVersion;
	uint32_t	FW_highestVersion;
	uint16_t	FW_DownloadStatus;
	uint16_t	DFU_isLocked;
	uint16_t	DFU_version;
	uint8_t		ivcamSerialNum[8];
	uint8_t		spare2[42];
};


/*************************/

struct ds5_ctrls {
	struct v4l2_ctrl_handler handler;
	struct {
		struct v4l2_ctrl *log;
		struct v4l2_ctrl *fw_version;
		struct v4l2_ctrl *gvd;
		struct v4l2_ctrl *get_depth_calib;
		struct v4l2_ctrl *set_depth_calib;
		struct v4l2_ctrl *get_coeff_calib;
		struct v4l2_ctrl *set_coeff_calib;
		struct v4l2_ctrl *ae_roi_get;
		struct v4l2_ctrl *ae_roi_set;
		struct v4l2_ctrl *ae_setpoint_get;
		struct v4l2_ctrl *ae_setpoint_set;
		struct v4l2_ctrl *erb;
		struct v4l2_ctrl *ewb;
		struct v4l2_ctrl *hwmc;
		struct v4l2_ctrl *laser_power;
		struct v4l2_ctrl *manual_laser_power;
		struct v4l2_ctrl *auto_exp;
		struct v4l2_ctrl *exposure;
		/* in DS5 manual gain only works with manual exposure */
		struct v4l2_ctrl *gain;
	};
};

struct ds5_resolution {
	u16 width;
	u16 height;
	u8 n_framerates;
	const u16 *framerates;
};

struct ds5_format {
	unsigned int n_resolutions;
	const struct ds5_resolution *resolutions;
	u32 mbus_code;
	u8 data_type;
};

struct ds5_sensor {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_mbus_framefmt format;
	u16 mux_pad;
	struct {
		const struct ds5_format *format;
		const struct ds5_resolution *resolution;
		u16 framerate;
	} config;
	bool streaming;
	/*struct ds5_vchan *vchan;*/
	const struct ds5_format *formats;
	unsigned int n_formats;
};

#ifdef CONFIG_TEGRA_CAMERA_PLATFORM
#include <media/camera_common.h>
#define ds5_mux_subdev camera_common_data
#else
struct ds5_mux_subdev {
	struct v4l2_subdev subdev;
};
#endif

struct ds5_variant {
	const struct ds5_format *formats;
	unsigned int n_formats;
};

struct ds5_dfu_dev {
	struct cdev ds5_cdev;
	struct class *ds5_class;
	int device_open_count;
	enum dfu_state dfu_state_flag;
	unsigned char *dfu_msg;
	u16 msg_write_once;
	unsigned char init_v4l_f;
	u32 bus_clk_rate;
};

enum {
	DS5_DS5U,
	DS5_ASR,
	DS5_AWG,
};

struct ds5 {
	struct {
		struct ds5_sensor sensor;
	} depth;
	struct {
		struct ds5_sensor sensor;
	} motion_t;
	struct {
		struct ds5_sensor sensor;
	} rgb;
	struct {
		struct ds5_sensor sensor;
	} imu;
	struct {
		struct ds5_mux_subdev sd;
		struct media_pad pads[DS5_MUX_PAD_COUNT];
		struct ds5_sensor *last_set;
	} mux;
	struct ds5_ctrls ctrls;
	struct ds5_dfu_dev dfu_dev;
	bool power;
	struct i2c_client *client;
	/*struct ds5_vchan virtual_channels[CSI2_MAX_VIRTUAL_CHANNELS];*/
	/* All below pointers are used for writing, cannot be const */
	struct mutex lock;
	struct regmap *regmap;
	struct regulator *vcc;
	const struct ds5_variant *variant;
	int is_depth;
	int is_y8;
	int is_rgb;
	int is_imu;
	u16 fw_version;
	u16 fw_build;
};

struct ds5_counters {
	unsigned int n_res;
	unsigned int n_fmt;
	unsigned int n_ctrl;
};

#define ds5_from_depth_sd(sd) container_of(sd, struct ds5, depth.sd)
#define ds5_from_motion_t_sd(sd) container_of(sd, struct ds5, motion_t.sd)
#define ds5_from_rgb_sd(sd) container_of(sd, struct ds5, rgb.sd)

static int ds5_write(struct ds5 *state, u16 reg, u16 val)
{
	int ret;
	u8 value[2];

	value[1] = val >> 8;
	value[0] = val & 0x00FF;

	dev_dbg(&state->client->dev,
			"%s(): writing to register: 0x%04x, value1: 0x%x, value2:0x%x\n",
			__func__, reg, value[1], value[0]);

	ret = regmap_raw_write(state->regmap, reg, value, sizeof(value));
	if (ret < 0)
		dev_err(&state->client->dev,
				"%s(): i2c write failed %d, 0x%04x = 0x%x\n",
				__func__, ret, reg, val);
	else
		if (state->dfu_dev.dfu_state_flag == DS5_DFU_IDLE)
			dev_dbg(&state->client->dev, "%s(): i2c write 0x%04x: 0x%x\n",
				__func__, reg, val);

	return ret;
}

static int ds5_raw_write(struct ds5 *state, u16 reg,
		const void *val, size_t val_len)
{
	int ret = regmap_raw_write(state->regmap, reg, val, val_len);
	if (ret < 0)
		dev_err(&state->client->dev,
				"%s(): i2c raw write failed %d, %04x size(%d) bytes\n",
				__func__, ret, reg, (int)val_len);
	else
		if (state->dfu_dev.dfu_state_flag == DS5_DFU_IDLE)
			dev_dbg(&state->client->dev,
					"%s(): i2c raw write 0x%04x: %d bytes\n",
					__func__, reg, (int)val_len);

	return ret;
}

static int ds5_read(struct ds5 *state, u16 reg, u16 *val)
{
	int ret = regmap_raw_read(state->regmap, reg, val, 2);
	if (ret < 0)
		dev_err(&state->client->dev, "%s(): i2c read failed %d, 0x%04x\n",
				__func__, ret, reg);
	else {
		if (state->dfu_dev.dfu_state_flag == DS5_DFU_IDLE)
			dev_dbg(&state->client->dev, "%s(): i2c read 0x%04x: 0x%x\n",
					__func__, reg, *val);
	}

	return ret;
}

static int ds5_raw_read(struct ds5 *state, u16 reg, void *val,
 size_t val_len)
{
	int ret = regmap_raw_read(state->regmap, reg, val, val_len);
	if (ret < 0)
		dev_err(&state->client->dev, "%s(): i2c read failed %d, 0x%04x\n",
			__func__, ret, reg);

	return ret;
}

/* Pad ops */

static const u16 ds5_default_framerate = 30;

// **********************
// FIXME: D16 width must be doubled, because an 8-bit format is used. Check how
// the Tegra driver propagates resolutions and formats.
// **********************

//TODO: keep 6, till 5 is supported by FW
static const u16 ds5_framerates[] = {5, 30};

#define DS5_FRAMERATE_DEFAULT_IDX 1

static const u16 ds5_framerate_30 = 30;

static const u16 ds5_framerate_15_30[] = {15, 30};

static const u16 ds5_framerate_25 = 25;

static const u16 ds5_depth_framerate_to_30[] = {5, 15, 30};
static const u16 ds5_framerate_to_30[] = {5, 10, 15, 30};
static const u16 ds5_framerate_to_60[] = {5, 15, 30, 60};
static const u16 ds5_framerate_to_90[] = {5, 15, 30, 60, 90};
static const u16 ds5_framerate_100[] = {100};
static const u16 ds5_framerate_90[] = {90};
static const u16 ds5_imu_framerates[] = {50, 100, 200, 400};

static const struct ds5_resolution d43x_depth_sizes[] = {
	{
		.width = 1280,
		.height = 720,
		.framerates = ds5_depth_framerate_to_30,
		.n_framerates = ARRAY_SIZE(ds5_depth_framerate_to_30),
	}, {
		.width =  848,
		.height = 480,
		.framerates = ds5_framerate_to_90,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_90),
	}, {
		.width =  848,
		.height = 100,
		.framerates = ds5_framerate_100,
		.n_framerates = ARRAY_SIZE(ds5_framerate_100),
	}, {
		.width =  640,
		.height = 480,
		.framerates = ds5_framerate_to_90,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_90),
	}, {
		.width =  640,
		.height = 360,
		.framerates = ds5_framerate_to_90,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_90),
	}, {
		.width =  480,
		.height = 270,
		.framerates = ds5_framerate_to_90,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_90),
	}, {
		.width =  424,
		.height = 240,
		.framerates = ds5_framerate_to_90,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_90),
	}, {
		.width =  256,
		.height = 144,
		.framerates = ds5_framerate_90,
		.n_framerates = ARRAY_SIZE(ds5_framerate_90),
	},
};

static const struct ds5_resolution d46x_depth_sizes[] = {
	{
		.width = 1280,
		.height = 960,
		.framerates = ds5_framerates,
		.n_framerates = ARRAY_SIZE(ds5_framerates),
	},{
		.width =  640,
		.height = 480,
		.framerates = ds5_framerates,
		.n_framerates = ARRAY_SIZE(ds5_framerates),
	},
};

static const struct ds5_resolution y8_sizes[] = {
	{
		.width = 1280,
		.height = 720,
		.framerates = ds5_depth_framerate_to_30,
		.n_framerates = ARRAY_SIZE(ds5_depth_framerate_to_30),
	}, {
		.width =  848,
		.height = 480,
		.framerates = ds5_framerate_to_90,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_90),
	}, {
		.width =  640,
		.height = 480,
		.framerates = ds5_framerate_to_90,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_90),
	}, {
		.width =  640,
		.height = 360,
		.framerates = ds5_framerate_to_90,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_90),
	}, {
		.width =  480,
		.height = 270,
		.framerates = ds5_framerate_to_90,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_90),
	}, {
		.width =  424,
		.height = 240,
		.framerates = ds5_framerate_to_90,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_90),
	}
};

static const struct ds5_resolution ds5_rlt_rgb_sizes[] = {
	{
		.width = 1280,
		.height = 800,
		.framerates = ds5_framerate_to_30,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_30),
	}, {
		.width = 1280,
		.height = 720,
		.framerates = ds5_framerate_to_30,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_30),
	}, {
		.width = 848,
		.height = 480,
		.framerates = ds5_framerate_to_60,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_60),
	}, {
		.width = 640,
		.height = 480,
		.framerates = ds5_framerate_to_60,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_60),
	}, {
		.width = 640,
		.height = 360,
		.framerates = ds5_framerate_to_90,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_90),
	}, {
		.width = 480,
		.height = 270,
		.framerates = ds5_framerate_to_90,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_90),
	}, {
		.width = 424,
		.height = 240,
		.framerates = ds5_framerate_to_90,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_90),
	},
};

static const struct ds5_resolution ds5_onsemi_rgb_sizes[] = {
	{
		.width = 640,
		.height = 480,
		.framerates = ds5_framerate_to_90,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_90),
	}, {
		.width = 960,
		.height = 720,
		.framerates = ds5_framerate_to_60,
		.n_framerates = ARRAY_SIZE(ds5_framerate_to_60),
	}, {
		.width = 1280,
		.height = 720,
		.framerates = ds5_framerates,
		.n_framerates = ARRAY_SIZE(ds5_framerates),
	}, {
		.width = 1920,
		.height = 1080,
		.framerates = ds5_framerates,
		.n_framerates = ARRAY_SIZE(ds5_framerates),
	}, {
		.width = 2048,
		.height = 1536,
		.framerates = ds5_framerates,
		.n_framerates = ARRAY_SIZE(ds5_framerates),
	},
};

static const struct ds5_resolution ds5_size_w10 = {
	.width =  1920,
	.height = 1080,
	.framerates = &ds5_framerate_30,
	.n_framerates = 1,
};

static const struct ds5_resolution d43x_calibration_sizes[] = {
	{
		.width =  1280,
		.height = 800,
		.framerates = ds5_framerate_15_30,
		.n_framerates = ARRAY_SIZE(ds5_framerate_15_30),
	},
};

static const struct ds5_resolution d46x_calibration_sizes[] = {
	{
		.width =  1600,
		.height = 1300,
		.framerates = ds5_framerate_15_30,
		.n_framerates = ARRAY_SIZE(ds5_framerate_15_30),
	},
};

static const struct ds5_resolution ds5_size_imu[] = {
	{
	.width =  32,
	.height = 1,
	.framerates = ds5_imu_framerates,
	.n_framerates = ARRAY_SIZE(ds5_imu_framerates),
	},
};

static const struct ds5_format ds5_depth_formats_d43x[] = {
	{
		// TODO: 0x31 is replaced with 0x1e since it caused low FPS in Jetson.
		.data_type = 0x1e,	/* Z16 */
		.mbus_code = MEDIA_BUS_FMT_UYVY8_1X16,
		.n_resolutions = ARRAY_SIZE(d43x_depth_sizes),
		.resolutions = d43x_depth_sizes,
	}, {
		.data_type = 0x2a,	/* Y8 */
		.mbus_code = MEDIA_BUS_FMT_Y8_1X8,
		.n_resolutions = ARRAY_SIZE(d43x_depth_sizes),
		.resolutions = d43x_depth_sizes,
	}, {
		.data_type = 0x24,	/* 24-bit Calibration */
		.mbus_code = MEDIA_BUS_FMT_RGB888_1X24,	/* FIXME */
		.n_resolutions = ARRAY_SIZE(d43x_calibration_sizes),
		.resolutions = d43x_calibration_sizes,
	},
};

static const struct ds5_format ds5_depth_formats_d46x[] = {
	{
		// TODO: 0x31 is replaced with 0x1e since it caused low FPS in Jetson.
		.data_type = 0x1e,	/* Z16 */
		.mbus_code = MEDIA_BUS_FMT_UYVY8_1X16,
		.n_resolutions = ARRAY_SIZE(d46x_depth_sizes),
		.resolutions = d46x_depth_sizes,
	}, {
		/* First format: default */
		.data_type = 0x2a,	/* Y8 */
		.mbus_code = MEDIA_BUS_FMT_Y8_1X8,
		.n_resolutions = ARRAY_SIZE(d46x_depth_sizes),
		.resolutions = d46x_depth_sizes,
	}, {
		.data_type = 0x24,	/* 24-bit Calibration */
		.mbus_code = MEDIA_BUS_FMT_RGB888_1X24,	/* FIXME */
		.n_resolutions = ARRAY_SIZE(d46x_calibration_sizes),
		.resolutions = d46x_calibration_sizes,
	},
};

#define DS5_DEPTH_N_FORMATS 1

static const struct ds5_format ds5_y_formats_ds5u[] = {
	{
		/* First format: default */
		.data_type = 0x2a,	/* Y8 */
		.mbus_code = MEDIA_BUS_FMT_Y8_1X8,
		.n_resolutions = ARRAY_SIZE(y8_sizes),
		.resolutions = y8_sizes,
	}, {
		.data_type = 0x1e,	/* Y8I */
		.mbus_code = MEDIA_BUS_FMT_VYUY8_1X16,
		.n_resolutions = ARRAY_SIZE(y8_sizes),
		.resolutions = y8_sizes,
	}, {
		.data_type = 0x24,	/* 24-bit Calibration */
		.mbus_code = MEDIA_BUS_FMT_RGB888_1X24,	/* FIXME */
		.n_resolutions = ARRAY_SIZE(d46x_calibration_sizes),
		.resolutions = d46x_calibration_sizes,
	},
};

static const struct ds5_format ds5_rlt_rgb_format = {
	.data_type = 0x1e,	/* UYVY */
	.mbus_code = MEDIA_BUS_FMT_YUYV8_1X16,
	.n_resolutions = ARRAY_SIZE(ds5_rlt_rgb_sizes),
	.resolutions = ds5_rlt_rgb_sizes,
};
#define DS5_RLT_RGB_N_FORMATS 1

static const struct ds5_format ds5_onsemi_rgb_format = {
	.data_type = 0x1e,	/* UYVY */
	.mbus_code = MEDIA_BUS_FMT_YUYV8_1X16,
	.n_resolutions = ARRAY_SIZE(ds5_onsemi_rgb_sizes),
	.resolutions = ds5_onsemi_rgb_sizes,
};
#define DS5_ONSEMI_RGB_N_FORMATS 1

static const struct ds5_variant ds5_variants[] = {
	[DS5_DS5U] = {
		.formats = ds5_y_formats_ds5u,
		.n_formats = ARRAY_SIZE(ds5_y_formats_ds5u),
	},
};

static const struct ds5_format ds5_imu_formats[] = {
	{
		/* First format: default */
		.data_type = 0x2a,	/* IMU DT */
		.mbus_code = MEDIA_BUS_FMT_Y8_1X8,
		.n_resolutions = ARRAY_SIZE(ds5_size_imu),
		.resolutions = ds5_size_imu,
	},
};

static const struct v4l2_mbus_framefmt ds5_mbus_framefmt_template = {
	.width = 0,
	.height = 0,
	.code = MEDIA_BUS_FMT_FIXED,
	.field = V4L2_FIELD_NONE,
	.colorspace = V4L2_COLORSPACE_DEFAULT,
	.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT,
	.quantization = V4L2_QUANTIZATION_DEFAULT,
	.xfer_func = V4L2_XFER_FUNC_DEFAULT,
};

/* Get readable sensor name */
static const char *ds5_get_sensor_name(struct ds5 *state)
{
	static const char *sensor_name[] = {"unknown", "RGB", "DEPTH", "Y8", "IMU"};
	int sensor_id = state->is_rgb * 1 + state->is_depth * 2 + \
			state->is_y8 * 3 + state->is_imu * 4;
	if (sensor_id >= (sizeof(sensor_name)/sizeof(*sensor_name)))
		sensor_id = 0;

	return sensor_name[sensor_id];
}

/* This is needed for .get_fmt()
 * and if streaming is started without .set_fmt() */
static void ds5_sensor_format_init(struct ds5_sensor *sensor)
{
	const struct ds5_format *fmt;
	struct v4l2_mbus_framefmt *ffmt;
	unsigned int i;

	if (sensor->config.format)
		return;

	dev_dbg(sensor->sd.dev, "%s(): on pad %u\n", __func__, sensor->mux_pad);

	ffmt = &sensor->format;
	*ffmt = ds5_mbus_framefmt_template;
	/* Use the first format */
	fmt = sensor->formats;
	ffmt->code = fmt->mbus_code;
	/* and the first resolution */
	ffmt->width = fmt->resolutions->width;
	ffmt->height = fmt->resolutions->height;

	sensor->config.format = fmt;
	sensor->config.resolution = fmt->resolutions;
	/* Set default framerate to 30, or to 1st one if not supported */
	for (i = 0; i < fmt->resolutions->n_framerates;i++) {
		if (fmt->resolutions->framerates[i] == ds5_framerate_30 /* fps */) {
			sensor->config.framerate = ds5_framerate_30;
			return;
		}
	}
	sensor->config.framerate = fmt->resolutions->framerates[0];
}

/* No locking needed for enumeration methods */
static int ds5_sensor_enum_mbus_code(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_mbus_code_enum *mce)
{
	struct ds5_sensor *sensor = container_of(sd, struct ds5_sensor, sd);

	if (mce->pad)
		return -EINVAL;

	if (mce->index >= sensor->n_formats)
		return -EINVAL;

	mce->code = sensor->formats[mce->index].mbus_code;

	return 0;
}

static int ds5_sensor_enum_frame_size(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_frame_size_enum *fse)
{
	struct ds5_sensor *sensor = container_of(sd, struct ds5_sensor, sd);
	const struct ds5_format *fmt;
	unsigned int i;

	for (i = 0, fmt = sensor->formats; i < sensor->n_formats; i++, fmt++)
		if (fse->code == fmt->mbus_code)
			break;

	if (i == sensor->n_formats)
		return -EINVAL;

	if (fse->index >= fmt->n_resolutions)
		return -EINVAL;

	fse->min_width = fse->max_width = fmt->resolutions[fse->index].width;
	fse->min_height = fse->max_height = fmt->resolutions[fse->index].height;

	return 0;
}

static int ds5_sensor_enum_frame_interval(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_frame_interval_enum *fie)
{
	struct ds5_sensor *sensor = container_of(sd, struct ds5_sensor, sd);
	const struct ds5_format *fmt;
	const struct ds5_resolution *res;
	unsigned int i;

	for (i = 0, fmt = sensor->formats; i < sensor->n_formats; i++, fmt++)
		if (fie->code == fmt->mbus_code)
			break;

	if (i == sensor->n_formats)
		return -EINVAL;

	for (i = 0, res = fmt->resolutions; i < fmt->n_resolutions; i++, res++)
		if (res->width == fie->width && res->height == fie->height)
			break;

	if (i == fmt->n_resolutions)
		return -EINVAL;

	if (fie->index >= res->n_framerates)
		return -EINVAL;

	fie->interval.numerator = 1;
	fie->interval.denominator = res->framerates[fie->index];

	return 0;
}

static int ds5_sensor_get_fmt(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_format *fmt)
{
	struct ds5_sensor *sensor = container_of(sd, struct ds5_sensor, sd);
	struct ds5 *state = v4l2_get_subdevdata(sd);

	if (fmt->pad)
		return -EINVAL;

	mutex_lock(&state->lock);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		fmt->format = *v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
	else
		fmt->format = sensor->format;

	mutex_unlock(&state->lock);

	dev_dbg(sd->dev, "%s(): pad %x, code %x, res %ux%u\n",
			__func__, fmt->pad, fmt->format.code,
			fmt->format.width, fmt->format.height);

	return 0;
}

/* Called with lock held */
static const struct ds5_format *ds5_sensor_find_format(
		struct ds5_sensor *sensor,
		struct v4l2_mbus_framefmt *ffmt,
		const struct ds5_resolution **best)
{
	const struct ds5_resolution *res;
	const struct ds5_format *fmt;
	unsigned long best_delta = ~0;
	unsigned int i;

	for (i = 0, fmt = sensor->formats; i < sensor->n_formats; i++, fmt++) {
		if (fmt->mbus_code == ffmt->code)
			break;
	}
	dev_dbg(sensor->sd.dev, "%s(): mbus_code = %x, code = %x \n",
			__func__, fmt->mbus_code, ffmt->code);

	if (i == sensor->n_formats)
		/* Not found, use default */
		fmt = sensor->formats;

	for (i = 0, res = fmt->resolutions; i < fmt->n_resolutions; i++, res++) {
		unsigned long delta = abs(ffmt->width * ffmt->height -
				res->width * res->height);
		if (delta < best_delta) {
			best_delta = delta;
			*best = res;
		}
	}

	ffmt->code = fmt->mbus_code;
	ffmt->width = (*best)->width;
	ffmt->height = (*best)->height;

	ffmt->field = V4L2_FIELD_NONE;
	/* Should we use V4L2_COLORSPACE_RAW for Y12I? */
	ffmt->colorspace = V4L2_COLORSPACE_SRGB;

	return fmt;
}

static int __ds5_sensor_set_fmt(struct ds5 *state, struct ds5_sensor *sensor,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_format *fmt)
{
	struct v4l2_mbus_framefmt *mf;// = &fmt->format;
	//unsigned r;

	dev_dbg(sensor->sd.dev, "%s(): state %p, "
			"sensor %p, cfg %p, fmt %p, fmt->format %p\n",
			__func__, state, sensor, cfg, fmt,  &fmt->format);

	mf = &fmt->format;

	if (fmt->pad)
		return -EINVAL;

	mutex_lock(&state->lock);

	sensor->config.format = ds5_sensor_find_format(sensor, mf,
						&sensor->config.resolution);
	//r = DS5_FRAMERATE_DEFAULT_IDX < sensor->config.resolution->n_framerates ?
	//	DS5_FRAMERATE_DEFAULT_IDX : 0;
	/* FIXME: check if a framerate has been set */
	//sensor->config.framerate = sensor->config.resolution->framerates[r];

	if (cfg && fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		*v4l2_subdev_get_try_format(&sensor->sd, cfg, fmt->pad) = *mf;
		dev_dbg(sensor->sd.dev, "%s():4l2_subdev_get_try_format ret: %p\n",
				__func__, cfg);
	}

	else
// FIXME: use this format in .s_stream()
		sensor->format = *mf;

	state->mux.last_set = sensor;

	mutex_unlock(&state->lock);

	dev_dbg(sensor->sd.dev, "%s(): pad: %x, code: %x, %ux%u\n",
			__func__, fmt->pad, fmt->format.code,
			fmt->format.width, fmt->format.height);

	return 0;
}

static int ds5_sensor_set_fmt(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_format *fmt)
{
	struct ds5_sensor *sensor = container_of(sd, struct ds5_sensor, sd);
	struct ds5 *state = v4l2_get_subdevdata(sd);

	return __ds5_sensor_set_fmt(state, sensor, cfg, fmt);
}

static int ds5_configure(struct ds5 *state)
{
	struct ds5_sensor *sensor;
	u16 fmt, md_fmt, vc_id;
	u16 dt_addr, md_addr, override_addr, fps_addr, width_addr, height_addr;
	int ret;

	if (state->is_depth) {
		sensor = &state->depth.sensor;
		dt_addr = DS5_DEPTH_STREAM_DT;
		md_addr = DS5_DEPTH_STREAM_MD;
		override_addr = DS5_DEPTH_OVERRIDE;
		fps_addr = DS5_DEPTH_FPS;
		width_addr = DS5_DEPTH_RES_WIDTH;
		height_addr = DS5_DEPTH_RES_HEIGHT;
		// TODO: read VC from device tree
		vc_id = 0;
		md_fmt = 0x12;
	} else if (state->is_rgb) {
		sensor = &state->rgb.sensor;
		dt_addr = DS5_RGB_STREAM_DT;
		md_addr = DS5_RGB_STREAM_MD;
		override_addr = 0;
		fps_addr = DS5_RGB_FPS;
		width_addr = DS5_RGB_RES_WIDTH;
		height_addr = DS5_RGB_RES_HEIGHT;
		vc_id = 1;
		md_fmt = 0x12;
	} else if (state->is_y8) {
		sensor = &state->motion_t.sensor;
		dt_addr = DS5_IR_STREAM_DT;
		md_addr = DS5_IR_STREAM_MD;
		override_addr = DS5_IR_OVERRIDE;
		fps_addr = DS5_IR_FPS;
		width_addr = DS5_IR_RES_WIDTH;
		height_addr = DS5_IR_RES_HEIGHT;
		vc_id = 2;
		md_fmt = 0x12;
	} else if (state->is_imu) {
		sensor = &state->imu.sensor;
		dt_addr = DS5_IMU_STREAM_DT;
		md_addr = DS5_IMU_STREAM_MD;
		override_addr = 0;
		fps_addr = DS5_IMU_FPS;
		width_addr = DS5_IMU_RES_WIDTH;
		height_addr = DS5_IMU_RES_HEIGHT;
		vc_id = 3;
		md_fmt = 0x0;
	} else {
		return -EINVAL;
	}

	fmt = sensor->streaming ? sensor->config.format->data_type : 0;

	/*
	 * Set depth stream Z16 data type as 0x31
	 * Set IR stream Y8I data type as 0x32
	 */
	if (state->is_depth && fmt != 0)
		ret = ds5_write(state, dt_addr, 0x31);
	else if (state->is_y8 && fmt != 0 &&
		 sensor->config.format->data_type == 0x1e)
		ret = ds5_write(state, dt_addr, 0x32);
	else
		ret = ds5_write(state, dt_addr, fmt);
	if (ret < 0)
		return ret;

	ret = ds5_write(state, md_addr, (vc_id << 8) | md_fmt);
	if (ret < 0)
		return ret;

	if (!sensor->streaming)
		return ret;

	if (override_addr != 0) {
		ret = ds5_write(state, override_addr, fmt);
		if (ret < 0)
			return ret;
	}

	ret = ds5_write(state, fps_addr, sensor->config.framerate);
	if (ret < 0)
		return ret;

	ret = ds5_write(state, width_addr, sensor->config.resolution->width);
	if (ret < 0)
		return ret;

	ret = ds5_write(state, height_addr, sensor->config.resolution->height);
	if (ret < 0)
		return ret;

	return 0;
}

static const struct v4l2_subdev_pad_ops ds5_depth_pad_ops = {
	.enum_mbus_code		= ds5_sensor_enum_mbus_code,
	.enum_frame_size	= ds5_sensor_enum_frame_size,
	.enum_frame_interval	= ds5_sensor_enum_frame_interval,
	.get_fmt		= ds5_sensor_get_fmt,
	.set_fmt		= ds5_sensor_set_fmt,
};

static int ds5_sensor_s_stream(struct v4l2_subdev *sd, int on)
{
	struct ds5_sensor *sensor = container_of(sd, struct ds5_sensor, sd);
	dev_dbg(sensor->sd.dev,
			"%s(): sensor: name=%s state=%d\n",
			__func__, sensor->sd.name, on);

	sensor->streaming = on;

	return 0;
}

static const struct v4l2_subdev_video_ops ds5_sensor_video_ops = {
	.s_stream		= ds5_sensor_s_stream,
};

static const struct v4l2_subdev_ops ds5_depth_subdev_ops = {
	.pad = &ds5_depth_pad_ops,
	.video = &ds5_sensor_video_ops,
};

/* Motion detection */

/* FIXME: identical to ds5_depth_pad_ops, use one for both */
static const struct v4l2_subdev_pad_ops ds5_motion_t_pad_ops = {
	.enum_mbus_code		= ds5_sensor_enum_mbus_code,
	.enum_frame_size	= ds5_sensor_enum_frame_size,
	.enum_frame_interval	= ds5_sensor_enum_frame_interval,
	.get_fmt		= ds5_sensor_get_fmt,
	.set_fmt		= ds5_sensor_set_fmt,
};

static const struct v4l2_subdev_ops ds5_motion_t_subdev_ops = {
	.pad = &ds5_motion_t_pad_ops,
	.video = &ds5_sensor_video_ops,
};

/* FIXME: identical to ds5_depth_pad_ops, use one for both? */
static const struct v4l2_subdev_pad_ops ds5_rgb_pad_ops = {
	.enum_mbus_code		= ds5_sensor_enum_mbus_code,
	.enum_frame_size	= ds5_sensor_enum_frame_size,
	.enum_frame_interval	= ds5_sensor_enum_frame_interval,
	.get_fmt		= ds5_sensor_get_fmt,
	.set_fmt		= ds5_sensor_set_fmt,
};

static const struct v4l2_subdev_ops ds5_rgb_subdev_ops = {
	.pad = &ds5_rgb_pad_ops,
	.video = &ds5_sensor_video_ops,
};

static const struct v4l2_subdev_pad_ops ds5_imu_pad_ops = {
	.enum_mbus_code		= ds5_sensor_enum_mbus_code,
	.enum_frame_size	= ds5_sensor_enum_frame_size,
	.enum_frame_interval	= ds5_sensor_enum_frame_interval,
	.get_fmt		= ds5_sensor_get_fmt,
	.set_fmt		= ds5_sensor_set_fmt,
};

static const struct v4l2_subdev_ops ds5_imu_subdev_ops = {
	.pad = &ds5_imu_pad_ops,
	.video = &ds5_sensor_video_ops,
};

static int ds5_hw_set_auto_exposure(struct ds5 *state, u32 base, s32 val)
{
	if (val != V4L2_EXPOSURE_APERTURE_PRIORITY &&
			val != V4L2_EXPOSURE_MANUAL)
		return -EINVAL;

	/*
	 * In firmware color auto exposure setting follow the uvc_menu_info
	 * exposure_auto_controls numbers, in drivers/media/usb/uvc/uvc_ctrl.c.
	 */
	if (state->is_rgb && val == V4L2_EXPOSURE_APERTURE_PRIORITY)
		val = 8;

	/*
	 * In firmware depth auto exposure on: 1, off: 0.
	 */
	if (!state->is_rgb) {
		if (val == V4L2_EXPOSURE_APERTURE_PRIORITY)
			val = 1;
		else if (val == V4L2_EXPOSURE_MANUAL)
			val = 0;
	}

	return ds5_write(state, base | DS5_AUTO_EXPOSURE_MODE, (u16)val);
}

/*
 * Manual exposure in us
 * Depth/Y8: between 100 and 200000 (200ms)
 * Color: between 100 and 1000000 (1s)
 */
static int ds5_hw_set_exposure(struct ds5 *state, u32 base, s32 val)
{
	int ret;

	if (val < 1)
		val = 1;
	if ((state->is_depth || state->is_y8) && val > MAX_DEPTH_EXP)
		val = MAX_DEPTH_EXP;
	if (state->is_rgb && val > MAX_RGB_EXP)
		val = MAX_RGB_EXP;

	/*
	 * Color and depth uses different unit:
	 *	Color: 1 is 100 us
	 *	Depth: 1 is 1 us
	 */

	ret = ds5_write(state, base | DS5_MANUAL_EXPOSURE_MSB, (u16)(val >> 16));
	if (!ret)
		ret = ds5_write(state, base | DS5_MANUAL_EXPOSURE_LSB,
				(u16)(val & 0xffff));

	return ret;
}

#define DS5_MAX_LOG_WAIT 200
#define DS5_MAX_LOG_SLEEP 10
#define DS5_MAX_LOG_POLL (DS5_MAX_LOG_WAIT / DS5_MAX_LOG_SLEEP)

// TODO: why to use DS5_DEPTH_Y_STREAMS_DT?
#define DS5_CAMERA_CID_BASE	(V4L2_CTRL_CLASS_CAMERA | DS5_DEPTH_STREAM_DT)

#define DS5_CAMERA_CID_LOG			(DS5_CAMERA_CID_BASE+0)
#define DS5_CAMERA_CID_LASER_POWER		(DS5_CAMERA_CID_BASE+1)
#define DS5_CAMERA_CID_MANUAL_LASER_POWER	(DS5_CAMERA_CID_BASE+2)
#define DS5_CAMERA_DEPTH_CALIBRATION_TABLE_GET	(DS5_CAMERA_CID_BASE+3)
#define DS5_CAMERA_DEPTH_CALIBRATION_TABLE_SET	(DS5_CAMERA_CID_BASE+4)
#define DS5_CAMERA_COEFF_CALIBRATION_TABLE_GET	(DS5_CAMERA_CID_BASE+5)
#define DS5_CAMERA_COEFF_CALIBRATION_TABLE_SET	(DS5_CAMERA_CID_BASE+6)
#define DS5_CAMERA_CID_FW_VERSION		(DS5_CAMERA_CID_BASE+7)
#define DS5_CAMERA_CID_GVD			(DS5_CAMERA_CID_BASE+8)
#define DS5_CAMERA_CID_AE_ROI_GET		(DS5_CAMERA_CID_BASE+9)
#define DS5_CAMERA_CID_AE_ROI_SET		(DS5_CAMERA_CID_BASE+10)
#define DS5_CAMERA_CID_AE_SETPOINT_GET		(DS5_CAMERA_CID_BASE+11)
#define DS5_CAMERA_CID_AE_SETPOINT_SET		(DS5_CAMERA_CID_BASE+12)
#define DS5_CAMERA_CID_ERB			(DS5_CAMERA_CID_BASE+13)
#define DS5_CAMERA_CID_EWB			(DS5_CAMERA_CID_BASE+14)
#define DS5_CAMERA_CID_HWMC			(DS5_CAMERA_CID_BASE+15)

#define DS5_CAMERA_CID_PWM			(DS5_CAMERA_CID_BASE+22)

/* the HWMC will remain for legacy tools compatibility,
 * HWMC_RW used for UVC compatibility*/
#define DS5_CAMERA_CID_HWMC_RW		(DS5_CAMERA_CID_BASE+32)

static int ds5_send_hwmc(struct ds5 *state, u16 cmdLen, struct hwm_cmd *cmd,
			 bool isRead, u16 *dataLen)
{
	int ret = 0;
	u16 status = 2;
	int retries = 100;
	int errorCode;
	int iter = retries;

	dev_dbg(&state->client->dev,
			"%s(): HWMC header: 0x%x, magic: 0x%x, opcode: 0x%x, "
			"param1: %d, param2: %d, param3: %d, param4: %d\n",
			__func__, cmd->header, cmd->magic_word, cmd->opcode,
			cmd->param1, cmd->param2, cmd->param3, cmd->param4);

	ds5_raw_write_with_check(state, 0x4900, cmd, cmdLen);

	ds5_write_with_check(state, 0x490C, 0x01); /* execute cmd */
	do {
		if (iter != retries)
			msleep_range(10);
		ret = ds5_read(state, 0x4904, &status);
	} while (iter-- && status != 0);

	if (ret || status != 0) {
		ds5_raw_read(state, 0x4900, &errorCode, 4);
		dev_err(&state->client->dev,
				"%s(): HWMC failed, ret: %d, status: %x, error code: %d\n",
				__func__, ret, status, errorCode);
		ret = -EAGAIN;
	}

	if (isRead) {
		if (*dataLen == 0) {
			ret = regmap_raw_read(state->regmap, 0x4908, dataLen, sizeof(u16));
			if (ret)
				return -EAGAIN;
		}

		dev_dbg(&state->client->dev, "%s(): HWMC read len: %d\n",
				__func__, *dataLen);
		// First 4 bytes of cmd->Data after read will include opcode
		ds5_raw_read_with_check(state, 0x4900, cmd->Data, *dataLen);

		/*This is neede for libreealsense, to align there code with UVC*/
		cmd->Data[1000] = (unsigned char)((*dataLen) & 0x00FF);
		cmd->Data[1001] = (unsigned char)(((*dataLen) & 0xFF00) >> 8);
	}

	return 0;
}

#define DS5_HWMC_DATA			0x4900
#define DS5_HWMC_STATUS			0x4904
#define DS5_HWMC_RESP_LEN		0x4908
#define DS5_HWMC_EXEC			0x490C

#define DS5_HWMC_STATUS_OK		0
#define DS5_HWMC_STATUS_ERR		1
#define DS5_HWMC_STATUS_WIP		2
#define DS5_HWMC_BUFFER_SIZE	1024

static int ds5_get_hwmc(struct ds5 *state, unsigned char *data)
{
	int ret = 0;
	u16 status = DS5_HWMC_STATUS_WIP;
	int retries = 100;
	int errorCode;
	u16 tmp_len = 0;
	const int SIZE_OF_HW_MONITOR_HEADER = 4;

	memset(data, 0, DS5_HWMC_BUFFER_SIZE);

	do {
		if (retries != 100)
			msleep_range(1);
		ret = ds5_read(state, DS5_HWMC_STATUS, &status);
	} while (!ret && retries-- && status != DS5_HWMC_STATUS_OK);

	if (ret || status != DS5_HWMC_STATUS_OK) {
		if (status == DS5_HWMC_STATUS_ERR) {
			ds5_raw_read(state, DS5_HWMC_DATA, &errorCode, sizeof(errorCode));
			dev_err(&state->client->dev,
					"%s(): HWMC failed, ret: %d, error code: %d\n",
					__func__, ret, errorCode);
		} else {
			dev_err(&state->client->dev,
					"%s(): HWMC failed because of timeout, ret: %d\n",
					__func__, ret);
		}
		return -EAGAIN;
	}

	ret = regmap_raw_read(state->regmap, DS5_HWMC_RESP_LEN,
			&tmp_len, sizeof(tmp_len));
	if (ret)
		return -EAGAIN;

	if (tmp_len > DS5_HWMC_BUFFER_SIZE)
		return -ENOBUFS;

	dev_dbg(&state->client->dev,
			"%s(): HWMC read len: %d, lrs_len: %d\n",
			__func__, tmp_len, tmp_len - SIZE_OF_HW_MONITOR_HEADER);

	ds5_raw_read_with_check(state, DS5_HWMC_DATA, data, tmp_len);

	/* This is needed for librealsense, to align there code with UVC,
	 * last word is length - 4 bytes header length */
	tmp_len -= SIZE_OF_HW_MONITOR_HEADER;
	data[DS5_HWMC_BUFFER_SIZE - 4] = (unsigned char)(tmp_len & 0x00FF);
	data[DS5_HWMC_BUFFER_SIZE - 3] = (unsigned char)((tmp_len & 0xFF00) >> 8);
	data[DS5_HWMC_BUFFER_SIZE - 2] = 0;
	data[DS5_HWMC_BUFFER_SIZE - 1] = 0;

	return 0;
}

static int ds5_set_calibration_data(struct ds5 *state,
		struct hwm_cmd *cmd, u16 length)
{
	int ret;
	int retries = 10;
	u16 status = 2;

	ds5_raw_write_with_check(state, 0x4900, cmd, length);

	ds5_write_with_check(state, 0x490c, 0x01); /* execute cmd */
	do {
		if (retries != 10)
			msleep_range(200);
		ret = ds5_read(state, 0x4904, &status);
	} while (retries-- && status != 0);

	if (ret || status != 0) {
		dev_err(&state->client->dev,
				"%s(): Failed to set calibration table %d,"
				"ret: %d, fw error: %x\n",
				__func__, cmd->param1, ret, status);
	}

	return -EINVAL;
}

static int ds5_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ds5 *state = container_of(ctrl->handler, struct ds5,
					 ctrls.handler);
	struct v4l2_subdev *sd = &state->mux.sd.subdev;
	int ret = 0;
	u16 base = DS5_DEPTH_CONTROL_BASE;

	if (state->is_rgb)
		base = DS5_RGB_CONTROL_BASE;
	else if (state->is_imu)
		return -EINVAL;

	v4l2_dbg(3, 1, sd, "ctrl: %s, value: %d\n", ctrl->name, ctrl->val);

	mutex_lock(&state->lock);

	switch (ctrl->id) {
	case V4L2_CID_ANALOGUE_GAIN:
		ret = ds5_write(state, base | DS5_MANUAL_GAIN, ctrl->val);
		break;

	case V4L2_CID_EXPOSURE_AUTO:
		ret = ds5_hw_set_auto_exposure(state, base, ctrl->val);
		break;

	case V4L2_CID_EXPOSURE_ABSOLUTE:
		ret = ds5_hw_set_exposure(state, base, ctrl->val);
		break;
	case DS5_CAMERA_CID_LASER_POWER:
		if (!state->is_rgb)
			ret = ds5_write(state, base | DS5_LASER_POWER,
					ctrl->val);
		break;
	case DS5_CAMERA_CID_MANUAL_LASER_POWER:
		if (!state->is_rgb)
			ret = ds5_write(state, base | DS5_MANUAL_LASER_POWER,
					ctrl->val);
		break;
	case DS5_CAMERA_DEPTH_CALIBRATION_TABLE_SET: {
		struct hwm_cmd *calib_cmd;

		dev_dbg(&state->client->dev,
				"%s(): DS5_CAMERA_DEPTH_CALIBRATION_TABLE_SET \n",
				__func__);
		dev_dbg(&state->client->dev,
				"%s(): table id: 0x%x\n",
				__func__, *((u8*)ctrl->p_new.p + 2));
		if (ctrl->p_new.p && DEPTH_CALIBRATION_ID ==
				*((u8*)ctrl->p_new.p + 2)) {
			calib_cmd = devm_kzalloc(&state->client->dev,
					sizeof(struct hwm_cmd) + 256, GFP_KERNEL);
			if (!calib_cmd) {
				dev_err(&state->client->dev,
						"%s(): Can't allocate memory for 0x%x\n",
						__func__, ctrl->id);
				ret = -ENOMEM;
				break;
			}
			memcpy(calib_cmd, &set_calib_data, sizeof (set_calib_data));
			calib_cmd->header = 276;
			calib_cmd->param1 = DEPTH_CALIBRATION_ID;
			memcpy(calib_cmd->Data, (u8*)ctrl->p_new.p , 256);
			ret = ds5_set_calibration_data(state, calib_cmd,
					sizeof(struct hwm_cmd) + 256);
			devm_kfree(&state->client->dev, calib_cmd);
		}
		break;
		}
	case DS5_CAMERA_COEFF_CALIBRATION_TABLE_SET: {
			struct hwm_cmd *calib_cmd;

			dev_dbg(&state->client->dev,
					"%s(): DS5_CAMERA_COEFF_CALIBRATION_TABLE_SET \n",
					__func__);
			dev_dbg(&state->client->dev,
					"%s(): table id %d\n",
					__func__, *((u8*)ctrl->p_new.p + 2));
			if (ctrl->p_new.p && COEF_CALIBRATION_ID ==
					*((u8*)ctrl->p_new.p + 2)) {
				calib_cmd = devm_kzalloc(&state->client->dev,
						sizeof(struct hwm_cmd) + 512, GFP_KERNEL);
				if (!calib_cmd) {
					dev_err(&state->client->dev,
							"%s(): Can't allocate memory for 0x%x\n",
							__func__, ctrl->id);
					ret = -ENOMEM;
					break;
				}
				memcpy(calib_cmd, &set_calib_data, sizeof (set_calib_data));
				calib_cmd->header = 532;
				calib_cmd->param1 = COEF_CALIBRATION_ID;
				memcpy(calib_cmd->Data, (u8*)ctrl->p_new.p , 512);
				ret = ds5_set_calibration_data(state, calib_cmd,
						sizeof(struct hwm_cmd) + 512);
				devm_kfree(&state->client->dev, calib_cmd);
			}

		}
		break;
	case DS5_CAMERA_CID_AE_ROI_SET: {
		struct hwm_cmd ae_roi_cmd;
		memcpy(&ae_roi_cmd, &set_ae_roi, sizeof(ae_roi_cmd));
		ae_roi_cmd.param1 = *((u16*)ctrl->p_new.p_u16);
		ae_roi_cmd.param2 = *((u16*)ctrl->p_new.p_u16 + 1);
		ae_roi_cmd.param3 = *((u16*)ctrl->p_new.p_u16 + 2);
		ae_roi_cmd.param4 = *((u16*)ctrl->p_new.p_u16 + 3);
		ret = ds5_send_hwmc(state, sizeof(struct hwm_cmd),
				&ae_roi_cmd, false, NULL);
		break;
		}
	case DS5_CAMERA_CID_AE_SETPOINT_SET: {
		struct hwm_cmd *ae_setpoint_cmd;
		if (ctrl->p_new.p_s32) {
			dev_dbg(&state->client->dev, "%s():0x%x \n",
					__func__, *(ctrl->p_new.p_s32));
			ae_setpoint_cmd = devm_kzalloc(&state->client->dev,
					sizeof(struct hwm_cmd) + 4, GFP_KERNEL);
			if (!ae_setpoint_cmd) {
				dev_err(&state->client->dev,
						"%s(): Can't allocate memory for 0x%x\n",
						__func__, ctrl->id);
				ret = -ENOMEM;
				break;
			}
			memcpy(ae_setpoint_cmd, &set_ae_setpoint, sizeof (set_ae_setpoint));
			memcpy(ae_setpoint_cmd->Data, (u8*)ctrl->p_new.p_s32 , 4);
			ret = ds5_send_hwmc(state, sizeof(struct hwm_cmd) + 4,
					ae_setpoint_cmd, false, NULL);
			devm_kfree(&state->client->dev, ae_setpoint_cmd);
		}
		break;
	}
	case DS5_CAMERA_CID_ERB:
		if (ctrl->p_new.p_u8) {
			u16 offset = 0;
			u16 size = 0;
			struct hwm_cmd *erb_cmd;

			offset = *(ctrl->p_new.p_u8) << 8;
			offset |= *(ctrl->p_new.p_u8 + 1);
			size = *(ctrl->p_new.p_u8 + 2) << 8;
			size |= *(ctrl->p_new.p_u8 + 3);

			dev_dbg(&state->client->dev, "%s(): offset %x, size: %x\n",
							__func__, offset, size);

			erb_cmd = devm_kzalloc(&state->client->dev,
					sizeof(struct hwm_cmd) + size, GFP_KERNEL);
			if (!erb_cmd) {
				dev_err(&state->client->dev,
						"%s(): Can't allocate memory for 0x%x\n",
						__func__, ctrl->id);
				ret = -ENOMEM;
				break;
			}
			memcpy(erb_cmd, &erb, sizeof(struct hwm_cmd));
			erb_cmd->param1 = offset;
			erb_cmd->param2 = size;
			ret = ds5_send_hwmc(state, sizeof(struct hwm_cmd),
					erb_cmd, true, &size);

			if (ret) {
				dev_err(&state->client->dev,
						"%s(): ERB cmd failed, ret: %d,"
						"requested size: %d, actual size: %d\n",
						__func__, ret, erb_cmd->param2, size);
				devm_kfree(&state->client->dev, erb_cmd);
				return -EAGAIN;
			}

			// Actual size returned from FW
			*(ctrl->p_new.p_u8 + 2) = (size & 0xFF00) >> 8;
			*(ctrl->p_new.p_u8 + 3) = (size & 0x00FF);

			memcpy(ctrl->p_new.p_u8 + 4, erb_cmd->Data + 4, size - 4);
			dev_dbg(&state->client->dev, "%s(): 0x%x 0x%x 0x%x 0x%x \n",
				__func__,
				*(ctrl->p_new.p_u8),
				*(ctrl->p_new.p_u8+1),
				*(ctrl->p_new.p_u8+2),
				*(ctrl->p_new.p_u8+3));
			devm_kfree(&state->client->dev, erb_cmd);
		}
		break;
	case DS5_CAMERA_CID_EWB:
		if (ctrl->p_new.p_u8) {
			u16 offset = 0;
			u16 size = 0;
			struct hwm_cmd *ewb_cmd;

			offset = *((u8*)ctrl->p_new.p_u8) << 8;
			offset |= *((u8*)ctrl->p_new.p_u8 + 1);
			size = *((u8*)ctrl->p_new.p_u8 + 2) << 8;
			size |= *((u8*)ctrl->p_new.p_u8 + 3);

			dev_dbg(&state->client->dev, "%s():0x%x 0x%x 0x%x 0x%x\n",
					__func__,
					*((u8*)ctrl->p_new.p_u8),
					*((u8*)ctrl->p_new.p_u8 + 1),
					*((u8*)ctrl->p_new.p_u8 + 2),
					*((u8*)ctrl->p_new.p_u8 + 3));

			ewb_cmd = devm_kzalloc(&state->client->dev,
					sizeof(struct hwm_cmd) + size,
					GFP_KERNEL);
			if (!ewb_cmd) {
				dev_err(&state->client->dev,
						"%s(): Can't allocate memory for 0x%x\n",
						__func__, ctrl->id);
				ret = -ENOMEM;
				break;
			}
			memcpy(ewb_cmd, &ewb, sizeof (ewb));
			ewb_cmd->header = 0x14 + size;
			ewb_cmd->param1 = offset; // start index
			ewb_cmd->param2 = size; // size
			memcpy(ewb_cmd->Data, (u8*)ctrl->p_new.p_u8 + 4, size);
			ret = ds5_send_hwmc(state, sizeof(struct hwm_cmd) + size,
					ewb_cmd, false, NULL);

			if (ret) {
				dev_err(&state->client->dev,
						"%s(): EWB cmd failed, ret: %d,"
						"requested size: %d, actual size: %d\n",
						__func__, ret, ewb_cmd->param2, size);
				devm_kfree(&state->client->dev, ewb_cmd);
				return -EAGAIN;
			}

			devm_kfree(&state->client->dev, ewb_cmd);
		}
		break;
	case DS5_CAMERA_CID_HWMC:
		if (ctrl->p_new.p_u8) {
			u16 dataLen = 0;
			u16 size = 0;

			size = *((u8*)ctrl->p_new.p_u8 + 1) << 8;
			size |= *((u8*)ctrl->p_new.p_u8 + 0);
			dev_dbg(&state->client->dev,
					"%s(): HWMC size %d\n",
					__func__, size);
			ret = ds5_send_hwmc(state, size + 4,
					(struct hwm_cmd *)ctrl->p_new.p_u8, true, &dataLen);
		}
		break;
	case DS5_CAMERA_CID_HWMC_RW:
		if (ctrl->p_new.p_u8) {
			u16 size = *((u8*)ctrl->p_new.p_u8 + 1) << 8;
			size |= *((u8*)ctrl->p_new.p_u8 + 0);
			ret = ds5_send_hwmc(state, size + 4,
					(struct hwm_cmd *)ctrl->p_new.p_u8, false, NULL);
		}
		break;
	case DS5_CAMERA_CID_PWM:
		if (state->is_depth)
			ret = ds5_write(state, base | DS5_PWM_FREQUENCY, ctrl->val);
		break;
	}

	mutex_unlock(&state->lock);

	return ret;
}

static int ds5_get_calibration_data(struct ds5 *state, enum table_id id,
		unsigned char *table, unsigned int length)
{
	struct hwm_cmd *cmd;
	int ret;
	int retries = 3;
	u16 status = 2;
	u16 table_length;

	cmd = devm_kzalloc(&state->client->dev,
			sizeof(struct hwm_cmd) + length + 4, GFP_KERNEL);
	if (!cmd) {
		dev_err(&state->client->dev, "%s(): Can't allocate memory\n", __func__);
		return -ENOMEM;
	}

	memcpy(cmd, &get_calib_data, sizeof(get_calib_data));
	cmd->param1 = id;
	ds5_raw_write_with_check(state, 0x4900, cmd, sizeof(struct hwm_cmd));
	ds5_write_with_check(state, 0x490c, 0x01); /* execute cmd */
	do {
		if (retries != 3)
			msleep_range(10);
		ret = ds5_read(state, 0x4904, &status);
	} while (ret && retries-- && status != 0);

	if (ret || status != 0) {
		dev_err(&state->client->dev,
				"%s(): Failed to get calibration table %d, fw error: %x\n",
				__func__, id, status);
		devm_kfree(&state->client->dev, cmd);
		return status;
	}

	// get table length from fw
	ret = regmap_raw_read(state->regmap, 0x4908,
			&table_length, sizeof(table_length));

	// read table
	ds5_raw_read_with_check(state, 0x4900, cmd->Data, table_length);

	// first 4 bytes are opcode HWM, not part of calibration table
	memcpy(table, cmd->Data + 4, length);
	devm_kfree(&state->client->dev, cmd);
	return 0;
}

static int ds5_gvd(struct ds5 *state, unsigned char *data)
{
	struct hwm_cmd cmd;
	int ret;
	u16 length = 0;
	u16 status = 2;
	u8 retries = 3;

	memcpy(&cmd, &gvd, sizeof(gvd));
	ds5_raw_write_with_check(state, 0x4900, &cmd, sizeof(cmd));
	ds5_write_with_check(state, 0x490c, 0x01); /* execute cmd */
	do {
		if (retries != 3)
			msleep_range(10);

		ret = ds5_read(state, 0x4904, &status);
	} while (ret && retries-- && status != 0);

	if (ret || status != 0) {
		dev_err(&state->client->dev,
				"%s(): Failed to read GVD, HWM cmd status: %x\n",
				__func__, status);
		return status;
	}

	ret = regmap_raw_read(state->regmap, 0x4908, &length, sizeof(length));
	ds5_raw_read_with_check(state, 0x4900, data, length);

	return ret;
}


static int ds5_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ds5 *state = container_of(ctrl->handler, struct ds5,
			ctrls.handler);
	u16 log_prepare[] = {0x0014, 0xcdab, 0x000f, 0x0000, 0x0400, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};
	u16 execute_cmd = 0x0001;
	unsigned int i;
	u32 data;
	int ret = 0;
	u16 base = (state->is_rgb) ? DS5_RGB_CONTROL_BASE : DS5_DEPTH_CONTROL_BASE;
	u16 reg;

	dev_dbg(&state->client->dev, "%s(): ctrl: %s \n",
		__func__, ctrl->name);

	switch (ctrl->id) {

	case V4L2_CID_ANALOGUE_GAIN:
		if (state->is_imu)
			return -EINVAL;
		ret = ds5_read(state, base | DS5_MANUAL_GAIN, ctrl->p_new.p_u16);
		break;

	case V4L2_CID_EXPOSURE_AUTO:
		if (state->is_imu)
			return -EINVAL;
		ds5_read(state, base | DS5_AUTO_EXPOSURE_MODE, &reg);
		*ctrl->p_new.p_u16 = reg;
		/* see ds5_hw_set_auto_exposure */
		if (!state->is_rgb) {
			if (reg == 1)
				*ctrl->p_new.p_u16 = V4L2_EXPOSURE_APERTURE_PRIORITY;
			else if (reg == 0)
				*ctrl->p_new.p_u16 = V4L2_EXPOSURE_MANUAL;
		}

		if (state->is_rgb && reg == 8)
			*ctrl->p_new.p_u16 = V4L2_EXPOSURE_APERTURE_PRIORITY;

		break;

	case V4L2_CID_EXPOSURE_ABSOLUTE:
		if (state->is_imu)
			return -EINVAL;
		/* see ds5_hw_set_exposure */
		ds5_read(state, base | DS5_MANUAL_EXPOSURE_MSB, &reg);
		data = ((u32)reg << 16) & 0xffff0000;
		ds5_read(state, base | DS5_MANUAL_EXPOSURE_LSB, &reg);
		data |= reg;
		*ctrl->p_new.p_u32 = data;
		break;

	case DS5_CAMERA_CID_LASER_POWER:
		if (!state->is_rgb)
			ds5_read(state, base | DS5_LASER_POWER, ctrl->p_new.p_u16);
		break;

	case DS5_CAMERA_CID_MANUAL_LASER_POWER:
		if (!state->is_rgb)
			ds5_read(state, base | DS5_MANUAL_LASER_POWER, ctrl->p_new.p_u16);
		break;

	case DS5_CAMERA_CID_LOG:
		// TODO: wrap HWMonitor command
		//       1. prepare and send command
		//       2. send command
		//       3. execute command
		//       4. wait for ccompletion
		ret = regmap_raw_write(state->regmap, 0x4900,
				log_prepare, sizeof(log_prepare));
		if (ret < 0)
			return ret;

		ret = regmap_raw_write(state->regmap, 0x490C,
				&execute_cmd, sizeof(execute_cmd));
		if (ret < 0)
			return ret;

		for (i = 0; i < DS5_MAX_LOG_POLL; i++) {
			ret = regmap_raw_read(state->regmap, 0x4904,
					&data, sizeof(data));
			dev_dbg(&state->client->dev, "%s(): log ready 0x%x\n",
				 __func__, data);
			if (ret < 0)
				return ret;
			if (!data)
				break;
			msleep_range(5);
		}

//		if (i == DS5_MAX_LOG_POLL)
//			return -ETIMEDOUT;

		ret = regmap_raw_read(state->regmap, 0x4908,
				&data, sizeof(data));
		dev_dbg(&state->client->dev, "%s(): log size 0x%x\n",
			 __func__, data);
		if (ret < 0)
			return ret;
		if (!data)
			return 0;
		if (data > 1024)
			return -ENOBUFS;
		ret = regmap_raw_read(state->regmap, 0x4900,
				ctrl->p_new.p_u8, data);
		break;
	case DS5_CAMERA_DEPTH_CALIBRATION_TABLE_GET:
		ret = ds5_get_calibration_data(state, DEPTH_CALIBRATION_ID,
				ctrl->p_new.p_u8, 256);
		break;
	case DS5_CAMERA_COEFF_CALIBRATION_TABLE_GET:
		ret = ds5_get_calibration_data(state, COEF_CALIBRATION_ID,
				ctrl->p_new.p_u8, 512);
		break;
	case DS5_CAMERA_CID_FW_VERSION:
		ret = ds5_read(state, DS5_FW_VERSION, &state->fw_version);
		ret = ds5_read(state, DS5_FW_BUILD, &state->fw_build);
		*ctrl->p_new.p_u32 = state->fw_version << 16;
		*ctrl->p_new.p_u32 |= state->fw_build;
		break;
	case DS5_CAMERA_CID_GVD:
		ret = ds5_gvd(state, ctrl->p_new.p_u8);
		break;
	case DS5_CAMERA_CID_AE_ROI_GET: {
		u16 len = 0;
		struct hwm_cmd *ae_roi_cmd;
		ae_roi_cmd = devm_kzalloc(&state->client->dev,
				sizeof(struct hwm_cmd) + 12, GFP_KERNEL);
		if (!ae_roi_cmd) {
			dev_err(&state->client->dev,
					"%s(): Can't allocate memory for 0x%x\n",
					__func__, ctrl->id);
			ret = -ENOMEM;
			break;
		}
		memcpy(ae_roi_cmd, &get_ae_roi, sizeof(struct hwm_cmd));
		ret = ds5_send_hwmc(state, sizeof(struct hwm_cmd),
				ae_roi_cmd, true, &len);
		memcpy(ctrl->p_new.p_u16, ae_roi_cmd->Data + 4, 8);
		devm_kfree(&state->client->dev, ae_roi_cmd);
		}
		break;
	case DS5_CAMERA_CID_AE_SETPOINT_GET: {
		u16 len = 0;
		struct hwm_cmd *ae_setpoint_cmd;
		ae_setpoint_cmd = devm_kzalloc(&state->client->dev,
				sizeof(struct hwm_cmd) + 8, GFP_KERNEL);
		if (!ae_setpoint_cmd) {
			dev_err(&state->client->dev,
					"%s(): Can't allocate memory for 0x%x\n",
					__func__, ctrl->id);
			ret = -ENOMEM;
			break;
		}
		memcpy(ae_setpoint_cmd, &get_ae_setpoint, sizeof(struct hwm_cmd));
		ret = ds5_send_hwmc(state, sizeof(struct hwm_cmd),
				ae_setpoint_cmd, true, &len);
		memcpy(ctrl->p_new.p_s32, ae_setpoint_cmd->Data + 4, 4);
		dev_dbg(&state->client->dev, "%s(): 0x%x \n",
					__func__,
					*(ctrl->p_new.p_s32));
		devm_kfree(&state->client->dev, ae_setpoint_cmd);
		}
		break;
	case DS5_CAMERA_CID_HWMC_RW:
		ds5_get_hwmc(state, ctrl->p_new.p_u8);
		break;
	case DS5_CAMERA_CID_PWM:
		if (state->is_depth)
			ds5_read(state, base | DS5_PWM_FREQUENCY, ctrl->p_new.p_u16);
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops ds5_ctrl_ops = {
	.s_ctrl	= ds5_s_ctrl,
	.g_volatile_ctrl = ds5_g_volatile_ctrl,
};

static const struct v4l2_ctrl_config ds5_ctrl_log = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_CID_LOG,
	.name = "Logger",
	.type = V4L2_CTRL_TYPE_U8,
	.dims = {1024},
	.elem_size = sizeof(u8),
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_READ_ONLY,
	.step = 1,
};

static const struct v4l2_ctrl_config ds5_ctrl_laser_power = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_CID_LASER_POWER,
	.name = "Laser power on/off",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 1,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
};

static const struct v4l2_ctrl_config ds5_ctrl_manual_laser_power = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_CID_MANUAL_LASER_POWER,
	.name = "Manual laser power",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 360,
	.step = 30,
	.def = 150,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
};

static const struct v4l2_ctrl_config ds5_ctrl_fw_version = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_CID_FW_VERSION,
	.name = "fw version",
	.type = V4L2_CTRL_TYPE_U32,
	.dims = {1},
	.elem_size = sizeof(u32),
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_READ_ONLY,
	.step = 1,
};

static const struct v4l2_ctrl_config ds5_ctrl_gvd = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_CID_GVD,
	.name = "GVD",
	.type = V4L2_CTRL_TYPE_U8,
	.dims = {239},
	.elem_size = sizeof(u8),
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_READ_ONLY,
	.step = 1,
};

static const struct v4l2_ctrl_config ds5_ctrl_get_depth_calib = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_DEPTH_CALIBRATION_TABLE_GET,
	.name = "get depth calib",
	.type = V4L2_CTRL_TYPE_U8,
	.dims = {256},
	.elem_size = sizeof(u8),
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_READ_ONLY,
	.step = 1,
};

static const struct v4l2_ctrl_config ds5_ctrl_set_depth_calib = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_DEPTH_CALIBRATION_TABLE_SET,
	.name = "set depth calib",
	.type = V4L2_CTRL_TYPE_U8,
	.dims = {256},
	.elem_size = sizeof(u8),
	.min = 0,
	.max = 0xFFFFFFFF,
	.def = 240,
	.step = 1,
};

static const struct v4l2_ctrl_config ds5_ctrl_get_coeff_calib = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_COEFF_CALIBRATION_TABLE_GET,
	.name = "get coeff calib",
	.type = V4L2_CTRL_TYPE_U8,
	.dims = {512},
	.elem_size = sizeof(u8),
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_READ_ONLY,
	.step = 1,
};

static const struct v4l2_ctrl_config ds5_ctrl_set_coeff_calib = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_COEFF_CALIBRATION_TABLE_SET,
	.name = "set coeff calib",
	.type = V4L2_CTRL_TYPE_U8,
	.dims = {512},
	.elem_size = sizeof(u8),
	.min = 0,
	.max = 0xFFFFFFFF,
	.def = 240,
	.step = 1,
};

static const struct v4l2_ctrl_config ds5_ctrl_ae_roi_get = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_CID_AE_ROI_GET,
	.name = "ae roi get",
	.type = V4L2_CTRL_TYPE_U8,
	.dims = {8},
	.elem_size = sizeof(u16),
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_READ_ONLY,
	.step = 1,
};

static const struct v4l2_ctrl_config ds5_ctrl_ae_roi_set = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_CID_AE_ROI_SET,
	.name = "ae roi set",
	.type = V4L2_CTRL_TYPE_U8,
	.dims = {8},
	.elem_size = sizeof(u16),
	.min = 0,
	.max = 0xFFFFFFFF,
	.def = 240,
	.step = 1,
};

static const struct v4l2_ctrl_config ds5_ctrl_ae_setpoint_get = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_CID_AE_SETPOINT_GET,
	.name = "ae setpoint get",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_READ_ONLY,
	.step = 1,
};

static const struct v4l2_ctrl_config ds5_ctrl_ae_setpoint_set = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_CID_AE_SETPOINT_SET,
	.name = "ae setpoint set",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 4095,
	.step = 1,
	.def = 0,
};

static const struct v4l2_ctrl_config ds5_ctrl_erb = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_CID_ERB,
	.name = "ERB eeprom read",
	.type = V4L2_CTRL_TYPE_U8,
	.dims = {1020},
	.elem_size = sizeof(u8),
	.min = 0,
	.max = 0xFFFFFFFF,
	.def = 240,
	.step = 1,
	.step = 1,
};

static const struct v4l2_ctrl_config ds5_ctrl_ewb = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_CID_EWB,
	.name = "EWB eeprom write",
	.type = V4L2_CTRL_TYPE_U8,
	.dims = {1020},
	.elem_size = sizeof(u8),
	.min = 0,
	.max = 0xFFFFFFFF,
	.def = 240,
	.step = 1,
	.step = 1,
};

static const struct v4l2_ctrl_config ds5_ctrl_hwmc = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_CID_HWMC,
	.name = "HWMC",
	.type = V4L2_CTRL_TYPE_U8,
	.dims = {DS5_HWMC_BUFFER_SIZE + 4},
	.elem_size = sizeof(u8),
	.min = 0,
	.max = 0xFFFFFFFF,
	.def = 240,
	.step = 1,
	.step = 1,
};

static const struct v4l2_ctrl_config ds5_ctrl_hwmc_rw = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_CID_HWMC_RW,
	.name = "HWMC_RW",
	.type = V4L2_CTRL_TYPE_U8,
	.dims = {DS5_HWMC_BUFFER_SIZE},
	.elem_size = sizeof(u8),
	.min = 0,
	.max = 0xFFFFFFFF,
	.def = 240,
	.step = 1,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
};

static const struct v4l2_ctrl_config ds5_ctrl_pwm = {
	.ops = &ds5_ctrl_ops,
	.id = DS5_CAMERA_CID_PWM,
	.name = "PWM Frequency Selector",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 1,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
};

static int ds5_mux_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct ds5 *state = v4l2_get_subdevdata(sd);

	dev_dbg(sd->dev, "%s(): %s (%p)\n", __func__, sd->name, fh);
	if (state->dfu_dev.dfu_state_flag)
		return -EBUSY;
	state->dfu_dev.device_open_count++;

	return 0;
};

static int ds5_mux_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct ds5 *state = v4l2_get_subdevdata(sd);

	dev_dbg(sd->dev, "%s(): %s (%p)\n", __func__, sd->name, fh);
	state->dfu_dev.device_open_count--;
	return 0;
};

static const struct v4l2_subdev_internal_ops ds5_sensor_internal_ops = {
	.open = ds5_mux_open,
	.close = ds5_mux_close,
};

static int ds5_ctrl_init(struct ds5 *state)
{
	const struct v4l2_ctrl_ops *ops = &ds5_ctrl_ops;
	struct ds5_ctrls *ctrls = &state->ctrls;
	struct v4l2_ctrl_handler *hdl = &ctrls->handler;
	struct v4l2_subdev *sd = &state->mux.sd.subdev;
	int ret;

	if (state->is_imu)
		return 0;

	ret = v4l2_ctrl_handler_init(hdl, DS5_N_CONTROLS);
	if (ret < 0) {
		v4l2_err(sd, "cannot init ctrl handler (%d)\n", ret);
		return ret;
	}

	if (state->is_depth || state->is_y8) {
		ctrls->laser_power = v4l2_ctrl_new_custom(hdl,
						&ds5_ctrl_laser_power,
						NULL);
		ctrls->manual_laser_power = v4l2_ctrl_new_custom(hdl,
						&ds5_ctrl_manual_laser_power,
						NULL);
	}

	/* Total gain */
	if (state->is_depth || state->is_y8) {
		ctrls->gain = v4l2_ctrl_new_std(hdl, ops,
						V4L2_CID_ANALOGUE_GAIN,
						16, 248, 1, 16);
	} else if (state->is_rgb) {
		ctrls->gain = v4l2_ctrl_new_std(hdl, ops,
						V4L2_CID_ANALOGUE_GAIN,
						0, 128, 1, 64);
	}

	if (ctrls->gain)
		ctrls->gain->flags =
				V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE;

	ctrls->auto_exp = v4l2_ctrl_new_std_menu(hdl, ops,
			V4L2_CID_EXPOSURE_AUTO,
			V4L2_EXPOSURE_APERTURE_PRIORITY,
			~((1 << V4L2_EXPOSURE_MANUAL) |
					(1 << V4L2_EXPOSURE_APERTURE_PRIORITY)),
					V4L2_EXPOSURE_APERTURE_PRIORITY);

	if (ctrls->auto_exp)
		ctrls->auto_exp->flags |=
				V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE;

	/* Exposure time: V4L2_CID_EXPOSURE_ABSOLUTE default unit: 100 us. */
	if (state->is_depth || state->is_y8) {
		ctrls->exposure = v4l2_ctrl_new_std(hdl, ops,
					V4L2_CID_EXPOSURE_ABSOLUTE,
					1, MAX_DEPTH_EXP, 1, DEF_DEPTH_EXP);
	} else if (state->is_rgb) {
		ctrls->exposure = v4l2_ctrl_new_std(hdl, ops,
					V4L2_CID_EXPOSURE_ABSOLUTE,
					1, MAX_RGB_EXP, 1, DEF_RGB_EXP);
	}

	if (ctrls->exposure) {
		ctrls->exposure->flags |=
				V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE;
		/* override default int type to u32 to match SKU & UVC */
		ctrls->exposure->type = V4L2_CTRL_TYPE_U32;
	}

	if (hdl->error) {
		v4l2_err(sd, "error creating controls (%d)\n", hdl->error);
		ret = hdl->error;
		v4l2_ctrl_handler_free(hdl);
		return ret;
	}

	ret = v4l2_ctrl_handler_setup(hdl);
	if (ret < 0) {
		dev_err(&state->client->dev,
			"failed to set default values for controls\n");
		v4l2_ctrl_handler_free(hdl);
		return ret;
	}

	// Add these after v4l2_ctrl_handler_setup so they won't be set up
	ctrls->log = v4l2_ctrl_new_custom(hdl, &ds5_ctrl_log, NULL);
	ctrls->fw_version = v4l2_ctrl_new_custom(hdl, &ds5_ctrl_fw_version, NULL);
	ctrls->gvd = v4l2_ctrl_new_custom(hdl, &ds5_ctrl_gvd, NULL);
	ctrls->get_depth_calib =
			v4l2_ctrl_new_custom(hdl, &ds5_ctrl_get_depth_calib, NULL);
	ctrls->set_depth_calib =
			v4l2_ctrl_new_custom(hdl, &ds5_ctrl_set_depth_calib, NULL);
	ctrls->get_coeff_calib =
			v4l2_ctrl_new_custom(hdl, &ds5_ctrl_get_coeff_calib, NULL);
	ctrls->set_coeff_calib =
			v4l2_ctrl_new_custom(hdl, &ds5_ctrl_set_coeff_calib, NULL);
	ctrls->ae_roi_get = v4l2_ctrl_new_custom(hdl, &ds5_ctrl_ae_roi_get, NULL);
	ctrls->ae_roi_set = v4l2_ctrl_new_custom(hdl, &ds5_ctrl_ae_roi_set, NULL);
	ctrls->ae_setpoint_get =
			v4l2_ctrl_new_custom(hdl, &ds5_ctrl_ae_setpoint_get, NULL);
	ctrls->ae_setpoint_set =
			v4l2_ctrl_new_custom(hdl, &ds5_ctrl_ae_setpoint_set, NULL);
	ctrls->erb = v4l2_ctrl_new_custom(hdl, &ds5_ctrl_erb, NULL);
	ctrls->ewb = v4l2_ctrl_new_custom(hdl, &ds5_ctrl_ewb, NULL);
	ctrls->hwmc = v4l2_ctrl_new_custom(hdl, &ds5_ctrl_hwmc, NULL);
	v4l2_ctrl_new_custom(hdl, &ds5_ctrl_hwmc_rw, NULL);

	if (state->is_depth)
		v4l2_ctrl_new_custom(hdl, &ds5_ctrl_pwm, NULL);

	state->mux.sd.subdev.ctrl_handler = hdl;

	return 0;
}

static int ds5_sensor_init(struct i2c_client *c, struct ds5 *state,
		struct ds5_sensor *sensor, const struct v4l2_subdev_ops *ops,
		const char *name)
{
	struct v4l2_subdev *sd = &sensor->sd;
	struct media_entity *entity = &sensor->sd.entity;
	struct media_pad *pad = &sensor->pad;
	dev_t *dev_num = &state->client->dev.devt;

	dev_dbg(sd->dev, "%s(): %p %s %p %p",
			__func__, c, c->name, state, state->client);

	v4l2_i2c_subdev_init(sd, c, ops);
	// See tegracam_v4l2.c tegracam_v4l2subdev_register()
	// Set owner to NULL so we can unload the driver module
	sd->owner = NULL;
	sd->internal_ops = &ds5_sensor_internal_ops;
	sd->grp_id = *dev_num;
	v4l2_set_subdevdata(sd, state);
	snprintf(sd->name, sizeof(sd->name), "D4XX %s %d-%04x",
		 name, i2c_adapter_id(c->adapter), c->addr);

	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	pad->flags = MEDIA_PAD_FL_SOURCE;
	entity->obj_type = MEDIA_ENTITY_TYPE_V4L2_SUBDEV;
	entity->function = MEDIA_ENT_F_CAM_SENSOR;
	return media_entity_pads_init(entity, 1, pad);
}

static int ds5_sensor_register(struct ds5 *state, struct ds5_sensor *sensor)
{
	struct v4l2_subdev *sd = &sensor->sd;
	struct media_entity *entity = &sensor->sd.entity;
	int ret;

	// FIXME: is async needed?
	ret = v4l2_device_register_subdev(state->mux.sd.subdev.v4l2_dev, sd);
	if (ret < 0) {
		dev_err(sd->dev, "%s(): %d: %d\n", __func__, __LINE__, ret);
		return ret;
	}

	ret = media_create_pad_link(entity, 0,
			&state->mux.sd.subdev.entity, sensor->mux_pad,
			MEDIA_LNK_FL_IMMUTABLE | MEDIA_LNK_FL_ENABLED);
	if (ret < 0) {
		dev_err(sd->dev, "%s(): %d: %d\n", __func__, __LINE__, ret);
		goto e_sd;
	}

	dev_dbg(sd->dev, "%s(): 0 -> %d\n", __func__, sensor->mux_pad);

	return 0;

e_sd:
	v4l2_device_unregister_subdev(sd);

	return ret;
}

static void ds5_sensor_remove(struct ds5_sensor *sensor)
{
	v4l2_device_unregister_subdev(&sensor->sd);

	media_entity_cleanup(&sensor->sd.entity);
}

static int ds5_depth_init(struct i2c_client *c, struct ds5 *state)
{
	/* Which mux pad we're connecting to */
	state->depth.sensor.mux_pad = DS5_MUX_PAD_DEPTH;
	return ds5_sensor_init(c, state, &state->depth.sensor,
			&ds5_depth_subdev_ops, "depth");
}

static int ds5_motion_t_init(struct i2c_client *c, struct ds5 *state)
{
	state->motion_t.sensor.mux_pad = DS5_MUX_PAD_MOTION_T;
	return ds5_sensor_init(c, state, &state->motion_t.sensor,
			&ds5_motion_t_subdev_ops, "motion detection");
}

static int ds5_rgb_init(struct i2c_client *c, struct ds5 *state)
{
	state->rgb.sensor.mux_pad = DS5_MUX_PAD_RGB;
	return ds5_sensor_init(c, state, &state->rgb.sensor,
			&ds5_rgb_subdev_ops, "rgb");
}

static int ds5_imu_init(struct i2c_client *c, struct ds5 *state)
{
	state->imu.sensor.mux_pad = DS5_MUX_PAD_IMU;
	return ds5_sensor_init(c, state, &state->imu.sensor,
			&ds5_imu_subdev_ops, "imu");
}

/* No locking needed */
static int ds5_mux_enum_mbus_code(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_mbus_code_enum *mce)
{
	struct ds5 *state = container_of(sd, struct ds5, mux.sd.subdev);
	struct v4l2_subdev_mbus_code_enum tmp = *mce;
	struct v4l2_subdev *remote_sd;
	int ret;

	switch (mce->pad) {
	case DS5_MUX_PAD_MOTION_T:
		remote_sd = &state->motion_t.sensor.sd;
		break;
	case DS5_MUX_PAD_DEPTH:
		remote_sd = &state->depth.sensor.sd;
		break;
	case DS5_MUX_PAD_RGB:
		remote_sd = &state->rgb.sensor.sd;
		break;
	case DS5_MUX_PAD_IMU:
		remote_sd = &state->imu.sensor.sd;
		break;
	case DS5_MUX_PAD_EXTERNAL:
		if (mce->index >= state->motion_t.sensor.n_formats +
				state->depth.sensor.n_formats)
			return -EINVAL;

		/*
		 * First list Left node / Motion Tracker formats, then depth.
		 * This should also help because D16 doesn't have a direct
		 * analog in MIPI CSI-2.
		 */
		if (mce->index < state->motion_t.sensor.n_formats) {
			remote_sd = &state->motion_t.sensor.sd;
		} else {
			tmp.index = mce->index - state->motion_t.sensor.n_formats;
			remote_sd = &state->depth.sensor.sd;
		}

		break;
	default:
		return -EINVAL;
	}

	tmp.pad = 0;
	if (state->is_rgb)
		remote_sd = &state->rgb.sensor.sd;
	if (state->is_depth)
		remote_sd = &state->depth.sensor.sd;
	if (state->is_y8)
		remote_sd = &state->motion_t.sensor.sd;
	if (state->is_imu)
		remote_sd = &state->imu.sensor.sd;

	/* Locks internally */
	ret = ds5_sensor_enum_mbus_code(remote_sd, cfg, &tmp);
	if (!ret)
		mce->code = tmp.code;

	return ret;
}

/* No locking needed */
static int ds5_mux_enum_frame_size(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_frame_size_enum *fse)
{
	struct ds5 *state = container_of(sd, struct ds5, mux.sd.subdev);
	struct v4l2_subdev_frame_size_enum tmp = *fse;
	struct v4l2_subdev *remote_sd;
	u32 pad = fse->pad;
	int ret;

	tmp.pad = 0;

	if (state->is_depth)
		pad = DS5_MUX_PAD_DEPTH;
	if (state->is_y8)
		pad = DS5_MUX_PAD_MOTION_T;
	if (state->is_rgb)
		pad = DS5_MUX_PAD_RGB;
	if (state->is_imu)
		pad = DS5_MUX_PAD_IMU;

	switch (pad) {
	case DS5_MUX_PAD_MOTION_T:
		remote_sd = &state->motion_t.sensor.sd;
		break;
	case DS5_MUX_PAD_DEPTH:
		remote_sd = &state->depth.sensor.sd;
		break;
	case DS5_MUX_PAD_RGB:
		remote_sd = &state->rgb.sensor.sd;
		break;
	case DS5_MUX_PAD_IMU:
		remote_sd = &state->imu.sensor.sd;
		break;
	case DS5_MUX_PAD_EXTERNAL:
		/*
		 * Assume, that different sensors don't support the same formats
		 * Try the Depth sensor first, then the Motion Tracker
		 */
		remote_sd = &state->depth.sensor.sd;
		ret = ds5_sensor_enum_frame_size(remote_sd, NULL, &tmp);
		if (!ret) {
			*fse = tmp;
			fse->pad = pad;
			return 0;
		}

		remote_sd = &state->motion_t.sensor.sd;
		break;
	default:
		return -EINVAL;
	}

	/* Locks internally */
	ret = ds5_sensor_enum_frame_size(remote_sd, NULL, &tmp);
	if (!ret) {
		*fse = tmp;
		fse->pad = pad;
	}

	return ret;
}

/* No locking needed */
static int ds5_mux_enum_frame_interval(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_frame_interval_enum *fie)
{
	struct ds5 *state = container_of(sd, struct ds5, mux.sd.subdev);
	struct v4l2_subdev_frame_interval_enum tmp = *fie;
	struct v4l2_subdev *remote_sd;
	u32 pad = fie->pad;
	int ret;

	tmp.pad = 0;

	dev_dbg(state->depth.sensor.sd.dev,
			"%s(): pad %d code %x width %d height %d\n",
			__func__, pad, tmp.code, tmp.width, tmp.height);

	if (state->is_depth)
		pad = DS5_MUX_PAD_DEPTH;
	if (state->is_y8)
		pad = DS5_MUX_PAD_MOTION_T;
	if (state->is_rgb)
		pad = DS5_MUX_PAD_RGB;
	if (state->is_imu)
		pad = DS5_MUX_PAD_IMU;

	switch (pad) {
	case DS5_MUX_PAD_MOTION_T:
		remote_sd = &state->motion_t.sensor.sd;
		break;
	case DS5_MUX_PAD_DEPTH:
		remote_sd = &state->depth.sensor.sd;
		break;
	case DS5_MUX_PAD_RGB:
		remote_sd = &state->rgb.sensor.sd;
		break;
	case DS5_MUX_PAD_IMU:
		remote_sd = &state->imu.sensor.sd;
		break;
	case DS5_MUX_PAD_EXTERNAL:
		/* Similar to ds5_mux_enum_frame_size() above */
		if (state->is_rgb)
			remote_sd = &state->rgb.sensor.sd;
		else
			remote_sd = &state->motion_t.sensor.sd;
		ret = ds5_sensor_enum_frame_interval(remote_sd, NULL, &tmp);
		if (!ret) {
			*fie = tmp;
			fie->pad = pad;
			return 0;
		}

		remote_sd = &state->motion_t.sensor.sd;
		break;
	default:
		return -EINVAL;
	}

	/* Locks internally */
	ret = ds5_sensor_enum_frame_interval(remote_sd, NULL, &tmp);
	if (!ret) {
		*fie = tmp;
		fie->pad = pad;
	}

	return ret;
}

/* No locking needed */
static int ds5_mux_set_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_format *fmt)
{
	struct ds5 *state = container_of(sd, struct ds5, mux.sd.subdev);
	struct v4l2_subdev_format tmp = *fmt;
	struct v4l2_subdev *remote_sd;
	u32 pad = fmt->pad;
	int ret = 0;

	if (state->is_depth)
		pad = DS5_MUX_PAD_DEPTH;
	if (state->is_y8)
		pad = DS5_MUX_PAD_MOTION_T;
	if (state->is_rgb)
		pad = DS5_MUX_PAD_RGB;
	if (state->is_imu)
		pad = DS5_MUX_PAD_IMU;
	//if (!state->is_rgb)
	//	if (fmt->format.code == MEDIA_BUS_FMT_UYVY8_1X16)
	//		pad = DS5_MUX_PAD_DEPTH;
	//	else
	//		pad = DS5_MUX_PAD_MOTION_T;
	//else
	//	pad = DS5_MUX_PAD_RGB;

	dev_dbg(sd->dev, "%s(): pad: %x %x: %ux%u\n",
			__func__, pad, fmt->format.code,
			fmt->format.width, fmt->format.height);

	switch (pad) {
	case DS5_MUX_PAD_MOTION_T:
		remote_sd = &state->motion_t.sensor.sd;
		break;
	case DS5_MUX_PAD_DEPTH:
		remote_sd = &state->depth.sensor.sd;
		break;
	case DS5_MUX_PAD_RGB:
		remote_sd = &state->rgb.sensor.sd;
		break;
	case DS5_MUX_PAD_IMU:
		remote_sd = &state->imu.sensor.sd;
		break;
	case DS5_MUX_PAD_EXTERNAL:
		if (state->is_rgb)
			remote_sd = &state->rgb.sensor.sd;
		else
			remote_sd = &state->mux.last_set->sd;
		break;
	default:
		return -EINVAL;
	}

	tmp.pad = 0;

	/* Locks internally */
	ret = ds5_sensor_set_fmt(remote_sd, cfg, &tmp);
	if (!ret) {
		*fmt = tmp;
		fmt->pad = pad;
	}

	return ret;
}

/* No locking needed */
static int ds5_mux_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_format *fmt)
{
	struct ds5 *state = container_of(sd, struct ds5, mux.sd.subdev);
	struct v4l2_subdev_format tmp = *fmt;
	struct v4l2_subdev *remote_sd;
	u32 pad = fmt->pad;
	int ret;

	dev_dbg(sd->dev, "%s(): %u %p\n", __func__, pad, state->mux.last_set);

	if (state->is_depth)
		pad = DS5_MUX_PAD_DEPTH;
	if (state->is_y8)
		pad = DS5_MUX_PAD_MOTION_T;
	if (state->is_rgb)
		pad = DS5_MUX_PAD_RGB;
	if (state->is_imu)
		pad = DS5_MUX_PAD_IMU;

	switch (pad) {
	case DS5_MUX_PAD_MOTION_T:
		remote_sd = &state->motion_t.sensor.sd;
		break;
	case DS5_MUX_PAD_DEPTH:
		remote_sd = &state->depth.sensor.sd;
		break;
	case DS5_MUX_PAD_EXTERNAL:
		remote_sd = &state->mux.last_set->sd;
		break;
	case DS5_MUX_PAD_RGB:
		remote_sd = &state->rgb.sensor.sd;
		break;
	case DS5_MUX_PAD_IMU:
		remote_sd = &state->imu.sensor.sd;
		break;
	default:
		return -EINVAL;
	}


	/* Locks internally */
	ret = ds5_sensor_get_fmt(remote_sd, cfg, &tmp);
	if (!ret) {
		*fmt = tmp;
		fmt->pad = pad;
	}

	return ret;
}

/* Video ops */
static int ds5_mux_g_frame_interval(struct v4l2_subdev *sd,
		struct v4l2_subdev_frame_interval *fi)
{
	struct ds5 *state = container_of(sd, struct ds5, mux.sd.subdev);
	struct ds5_sensor *sensor = NULL;

	if (NULL == sd || NULL == fi)
		return -EINVAL;

	sensor = state->mux.last_set;

	fi->interval.numerator = 1;
	fi->interval.denominator = sensor->config.framerate;

	dev_dbg(sd->dev, "%s(): %s %u\n", __func__, sd->name,
			fi->interval.denominator);

	return 0;
}

static u16 __ds5_probe_framerate(const struct ds5_resolution *res, u16 target)
{
	int i;
	u16 framerate;

	for (i = 0; i < res->n_framerates; i++) {
		framerate = res->framerates[i];
		if (target <= framerate)
			return framerate;
	}

	return res->framerates[res->n_framerates - 1];
}

static int ds5_mux_s_frame_interval(struct v4l2_subdev *sd,
		struct v4l2_subdev_frame_interval *fi)
{
	struct ds5 *state = container_of(sd, struct ds5, mux.sd.subdev);
	struct ds5_sensor *sensor = NULL;
	u16 framerate = 1;

	if (NULL == sd || NULL == fi || fi->interval.numerator == 0)
		return -EINVAL;

	sensor = state->mux.last_set;

	framerate = fi->interval.denominator / fi->interval.numerator;
	framerate = __ds5_probe_framerate(sensor->config.resolution, framerate);
	sensor->config.framerate = framerate;
	fi->interval.numerator = 1;
	fi->interval.denominator = framerate;

	dev_dbg(sd->dev, "%s(): %s %u\n", __func__, sd->name, framerate);

	return 0;
}

static int ds5_mux_s_stream(struct v4l2_subdev *sd, int on)
{
	struct ds5 *state = container_of(sd, struct ds5, mux.sd.subdev);
	u16 streaming, status;
	int ret = 0;
	unsigned int i = 0;
	int restore_val = 0;
	u16 config_status_base, stream_status_base, stream_id;

	if (state->is_depth) {
		config_status_base = DS5_DEPTH_CONFIG_STATUS;
		stream_status_base = DS5_DEPTH_STREAM_STATUS;
		stream_id = DS5_STREAM_DEPTH;
	} else if (state->is_rgb) {
		config_status_base = DS5_RGB_CONFIG_STATUS;
		stream_status_base = DS5_RGB_STREAM_STATUS;
		stream_id = DS5_STREAM_RGB;
	} else if (state->is_y8) {
		config_status_base = DS5_IR_CONFIG_STATUS;
		stream_status_base = DS5_IR_STREAM_STATUS;
		stream_id = DS5_STREAM_IR;
	} else if (state->is_imu) {
		config_status_base = DS5_IMU_CONFIG_STATUS;
		stream_status_base = DS5_IMU_STREAM_STATUS;
		stream_id = DS5_STREAM_IMU;
	} else {
		return -EINVAL;
	}

	dev_dbg(&state->client->dev, "s_stream for stream %s, on = %d\n",
			state->mux.last_set->sd.name, on);

	restore_val = state->mux.last_set->streaming;
	state->mux.last_set->streaming = on;

	if (on) {
		ret = ds5_configure(state);
		if (ret)
			goto restore_s_state;

		ret = ds5_write(state, DS5_START_STOP_STREAM,
				DS5_STREAM_START | stream_id);
		if (ret < 0)
			goto restore_s_state;

		// check streaming status from FW
		for (i = 0; i < DS5_START_MAX_COUNT; i++) {
			ds5_read(state, stream_status_base, &streaming);
			ds5_read(state, config_status_base, &status);
			if ((status & DS5_STATUS_STREAMING) &&
					streaming == DS5_STREAM_STREAMING)
				break;

			msleep_range(DS5_START_POLL_TIME);
		}

		if (DS5_START_MAX_COUNT == i) {
			dev_err(&state->client->dev,
				"start streaming failed, exit on timeout\n");
			/* notify fw */
			ret = ds5_write(state, DS5_START_STOP_STREAM,
					DS5_STREAM_STOP | stream_id);
			ret = -EAGAIN;
			goto restore_s_state;
		} else {
			dev_dbg(&state->client->dev, "started after %dms\n",
				i * DS5_START_POLL_TIME);
		}
	} else {
		ret = ds5_write(state, DS5_START_STOP_STREAM,
				DS5_STREAM_STOP | stream_id);
		if (ret < 0)
			goto restore_s_state;
	}

	ds5_read(state, config_status_base, &status);
	ds5_read(state, stream_status_base, &streaming);
	dev_info(&state->client->dev,
			"%s %s, stream_status 0x%x:%x, config_status 0x%x:%x\n",
			ds5_get_sensor_name(state),
			(on)?"START":"STOP",
			stream_status_base, streaming,
			config_status_base, status);

	return ret;

restore_s_state:
	ds5_read(state, config_status_base, &status);
	dev_err(&state->client->dev,
			"%s stream toggle failed! %x status 0x%04x\n",
			ds5_get_sensor_name(state) ,restore_val, status);

	state->mux.last_set->streaming = restore_val;

	return ret;
}

//static int ds5_set_power(struct ds5 *state, int on)
//{
//	int ret = 0;
//
//	mutex_lock(&state->lock);
//
//	if (state->power != !on) {
//		mutex_unlock(&state->lock);
//		return 0;
//	}
//
////	gpio_set_value_cansleep(state->pwdn_gpio, on);
//
//	dev_info(&state->client->dev, "%s(): power %d\n", __func__, on);
//
//	usleep_range(100, 200);
//
//	if (on) {
//		state->power = true;
//	} else {
//		state->power = false;
//	}
//
//	mutex_unlock(&state->lock);
//
//	/* TODO: Restore controls when powering on */
//	//if (on)
//	//	ret = v4l2_ctrl_handler_setup(&state->ctrls.handler);
//
//	return ret;
//}

/* Core ops */
/*static int ds5_mux_set_power(struct v4l2_subdev *sd, int on)
{
	struct ds5 *state = container_of(sd, struct ds5, mux.sd.subdev);

	return ds5_set_power(state, on);
}*/

static const struct v4l2_subdev_pad_ops ds5_mux_pad_ops = {
	.enum_mbus_code		= ds5_mux_enum_mbus_code,
	.enum_frame_size	= ds5_mux_enum_frame_size,
	.enum_frame_interval	= ds5_mux_enum_frame_interval,
	.get_fmt		= ds5_mux_get_fmt,
	.set_fmt		= ds5_mux_set_fmt,
};

static const struct v4l2_subdev_core_ops ds5_mux_core_ops = {
	//.s_power = ds5_mux_set_power,
	.log_status = v4l2_ctrl_subdev_log_status,
};

static const struct v4l2_subdev_video_ops ds5_mux_video_ops = {
	.g_frame_interval	= ds5_mux_g_frame_interval,
	.s_frame_interval	= ds5_mux_s_frame_interval,
	.s_stream		= ds5_mux_s_stream,
};

static const struct v4l2_subdev_ops ds5_mux_subdev_ops = {
	.core = &ds5_mux_core_ops,
	.pad = &ds5_mux_pad_ops,
	.video = &ds5_mux_video_ops,
};

static int ds5_mux_registered(struct v4l2_subdev *sd)
{
	struct ds5 *state = v4l2_get_subdevdata(sd);
	int ret = ds5_sensor_register(state, &state->depth.sensor);
	if (ret < 0)
		return ret;

	ret = ds5_sensor_register(state, &state->motion_t.sensor);
	if (ret < 0)
		goto e_depth;

	ret = ds5_sensor_register(state, &state->rgb.sensor);
	if (ret < 0)
		goto e_rgb;

	ret = ds5_sensor_register(state, &state->imu.sensor);
	if (ret < 0)
		goto e_imu;

	return 0;

e_imu:
	v4l2_device_unregister_subdev(&state->rgb.sensor.sd);

e_rgb:
	v4l2_device_unregister_subdev(&state->motion_t.sensor.sd);

e_depth:
	v4l2_device_unregister_subdev(&state->depth.sensor.sd);

	return ret;
}

static void ds5_mux_unregistered(struct v4l2_subdev *sd)
{
	struct ds5 *state = v4l2_get_subdevdata(sd);
	ds5_sensor_remove(&state->imu.sensor);
	ds5_sensor_remove(&state->rgb.sensor);
	ds5_sensor_remove(&state->motion_t.sensor);
	ds5_sensor_remove(&state->depth.sensor);
}

static const struct v4l2_subdev_internal_ops ds5_mux_internal_ops = {
	.open = ds5_mux_open,
	.close = ds5_mux_close,
	.registered = ds5_mux_registered,
	.unregistered = ds5_mux_unregistered,
};

static int ds5_mux_register(struct i2c_client *c, struct ds5 *state)
{
	return v4l2_async_register_subdev(&state->mux.sd.subdev);
}

static int ds5_hw_init(struct i2c_client *c, struct ds5 *state)
{
	struct v4l2_subdev *sd = &state->mux.sd.subdev;
	u16 mipi_status, n_lanes, phy, drate_min, drate_max;
	int ret = ds5_read(state, DS5_MIPI_SUPPORT_LINES, &n_lanes);
	if (!ret)
		ret = ds5_read(state, DS5_MIPI_SUPPORT_PHY, &phy);

	if (!ret)
		ret = ds5_read(state, DS5_MIPI_DATARATE_MIN, &drate_min);

	if (!ret)
		ret = ds5_read(state, DS5_MIPI_DATARATE_MAX, &drate_max);

	if (!ret)
		dev_info(sd->dev, "%s(): %d: %u lanes, phy %x, data rate %u-%u\n",
			 __func__, __LINE__, n_lanes, phy, drate_min, drate_max);

#ifdef CONFIG_TEGRA_CAMERA_PLATFORM
	n_lanes = state->mux.sd.numlanes;
#else
	n_lanes = 2;
#error get lane information
#endif

	ret = ds5_write(state, DS5_MIPI_LANE_NUMS, n_lanes - 1);
	if (!ret)
		ret = ds5_write(state, DS5_MIPI_LANE_DATARATE, MIPI_LANE_RATE);

	ret = ds5_read(state, DS5_MIPI_CONF_STATUS, &mipi_status);

	dev_dbg(sd->dev, "%s(): %d phandle %x node %s status %x\n", __func__, __LINE__,
		 c->dev.of_node->phandle, c->dev.of_node->full_name, mipi_status);

	return ret;
}

static int ds5_mux_init(struct i2c_client *c, struct ds5 *state)
{
	struct v4l2_subdev *sd = &state->mux.sd.subdev;
	struct media_entity *entity = &state->mux.sd.subdev.entity;
	struct media_pad *pads = state->mux.pads, *pad;
	unsigned int i;
	int ret;

	v4l2_i2c_subdev_init(sd, c, &ds5_mux_subdev_ops);
	// See tegracam_v4l2.c tegracam_v4l2subdev_register()
	// Set owner to NULL so we can unload the driver module
	sd->owner = NULL;
	sd->internal_ops = &ds5_mux_internal_ops;
	v4l2_set_subdevdata(sd, state);
	snprintf(sd->name, sizeof(sd->name), "DS5 mux %d-%04x",
		 i2c_adapter_id(c->adapter), c->addr);

	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	entity->obj_type = MEDIA_ENTITY_TYPE_V4L2_SUBDEV;
	entity->function = MEDIA_ENT_F_CAM_SENSOR;

	pads[0].flags = MEDIA_PAD_FL_SOURCE;
	for (i = 1, pad = pads + 1; i < ARRAY_SIZE(state->mux.pads); i++, pad++)
		pad->flags = MEDIA_PAD_FL_SINK;

	ret = media_entity_pads_init(entity, ARRAY_SIZE(state->mux.pads), pads);
	if (ret < 0)
		return ret;

	ret = ds5_ctrl_init(state);
	if (ret < 0)
		goto e_entity;

	if (state->is_depth)
		state->mux.last_set = &state->depth.sensor;
	else if (state->is_rgb)
		state->mux.last_set = &state->rgb.sensor;
	else if (state->is_y8)
		state->mux.last_set = &state->motion_t.sensor;
	else
		state->mux.last_set = &state->imu.sensor;

#ifdef CONFIG_TEGRA_CAMERA_PLATFORM
	state->mux.sd.dev = &c->dev;
	ret = camera_common_initialize(&state->mux.sd, "d4xx");
	if (ret) {
		dev_err(&c->dev, "Failed to initialize d4xx.\n");
		goto e_ctrl;
	}
#endif

	return 0;

#ifdef CONFIG_TEGRA_CAMERA_PLATFORM
e_ctrl:
	v4l2_ctrl_handler_free(sd->ctrl_handler);
#endif
e_entity:
	media_entity_cleanup(entity);

	return ret;
}

#define USE_Y

static int ds5_fixed_configuration(struct i2c_client *client, struct ds5 *state)
{
	struct ds5_sensor *sensor;
	u16 cfg0 = 0, cfg0_md = 0, cfg1 = 0, cfg1_md = 0;
	u16 dw = 0, dh = 0, yw = 0, yh = 0, dev_type = 0;
	int ret;

	ret = ds5_read(state, DS5_DEPTH_STREAM_DT, &cfg0);
	if (!ret)
		ret = ds5_read(state, DS5_DEPTH_STREAM_MD, &cfg0_md);
	if (!ret)
		ret = ds5_read(state, DS5_DEPTH_RES_WIDTH, &dw);
	if (!ret)
		ret = ds5_read(state, DS5_DEPTH_RES_HEIGHT, &dh);
	if (!ret)
		ret = ds5_read(state, DS5_IR_STREAM_DT, &cfg1);
	if (!ret)
		ret = ds5_read(state, DS5_IR_STREAM_MD, &cfg1_md);
	if (!ret)
		ret = ds5_read(state, DS5_IR_RES_WIDTH, &yw);
	if (!ret)
		ret = ds5_read(state, DS5_IR_RES_HEIGHT, &yh);
	if (!ret)
		ret = ds5_read(state, DS5_DEVICE_TYPE, &dev_type);
	if (ret < 0)
		return ret;

	dev_dbg(&client->dev, "%s(): cfg0 %x %ux%u cfg0_md %x %ux%u\n", __func__,
		 cfg0, dw, dh, cfg0_md, yw, yh);

	dev_dbg(&client->dev, "%s(): cfg1 %x %ux%u cfg1_md %x %ux%u\n", __func__,
		 cfg1, dw, dh, cfg1_md, yw, yh);

	sensor = &state->depth.sensor;
	switch (dev_type) {
	case DS5_DEVICE_TYPE_D43X:
	case DS5_DEVICE_TYPE_D45X:
		sensor->formats = ds5_depth_formats_d43x;
		break;
	case DS5_DEVICE_TYPE_D46X:
		sensor->formats = ds5_depth_formats_d46x;
		break;
	default:
		sensor->formats = ds5_depth_formats_d46x;
	}
	sensor->n_formats = 1;
	sensor->mux_pad = DS5_MUX_PAD_DEPTH;

	sensor = &state->motion_t.sensor;
	sensor->formats = state->variant->formats;
	sensor->n_formats = state->variant->n_formats;
	sensor->mux_pad = DS5_MUX_PAD_MOTION_T;
	switch (dev_type) {
	case DS5_DEVICE_TYPE_D45X:
	case DS5_DEVICE_TYPE_D43X: {
		unsigned int *calib_resolutions_size =
				(unsigned int *)&(
						sensor->formats[ARRAY_SIZE(ds5_y_formats_ds5u)-1]
										.n_resolutions);
		const struct ds5_resolution** calib_resolutions =
				(const struct ds5_resolution**)
				&(sensor->formats[ARRAY_SIZE(ds5_y_formats_ds5u)-1]
								.resolutions);
		*calib_resolutions_size = ARRAY_SIZE(d43x_calibration_sizes),
				*calib_resolutions = d43x_calibration_sizes;
		break;
		}
	case DS5_DEVICE_TYPE_D46X: {
		dev_dbg(&client->dev, "%s(): DS5_DEVICE_TYPE_D46X for calib\n",
				__func__);
		break;
		}
	}

	sensor = &state->rgb.sensor;
	switch (dev_type) {
	case DS5_DEVICE_TYPE_D43X:
	case DS5_DEVICE_TYPE_D46X:
		sensor->formats = &ds5_onsemi_rgb_format;
		sensor->n_formats = DS5_ONSEMI_RGB_N_FORMATS;
		break;
	case DS5_DEVICE_TYPE_D45X:
		sensor->formats = &ds5_rlt_rgb_format;
		sensor->n_formats = DS5_RLT_RGB_N_FORMATS;
		break;
	default:
		sensor->formats = &ds5_onsemi_rgb_format;
		sensor->n_formats = DS5_ONSEMI_RGB_N_FORMATS;
	}
	sensor->mux_pad = DS5_MUX_PAD_RGB;

	sensor = &state->imu.sensor;
	sensor->formats = ds5_imu_formats;
	sensor->n_formats = 1;
	sensor->mux_pad = DS5_MUX_PAD_IMU;

	/* Development: set a configuration during probing */
	if ((cfg0 & 0xff00) == 0x1800) {
		/* MIPI CSI-2 YUV420 isn't supported by V4L, reconfigure to Y8 */
		struct v4l2_subdev_format fmt = {
			.which = V4L2_SUBDEV_FORMAT_ACTIVE,
			.pad = 0,
			/* Use template to fill in .field, .colorspace etc. */
			.format = ds5_mbus_framefmt_template,
		};

//#undef USE_Y
#ifdef USE_Y
		/* Override .width, .height, .code */
		fmt.format.width = yw;
		fmt.format.height = yh;
		fmt.format.code = MEDIA_BUS_FMT_UYVY8_2X8;

		state->mux.sd.mode_prop_idx = 0;
		state->motion_t.sensor.streaming = true;
		state->depth.sensor.streaming = true;
		ret = __ds5_sensor_set_fmt(state, &state->motion_t.sensor, NULL, &fmt);
#else
		fmt.format.width = dw;
		fmt.format.height = dh;
		fmt.format.code = MEDIA_BUS_FMT_UYVY8_1X16;

		state->mux.sd.mode_prop_idx = 1;
		state->motion_t.sensor.streaming = false;
		state->depth.sensor.streaming = true;
		ret = __ds5_sensor_set_fmt(state, &state->depth.sensor, NULL, &fmt);
#endif
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int ds5_parse_cam(struct i2c_client *client, struct ds5 *state)
{
	int ret = ds5_fixed_configuration(client, state);
	if (ret < 0)
		return ret;

	ds5_sensor_format_init(&state->depth.sensor);
	ds5_sensor_format_init(&state->motion_t.sensor);
	ds5_sensor_format_init(&state->rgb.sensor);
	ds5_sensor_format_init(&state->imu.sensor);

	return 0;
}

static void ds5_mux_remove(struct ds5 *state)
{
#ifdef CONFIG_TEGRA_CAMERA_PLATFORM
	camera_common_cleanup(&state->mux.sd);
#endif
	v4l2_async_unregister_subdev(&state->mux.sd.subdev);
	v4l2_ctrl_handler_free(state->mux.sd.subdev.ctrl_handler);
	media_entity_cleanup(&state->mux.sd.subdev.entity);
}

static const struct regmap_config ds5_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
	.reg_format_endian = REGMAP_ENDIAN_NATIVE,
	.val_format_endian = REGMAP_ENDIAN_NATIVE,
};

static int ds5_dfu_wait_for_status(struct ds5 *state)
{
	int i, ret = 0;
	u16 status;

	for (i = 0; i < DS5_START_MAX_COUNT; i++)
	{
		ds5_read(state, 0x5000, &status);
		if (status == 0x0001 || status == 0x0002)
		{
			dev_err(&state->client->dev,
					"%s(): dfu failed status(0x%4x)\n",
					__func__, status);
			ret = -EREMOTEIO;
			break;
		}
		if (!status)
			break;
		msleep_range(DS5_START_POLL_TIME);
		}

	return ret;
};

static int ds5_dfu_switch_to_dfu(struct ds5 *state)
{
	int ret;
	int i = DS5_START_MAX_COUNT;
	u16 status;

	ds5_raw_write_with_check(state, 0x4900,
			&cmd_switch_to_dfu, sizeof(cmd_switch_to_dfu));
	ds5_write_with_check(state, 0x490c, 0x01); /* execute cmd */
	/*Wait for DFU fw to boot*/
	do {
		msleep_range(DS5_START_POLL_TIME*10);
		ret = ds5_read(state, 0x5000, &status);
	} while (ret && i-- );
	return ret;
};

static int ds5_dfu_wait_for_get_dfu_status(struct ds5 *state,
		enum dfu_fw_state exp_state)
{
	int ret = 0;
	u16 status,dfu_state_len = 0x0000;
	unsigned char dfu_asw_buf[DFU_WAIT_RET_LEN];
	unsigned int dfu_wr_wait_msec = 0;
	do {
		ds5_write_with_check(state, 0x5008, 0x0003); // Get Write state
		do {
			ds5_read_with_check(state, 0x5000, &status);
			if (status == 0x0001) {
				dev_err(&state->client->dev,
						"%s(): Write status error I2C_STATUS_ERROR(1)\n",
						__func__);
				return -EINVAL;
			} else
				if (status == 0x0002 && dfu_wr_wait_msec)
					msleep_range(dfu_wr_wait_msec);

		} while (status);

		ds5_read_with_check(state, 0x5004, &dfu_state_len);
		if (dfu_state_len != DFU_WAIT_RET_LEN) {
			dev_err(&state->client->dev,
					"%s(): Wrong answer len (%d)\n", __func__, dfu_state_len);
			return -EINVAL;
		}
		ds5_raw_read_with_check(state, 0x4e00, &dfu_asw_buf, DFU_WAIT_RET_LEN);
		if (dfu_asw_buf[0]) {
			dev_err(&state->client->dev,
					"%s(): Wrong dfu_status (%d)\n", __func__, dfu_asw_buf[0]);
			return -EINVAL;
		}
		dfu_wr_wait_msec = (((unsigned int)dfu_asw_buf[3]) << 16)
						| (((unsigned int)dfu_asw_buf[2]) << 8)
						| dfu_asw_buf[1];
	} while (dfu_asw_buf[4] == dfuDNBUSY && exp_state == dfuDNLOAD_IDLE);


	if (dfu_asw_buf[4] != exp_state) {
		dev_notice(&state->client->dev,
				"%s(): Wrong dfu_state (%d) while expected(%d)\n",
				__func__, dfu_asw_buf[4], exp_state);
		ret = -EINVAL;
	}
	return ret;
};

static int ds5_dfu_get_dev_info(struct ds5 *state, struct __fw_status *buf)
{
	int ret;
	u16 len = 0;

	ret = ds5_write(state, 0x5008, 0x0002); //Upload DFU cmd
	if (!ret)
		ret = ds5_dfu_wait_for_status(state);
	if (!ret)
		ds5_read_with_check(state, 0x5004, &len);
	/*Sanity check*/
	if (len == sizeof(struct __fw_status)) {
		ds5_raw_read_with_check(state, 0x4e00, buf, len);
	} else {
		dev_err(&state->client->dev,
				"%s(): Wrong state size (%d)\n",
				__func__, len);
		ret = -EINVAL;
	}
	return ret;
};

static int ds5_dfu_detach(struct ds5 *state)
{
	int ret;
	struct __fw_status buf = {0};

	ds5_write_with_check(state, 0x500c, 0x00);
	ret = ds5_dfu_wait_for_get_dfu_status(state, dfuIDLE);
	if (!ret)
		ret = ds5_dfu_get_dev_info(state, &buf);
	dev_notice(&state->client->dev, "%s():DFU ver (0x%x) recieved\n",
			__func__, buf.DFU_version);
	dev_notice(&state->client->dev, "%s():FW last version (0x%x) recieved\n",
			__func__, buf.FW_lastVersion);
	dev_notice(&state->client->dev, "%s():FW status (%s)\n",
			__func__, buf.DFU_isLocked ? "locked" : "unlocked");
	return ret;
};

/* When a process reads from our device, this gets called. */
static ssize_t ds5_dfu_device_read(struct file *flip,
		char __user *buffer, size_t len, loff_t *offset)
{
	struct ds5 *state = flip->private_data;
	u16 fw_ver;
	char msg[20];
	int ret = 0;

	if (mutex_lock_interruptible(&state->lock))
		return -ERESTARTSYS;
	ret = ds5_read(state, DS5_FW_VERSION, &fw_ver);
	if (ret < 0)
		goto e_dfu_read_failed;
	snprintf(msg, sizeof(msg) ,"DFU info: \tver: (0x%x)\n", fw_ver);
	if (copy_to_user(buffer, msg, strlen(msg)))
		ret = -EFAULT;
	else {
		state->dfu_dev.msg_write_once = ~state->dfu_dev.msg_write_once;
		ret = strlen(msg) & state->dfu_dev.msg_write_once;
	}

e_dfu_read_failed:
	mutex_unlock(&state->lock);
	return ret;
};

static ssize_t ds5_dfu_device_write(struct file *flip,
		const char __user *buffer, size_t len, loff_t *offset)
{
	struct ds5 *state = flip->private_data;
	int ret = 0;

	if (mutex_lock_interruptible(&state->lock))
		return -ERESTARTSYS;
	switch (state->dfu_dev.dfu_state_flag) {

	case DS5_DFU_OPEN:
		ret = ds5_dfu_switch_to_dfu(state);
		if (ret < 0) {
			dev_err(&state->client->dev, "%s(): Switch to dfu failed (%d)\n",
					__func__, ret);
			goto dfu_write_error;
		}
		/*no break - procceed to recovery*/
	case DS5_DFU_RECOVERY:
		ret = ds5_dfu_detach(state);
		if (ret < 0) {
			dev_err(&state->client->dev, "%s(): Detach failed (%d)\n",
					__func__, ret);
			goto dfu_write_error;
		}
		state->dfu_dev.dfu_state_flag = DS5_DFU_IN_PROGRESS;
		state->dfu_dev.init_v4l_f = 1;

		/*no break - procceed to download*/
	case DS5_DFU_IN_PROGRESS: {
		unsigned int dfu_full_blocks = len / DFU_BLOCK_SIZE;
		unsigned int dfu_part_blocks = len % DFU_BLOCK_SIZE;

		while (dfu_full_blocks--) {
			if(copy_from_user(state->dfu_dev.dfu_msg, buffer, DFU_BLOCK_SIZE)) {
				ret = -EFAULT;
				goto dfu_write_error;
			}
			ret = ds5_raw_write(state, 0x4a00,
					state->dfu_dev.dfu_msg, DFU_BLOCK_SIZE);
			if (ret < 0)
				goto dfu_write_error;
			ret = ds5_dfu_wait_for_get_dfu_status (state, dfuDNLOAD_IDLE);
			if (ret < 0)
				goto dfu_write_error;
			buffer += DFU_BLOCK_SIZE;
		}
		if(copy_from_user(state->dfu_dev.dfu_msg , buffer, dfu_part_blocks)) {
				ret = -EFAULT;
				goto dfu_write_error;
		}
		if (dfu_part_blocks) {
			ret = ds5_raw_write(state, 0x4a00,
					state->dfu_dev.dfu_msg, dfu_part_blocks);
			if (!ret) {
				ret = ds5_dfu_wait_for_get_dfu_status (state, dfuDNLOAD_IDLE);
			}
			if (!ret)
				ret = ds5_write(state, 0x4a04, 0x00); /*Download complete */
			if (!ret)
				ret = ds5_dfu_wait_for_get_dfu_status (state, dfuMANIFEST);
			if (ret < 0)
				goto dfu_write_error;
			state->dfu_dev.dfu_state_flag = DS5_DFU_DONE;
		}
		dev_notice(&state->client->dev, "%s(): DFU block (%d) bytes written\n",
				__func__, (int)len);
		break;
	}
	default:
		dev_err(&state->client->dev, "%s(): Wrong state (%d)\n",
				__func__, state->dfu_dev.dfu_state_flag);
		ret = -EINVAL;
		goto dfu_write_error;

	};
	mutex_unlock(&state->lock);
	return len;

dfu_write_error:
	//TODO: Reset device here
	state->dfu_dev.dfu_state_flag = DS5_DFU_ERROR;
	mutex_unlock(&state->lock);
	return ret;
};

static int ds5_dfu_device_open(struct inode *inode, struct file *file)
{
	struct ds5 *state = container_of(inode->i_cdev, struct ds5,
			dfu_dev.ds5_cdev);
	struct i2c_adapter *parent = i2c_parent_is_i2c_adapter(
			state->client->adapter);

	if (state->dfu_dev.device_open_count)
		return -EBUSY;
	state->dfu_dev.device_open_count++;
	if (state->dfu_dev.dfu_state_flag != DS5_DFU_RECOVERY)
		state->dfu_dev.dfu_state_flag = DS5_DFU_OPEN;
	state->dfu_dev.dfu_msg = devm_kzalloc(&state->client->dev,
			DFU_BLOCK_SIZE, GFP_KERNEL);
	if (!state->dfu_dev.dfu_msg)
		return -ENOMEM;

	file->private_data = state;

	/* get i2c controller and set dfu bus clock rate */
	while (parent && i2c_parent_is_i2c_adapter(parent))
		parent = i2c_parent_is_i2c_adapter(state->client->adapter);

	if (!parent)
		return 0;

	dev_dbg(&state->client->dev, "%s(): i2c-%d bus_clk = %d, set %d\n",
			__func__,
			i2c_adapter_id(parent),
			i2c_get_adapter_bus_clk_rate(parent),
			DFU_I2C_BUS_CLK_RATE);

	state->dfu_dev.bus_clk_rate = i2c_get_adapter_bus_clk_rate(parent);
	i2c_set_adapter_bus_clk_rate(parent, DFU_I2C_BUS_CLK_RATE);
	return 0;
};

static int ds5_v4l_init(struct i2c_client *c, struct ds5 *state)
{
	int ret;

	ret = ds5_parse_cam(c, state);
	if (ret < 0)
		return ret;

	ret = ds5_depth_init(c, state);
	if (ret < 0)
		return ret;

	ret = ds5_motion_t_init(c, state);
	if (ret < 0)
		goto e_depth;

	ret = ds5_rgb_init(c, state);
	if (ret < 0)
		goto e_motion_t;

	ret = ds5_imu_init(c, state);
	if (ret < 0)
		goto e_rgb;

	ret = ds5_mux_init(c, state);
	if (ret < 0)
		goto e_imu;

	ret = ds5_hw_init(c, state);
	if (ret < 0)
		goto e_mux;

	ret = ds5_mux_register(c, state);
	if (ret < 0)
		goto e_mux;

	return 0;
e_mux:
#ifdef CONFIG_TEGRA_CAMERA_PLATFORM
	camera_common_cleanup(&state->mux.sd);
#endif
	v4l2_ctrl_handler_free(state->mux.sd.subdev.ctrl_handler);
	media_entity_cleanup(&state->mux.sd.subdev.entity);
e_imu:
	media_entity_cleanup(&state->imu.sensor.sd.entity);
e_rgb:
	media_entity_cleanup(&state->rgb.sensor.sd.entity);
e_motion_t:
	media_entity_cleanup(&state->motion_t.sensor.sd.entity);
e_depth:
	media_entity_cleanup(&state->depth.sensor.sd.entity);
	return ret;
}

static int ds5_dfu_device_release(struct inode *inode, struct file *file)
{
	struct ds5 *state = container_of(inode->i_cdev, struct ds5, dfu_dev.ds5_cdev);
	struct i2c_adapter *parent = i2c_parent_is_i2c_adapter(
			state->client->adapter);

	state->dfu_dev.device_open_count--;
	if (state->dfu_dev.dfu_state_flag != DS5_DFU_RECOVERY)
		state->dfu_dev.dfu_state_flag = DS5_DFU_IDLE;
	if (state->dfu_dev.dfu_state_flag == DS5_DFU_DONE
			&& state->dfu_dev.init_v4l_f)
		ds5_v4l_init(state->client, state);
	state->dfu_dev.init_v4l_f = 0;
	if (state->dfu_dev.dfu_msg)
		devm_kfree(&state->client->dev, state->dfu_dev.dfu_msg);
	state->dfu_dev.dfu_msg = NULL;

	/* get i2c controller and restore bus clock rate */
	while (parent && i2c_parent_is_i2c_adapter(parent))
		parent = i2c_parent_is_i2c_adapter(state->client->adapter);
	if (!parent)
		return 0;
	dev_dbg(&state->client->dev, "%s(): i2c-%d bus_clk %d, restore to %d\n",
			__func__, i2c_adapter_id(parent),
			i2c_get_adapter_bus_clk_rate(parent),
			state->dfu_dev.bus_clk_rate);

	i2c_set_adapter_bus_clk_rate(parent, state->dfu_dev.bus_clk_rate);
	return 0;
};

static const struct file_operations ds5_device_file_ops = {
	.owner = THIS_MODULE,
	.read = &ds5_dfu_device_read,
	.write = &ds5_dfu_device_write,
	.open = &ds5_dfu_device_open,
	.release = &ds5_dfu_device_release
};

struct class* g_ds5_class;
atomic_t primary_chardev=ATOMIC_INIT(0);

static int ds5_chrdev_init(struct i2c_client *c, struct ds5 *state)
{
	struct cdev *ds5_cdev = &state->dfu_dev.ds5_cdev;
	struct class* *ds5_class = &state->dfu_dev.ds5_class;
	struct device *chr_dev;
	char dev_name[sizeof(DS5_DRIVER_NAME_DFU) +5];
	dev_t *dev_num = &c->dev.devt;
	int ret;

	dev_dbg(&c->dev, "%s()\n", __func__);
	/* Request the kernel for N_MINOR devices */
	ret = alloc_chrdev_region(dev_num, 0, 1, DS5_DRIVER_NAME_DFU);
	if (ret < 0)
		return ret;

	if (!atomic_cmpxchg (&primary_chardev, 0, MAJOR(*dev_num))) {
		dev_dbg(&c->dev, "%s(): <Major, Minor>: <%d, %d>\n",
				__func__, MAJOR(*dev_num), MINOR(*dev_num));
		/* Create a class : appears at /sys/class */
		*ds5_class = class_create(THIS_MODULE, DS5_DRIVER_NAME_CLASS);
		if (IS_ERR(*ds5_class)) {
			dev_err(&c->dev, "Could not create class device\n");
			unregister_chrdev_region(0, 1);
			ret = PTR_ERR(*ds5_class);
			return ret;
		}
		g_ds5_class = *ds5_class;
	} else
		*ds5_class = g_ds5_class;
	/* Associate the cdev with a set of file_operations */
	cdev_init(ds5_cdev, &ds5_device_file_ops);
	/* Build up the current device number. To be used further */
	*dev_num = MKDEV(MAJOR(*dev_num), MINOR(*dev_num));
	/* Create a device node for this device. */
	snprintf (dev_name, sizeof(dev_name), "%s%d",
			DS5_DRIVER_NAME_DFU, MAJOR(*dev_num));
	chr_dev = device_create(*ds5_class, NULL, *dev_num, NULL, dev_name);
	if (IS_ERR(chr_dev)) {
		ret = PTR_ERR(chr_dev);
		dev_err(&c->dev,"Could not create device\n");
		class_destroy(*ds5_class);
		unregister_chrdev_region(0, 1);
		return ret;
	}
	cdev_add(ds5_cdev, *dev_num, 1);
	return 0;
};

static int ds5_chrdev_remove(struct ds5 *state)
{
	struct class* *ds5_class = &state->dfu_dev.ds5_class;
	dev_t *dev_num = &state->client->dev.devt;

	dev_dbg(&state->client->dev, "%s()\n", __func__);
	unregister_chrdev_region(*dev_num, 1);
	device_destroy(*ds5_class, *dev_num);
	if (atomic_cmpxchg (&primary_chardev, MAJOR(*dev_num), 0) == MAJOR(*dev_num))
		class_destroy(*ds5_class);
	return 0;
}

/* SYSFS attributes */

static ssize_t ds5_fw_ver_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *c = to_i2c_client(dev);
	struct ds5 *state = container_of(i2c_get_clientdata(c),
			struct ds5, mux.sd.subdev);

	ds5_read(state, DS5_FW_VERSION, &state->fw_version);
	ds5_read(state, DS5_FW_BUILD, &state->fw_build);

	return snprintf(buf, PAGE_SIZE, "D4XX Sensor: %s, Version: %d.%d.%d.%d\n",
			ds5_get_sensor_name(state),
			(state->fw_version >> 8) & 0xff, state->fw_version & 0xff,
			(state->fw_build >> 8) & 0xff, state->fw_build & 0xff);
}

static DEVICE_ATTR_RO(ds5_fw_ver);

/* Derive 'device_attribute' structure for a read register's attribute */
struct dev_ds5_reg_attribute {
	struct device_attribute attr;
	u16 reg;	// register
	u8 valid;	// validity of above data
};

/** Read DS5 register.
 * ds5_read_reg_show will actually read register from ds5 while
 * ds5_read_reg_store will store register to read
 * Example:
 * echo -n "0xc03c" >ds5_read_reg
 * Read register result:
 * cat ds5_read_reg
 * Expected:
 * reg:0xc93c, result:0x11
 */
static ssize_t ds5_read_reg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u16 rbuf;
	int n;
	struct i2c_client *c = to_i2c_client(dev);
	struct ds5 *state = container_of(i2c_get_clientdata(c),
			struct ds5, mux.sd.subdev);
	struct dev_ds5_reg_attribute *ds5_rw_attr = container_of(attr,
			struct dev_ds5_reg_attribute, attr);
	if (ds5_rw_attr->valid != 1)
		return -EINVAL;
	ds5_read(state, ds5_rw_attr->reg, &rbuf);

	n = snprintf(buf, PAGE_SIZE, "register:0x%4x, value:0x%02x\n",
			ds5_rw_attr->reg, rbuf);

	return n;
}

/** Read DS5 register - Store reg to attr struct pointer
 * ds5_read_reg_show will actually read register from ds5 while
 * ds5_read_reg_store will store module, offset and length
 */
static ssize_t ds5_read_reg_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct dev_ds5_reg_attribute *ds5_rw_attr = container_of(attr,
			struct dev_ds5_reg_attribute, attr);
	int rc = -1;
	u32 reg;
	ds5_rw_attr->valid = 0;
	/* Decode input */
	rc = sscanf(buf, "0x%04x", &reg);
	if (rc != 1)
		return -EINVAL;
	ds5_rw_attr->reg = reg;
	ds5_rw_attr->valid = 1;
	return count;
}

#define DS5_RW_REG_ATTR(_name) \
		struct dev_ds5_reg_attribute dev_attr_##_name = { \
				__ATTR(_name, S_IRUGO | S_IWUSR, \
						ds5_read_reg_show, ds5_read_reg_store), \
						0, 0 }

static DS5_RW_REG_ATTR(ds5_read_reg);

static ssize_t ds5_write_reg_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *c = to_i2c_client(dev);
	struct ds5 *state = container_of(i2c_get_clientdata(c),
			struct ds5, mux.sd.subdev);

	int rc = -1;
	u32 reg, w_val = 0;
	u16 val = -1;
	/* Decode input */
	rc = sscanf(buf, "0x%04x 0x%04x", &reg, &w_val);
	if (rc != 2)
		return -EINVAL;
	val = w_val & 0xffff;
	mutex_lock(&state->lock);
	ds5_write(state, reg, val);
	mutex_unlock(&state->lock);
	return count;
}

static DEVICE_ATTR_WO(ds5_write_reg);

static struct attribute *ds5_attributes[] = {
		&dev_attr_ds5_fw_ver.attr,
		&dev_attr_ds5_read_reg.attr.attr,
		&dev_attr_ds5_write_reg.attr,
		NULL
};

static const struct attribute_group ds5_attr_group = {
	.attrs = ds5_attributes,
};

static int ds5_probe(struct i2c_client *c, const struct i2c_device_id *id)
{
	struct ds5 *state = devm_kzalloc(&c->dev, sizeof(*state), GFP_KERNEL);
	u16 rec_state;
	int ret, err = 0;
	const char *str;

	if (!state)
		return -ENOMEM;

	mutex_init(&state->lock);


	state->client = c;
	dev_warn(&c->dev, "Probing new driver for D45x\n");
	state->variant = ds5_variants + id->driver_data;

	state->vcc = devm_regulator_get(&c->dev, "vcc");
	if (IS_ERR(state->vcc)) {
		ret = PTR_ERR(state->vcc);
		dev_warn(&c->dev, "failed %d to get vcc regulator\n", ret);
		return ret;
	}

	if (state->vcc) {
		ret = regulator_enable(state->vcc);
		if (ret < 0) {
			dev_warn(&c->dev, "failed %d to enable the vcc regulator\n", ret);
			return ret;
		}
	}
	state->regmap = devm_regmap_init_i2c(c, &ds5_regmap_config);
	if (IS_ERR(state->regmap)) {
		ret = PTR_ERR(state->regmap);
		dev_err(&c->dev, "regmap init failed: %d\n", ret);
		goto e_regulator;
	}
	ret = ds5_chrdev_init(c, state);
	if (ret < 0)
		goto e_regulator;
	ret = ds5_read(state, 0x5020, &rec_state);
	if (ret < 0) {
		dev_err(&c->dev, "%s(): cannot communicate with D4XX: %d\n",
				__func__, ret);
		goto e_chardev;
	}

	if (rec_state == 0x201) {
		dev_info(&c->dev, "%s(): D4XX recovery state\n", __func__);
		state->dfu_dev.dfu_state_flag = DS5_DFU_RECOVERY;
		return 0;
	}

	state->is_depth = 0;
	state->is_y8 = 0;
	state->is_rgb = 0;
	state->is_imu = 0;

	err = of_property_read_string(c->dev.of_node, "cam-type",
			&str);
	if (!err && !strncmp(str, "Depth", strlen("Depth")))
		state->is_depth = 1;
	if (!err && !strncmp(str, "Y8", strlen("Y8")))
		state->is_y8 = 1;
	if (!err && !strncmp(str, "RGB", strlen("RGB")))
		state->is_rgb = 1;
	if (!err && !strncmp(str, "IMU", strlen("IMU")))
		state->is_imu = 1;

	ds5_read_with_check(state, DS5_FW_VERSION, &state->fw_version);
	ds5_read_with_check(state, DS5_FW_BUILD, &state->fw_build);

	dev_info(&c->dev, "D4XX Sensor: %s, firmware build: %d.%d.%d.%d\n",
			ds5_get_sensor_name(state),
			(state->fw_version >> 8) & 0xff, state->fw_version & 0xff,
			(state->fw_build >> 8) & 0xff, state->fw_build & 0xff);

	ret = ds5_v4l_init(c, state);
	if (ret < 0)
		goto e_chardev;
	/* DONOT!! Override I2C drvdata */
//	i2c_set_clientdata(c, state);

/*	regulators? clocks?
	devm_regulator_bulk_get(&c->dev, DS5_N_SUPPLIES, state->supplies);
	state->clock = devm_clk_get(&c->dev, DS5_CLK_NAME);
	if (IS_ERR(state->clock)) {
		ret = -EPROBE_DEFER;
		goto err;
	}
*/
	/* Custom sysfs attributes */
	/* create the sysfs file group */
	err = sysfs_create_group(&state->client->dev.kobj, &ds5_attr_group);

	return 0;

e_chardev:
	ds5_chrdev_remove(state);
e_regulator:
	if (state->vcc)
		regulator_disable(state->vcc);
	return ret;
}

static int ds5_remove(struct i2c_client *c)
{
	struct ds5 *state = container_of(i2c_get_clientdata(c), struct ds5, mux.sd.subdev);

	dev_info(&c->dev, "D4XX remove %s\n",
			ds5_get_sensor_name(state));
	if (state->vcc)
		regulator_disable(state->vcc);
//	gpio_free(state->pwdn_gpio);
	ds5_chrdev_remove(state);
	if (state->dfu_dev.dfu_state_flag != DS5_DFU_RECOVERY) {
		sysfs_remove_group(&c->dev.kobj, &ds5_attr_group);
		ds5_mux_remove(state);
	}
	return 0;
}

static const struct i2c_device_id ds5_id[] = {
	{ DS5_DRIVER_NAME, DS5_DS5U },
	{ DS5_DRIVER_NAME_ASR, DS5_ASR },
	{ DS5_DRIVER_NAME_AWG, DS5_AWG },
	{ },
};
MODULE_DEVICE_TABLE(i2c, ds5_id);

static struct i2c_driver ds5_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = DS5_DRIVER_NAME
	},
	.probe		= ds5_probe,
	.remove		= ds5_remove,
	.id_table	= ds5_id,
};

module_i2c_driver(ds5_i2c_driver);

MODULE_DESCRIPTION("Intel RealSense D4XX Camera Driver");
MODULE_AUTHOR( "Guennadi Liakhovetski <guennadi.liakhovetski@intel.com>,\n\
				Nael Masalha <nael.masalha@intel.com>,\n\
				Alexander Gantman <alexander.gantman@intel.com>,\n\
				Emil Jahshan <emil.jahshan@intel.com>,\n\
				Xin Zhang <xin.x.zhang@intel.com>,\n\
				Qingwu Zhang <qingwu.zhang@intel.com>,\n\
				Evgeni Raikhel <evgeni.raikhel@intel.com>,\n\
				Shikun Ding <shikun.ding@intel.com>");
MODULE_AUTHOR("Dmitry Perchanov <dmitry.perchanov@intel.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.1.13.b");
