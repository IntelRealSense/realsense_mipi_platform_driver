

#include <stdio.h>
#include <stdint.h>
/* librealsense/src/platform/uvc-device.h */
#pragma pack( push, 1 )
struct uvc_header
{
    uint8_t length;  // UVC Metadata total length is
                     // limited by design to 255 bytes
    uint8_t info;
    uint32_t timestamp;
    uint8_t source_clock[6];
};

struct uvc_header_mipi
{
    struct uvc_header header;
    uint32_t frame_counter;
};
#pragma pack( pop )

const uint8_t uvc_header_mipi_size = sizeof(struct uvc_header_mipi );

/* librealsense/src/metadata.h */
/**\brief md_mode - enumerates the types of metadata modes(structs) supported */
enum md_type
{
    META_DATA_INTEL_DEPTH_CONTROL_ID        = 0x80000000,
    META_DATA_INTEL_CAPTURE_TIMING_ID       = 0x80000001,
    META_DATA_INTEL_CONFIGURATION_ID        = 0x80000002,
    META_DATA_INTEL_STAT_ID                 = 0x80000003,
    META_DATA_INTEL_FISH_EYE_CONTROL_ID     = 0x80000004,
    META_DATA_MIPI_INTEL_RGB_ID             = 0x80000005, // D457 - added as w/a for a fw bug (which sends META_DATA_INTEL_RGB_CONTROL_ID even for mipi rgb frames)
    META_DATA_INTEL_RGB_CONTROL_ID          = 0x80000005,
    META_DATA_INTEl_FE_FOV_MODEL_ID         = 0x80000006,
    META_DATA_INTEl_FE_CAMERA_EXTRINSICS_ID = 0x80000007,
    META_DATA_CAPTURE_STATS_ID              = 0x00000003,
    META_DATA_CAMERA_EXTRINSICS_ID          = 0x00000004,
    META_DATA_CAMERA_INTRINSICS_ID          = 0x00000005,
    META_DATA_INTEL_L500_CAPTURE_TIMING_ID  = 0x80000010,
    META_DATA_INTEL_L500_DEPTH_CONTROL_ID   = 0x80000012,
    META_DATA_CAMERA_DEBUG_ID               = 0x800000FF,
    META_DATA_HID_IMU_REPORT_ID             = 0x80001001,
    META_DATA_HID_CUSTOM_TEMP_REPORT_ID     = 0x80001002,
    META_DATA_MIPI_INTEL_DEPTH_ID           = 0x80010000,
    //META_DATA_MIPI_INTEL_RGB_ID             = 0x80010001, // D457 - to be restored after the FW bug is resolved
};
#pragma pack(push, 1)
/**\brief md_header - metadata header is a integral part of all rs4XX metadata objects */
struct md_header
{
    enum md_type     md_type_id;         // The type of the metadata struct
    uint32_t    md_size;            // Actual size of metadata struct without header
};
/**\brief md_mipi_depth_mode - properties associated with sensor configuration
 *  during video streaming. Corresponds to FW STMetaDataExtMipiDepthIR object*/
struct md_mipi_depth_mode
{
    struct md_header   header;
    uint8_t     version;  // maybe need to replace by uint8_t for version and 3 others for reserved
    uint16_t    calib_info;
    uint8_t     reserved;
    uint32_t    flags;              // Bit array to specify attributes that are valid
    uint32_t    hw_timestamp;
    uint32_t    optical_timestamp;   //In microsecond unit
    uint32_t    exposure_time;      //The exposure time in microsecond unit
    uint32_t    manual_exposure;
    uint16_t    laser_power;
    uint16_t    trigger;
    uint8_t     projector_mode;
    uint8_t     preset;
    uint8_t     manual_gain;
    uint8_t     auto_exposure_mode;
    uint16_t    input_width;
    uint16_t    input_height;
    uint32_t    sub_preset_info;
    uint32_t    crc;
};
const uint8_t md_mipi_depth_mode_size = sizeof(struct md_mipi_depth_mode);
/**\brief md_mipi_rgb_mode - properties associated with sensor configuration
 *  during video streaming. Corresponds to FW STMetaDataExtMipiRgb object*/
struct md_mipi_rgb_mode
{
    struct md_header   header;
    uint32_t    version;  // maybe need to replace by uint8_t for version and 3 others for reserved
    uint32_t    flags;              // Bit array to specify attributes that are valid
    uint32_t    hw_timestamp;
    uint8_t     brightness;
    uint8_t     contrast;
    uint8_t     saturation;
    uint8_t     sharpness;
    uint16_t    auto_white_balance_temp;
    uint16_t    gamma;
    uint16_t    manual_exposure;
    uint16_t    manual_white_balance;
    uint8_t     auto_exposure_mode;
    uint8_t     gain;
    uint8_t     backlight_compensation;
    uint8_t     hue;
    uint8_t     power_line_frequency;
    uint8_t     low_light_compensation;
    uint16_t    input_width;
    uint16_t    input_height;
    uint8_t     reserved[6];
    uint32_t    crc;
};
const uint8_t md_mipi_rgb_mode_size = sizeof(struct md_mipi_rgb_mode);
#pragma pack(pop)

/**\brief metadata_mipi_raw - metadata structure
 *  layout as transmitted and received by backend */
struct metadata_mipi_depth_raw
{
    struct uvc_header_mipi    header;
    struct md_mipi_depth_mode           depth_mode;
};

int main(int argc, char **argv)
{
    char *fname;
    char *appname = argv[0];
    char buf[1024];
    int n;
    FILE *fp;
    struct md_mipi_depth_mode *md;
    struct metadata_mipi_depth_raw *md_raw;
    if (argc < 2)
        printf("Missing file name\n");
    fname = argv[1];
    printf("filename: %s\n", fname);
    fp = fopen(fname,"r");
    if (fp)
        printf("File found!\n");
    else {
        printf("File not found!\n");
        return 1;
    }
    n = fread(buf, sizeof(buf), 1, fp);
    printf("n: %d\n",n);

    
    md_raw = (struct metadata_mipi_depth_raw *)(buf);
    md = &md_raw->depth_mode;
    printf("raw: --------------------------------- %d\n", uvc_header_mipi_size);
    printf("framecounter: %d\n", md_raw->header.frame_counter);
    printf("length: %d\n", md_raw->header.header.length);
    printf("info: 0x%x\n", md_raw->header.header.info);
    printf("timestamp: %d\n", md_raw->header.header.timestamp);

    printf("---------------------------------\n");
    printf("type_id: 0x%x size: %d\n", md->header.md_type_id, md->header.md_size);

    printf("version: %d\n", md->version);
    printf("calib_info: %d\n", md->calib_info);
    printf("reserved: 0x%x\n", md->reserved);
    printf("flags: 0x%x\n", md->flags);
    printf("hw_timestamp: %d\n", md->hw_timestamp);
    printf("optical_timestamp: %d\n", md->optical_timestamp);
    printf("exposure_time: %d\n", md->exposure_time);
    printf("manual_exposure: %d\n", md->manual_exposure);
    printf("laser_power: %d\n", md->laser_power);
    printf("trigger: %d\n", md->trigger);
    printf("projector_mode: %d\n", md->projector_mode);
    printf("preset: %d\n", md->preset);
    printf("manual_gain: %d\n", md->manual_gain);
    printf("auto_exposure_mode: %d\n", md->auto_exposure_mode);
    printf("input_width: %d\n", md->input_width);
    printf("input_height: %d\n", md->input_height);
    printf("sub_preset_info: 0x%x\n", md->sub_preset_info);
    printf("crc: 0x%x\n", md->crc );

    if (fp)
        fclose(fp);

    return 0;
}