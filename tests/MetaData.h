#ifndef _METADATA_
#define _METADATA_

#define META_DATA_INTEL_DEPTH_CONTROL_ID        0x80000000;
#define META_DATA_INTEL_CAPTURE_TIMING_ID       0x80000001;
#define META_DATA_INTEL_CONFIGURATION_ID        0x80000002;
#define META_DATA_INTEL_STAT_ID                 0x80000003;
#define META_DATA_CAPTURE_STATS_ID              0x00000003;

typedef struct
{
    uint32_t    metaDataID;
    uint32_t    size;
}STMetaDataIdHeader;

//////////////////////////
// Intel Capture Timing //
//////////////////////////

typedef struct
{
    STMetaDataIdHeader  metaDataIdHeader;
    uint32_t    version;
    uint32_t    flag;
    uint32_t    frameCounter;
    uint32_t    opticalTimestamp;   //In millisecond unit
    uint32_t    readoutTime;        //The readout time in microsecond second unit
    uint32_t    exposureTime;       //The exposure time in microsecond second unit
    uint32_t    frameInterval ;     //The frame interval in microsecond second unit
    uint32_t    pipeLatency;        //The latency between start of frame to frame ready in USB buffer
} __attribute__((packed))STMetaDataIntelCaptureTiming;

///////////////////
// Capture Stats //
///////////////////

typedef struct
{
    STMetaDataIdHeader  metaDataIdHeader;
    uint32_t Flags;
    uint32_t hwTimestamp;
    uint64_t ExposureTime;
    uint64_t ExposureCompensationFlags;
    int32_t  ExposureCompensationValue;
    uint32_t IsoSpeed;
    uint32_t FocusState;
    uint32_t LensPosition; // a.k.a Focus
    uint32_t WhiteBalance;
    uint32_t Flash;
    uint32_t FlashPower;
    uint32_t ZoomFactor;
    uint64_t SceneMode;
    uint64_t SensorFramerate;
}__attribute__((packed)) STMetaDataCaptureStats;

////////////////////////
//Intel Depth Control //
////////////////////////

typedef struct
{
    STMetaDataIdHeader  metaDataIdHeader;
    uint32_t    version;
    uint32_t    flag;
    uint32_t    manualGain;         //Manual gain value
    uint32_t    manualExposure;     //Manual exposure
    uint32_t    laserPower ;        //Laser power value
    uint32_t    autoExposureMode;   //AE mode
    uint32_t    exposurePriority ;
    uint32_t    exposureROILeft;
    uint32_t    exposureROIRight;
    uint32_t    exposureROITop;
    uint32_t    exposureROIBottom;
    uint32_t    preset;
    uint8_t     projectorMode;
    uint8_t     reserved;
    uint16_t    ledPower;

}__attribute__((packed)) STMetaDataIntelDepthControl;


////////////////////////
//Intel Configuration //
////////////////////////

typedef union STTriggerMode
{
    uint16_t value;
    struct
    {
        uint16_t  inSync      :1;
        uint16_t  extTrigger  :1;
        uint16_t  reserved   :14;
    }fields;
}STTriggerMode;

typedef struct
{
    STMetaDataIdHeader  metaDataIdHeader;
    uint32_t      version;
    uint32_t      flag;
    uint8_t       HWType;             //(IVCAM2 , DS5 etc)
    uint8_t       SKUsID;
    uint32_t      cookie;             //Place Holder enable FW to bundle cookie with control state and configuration.
    uint16_t      format ;
    uint16_t      width;              //Requested resolution
    uint16_t      height ;
    uint16_t      FPS;                //Requested FPS
    STTriggerMode trigger;            /*Byte <0> \96 0 free-running
                                                 1 in sync
                                                 2 external trigger (depth only)
                                      Byte <1> \96 configured delay (depth only)*/
    uint16_t      calibrationCount;
    uint8_t       Reserved[6];
}__attribute__((packed))STMetaDataIntelConfiguration;

////////////////////////////
//    MetaData modes      //
////////////////////////////
typedef struct
{
    STMetaDataIntelCaptureTiming intelCaptureTiming;
    STMetaDataCaptureStats       captureStats;
    STMetaDataIntelDepthControl  intelDepthControl;
    STMetaDataIntelConfiguration intelConfiguration;
    uint32_t crc32;
}__attribute__((packed))STMetaDataDepthYNormalMode;

////////////////////////////
//       SUB preset       //
////////////////////////////
typedef union STSubPresetInfo
{
    uint32_t value;
    struct
    {
	uint32_t  id		:4;	// -
	uint32_t  numOfItems	:8;	// - according to SubPresetTool ver 2.0.0
	uint32_t  itemIndex	:8;	// -
	uint32_t  iteration	:6;	// - for debug purposes
	uint32_t  itemIteration	:6;	// - for debug purposes
    };
}STSubPresetInfo;


////////////////////////////
//    NEW DEPTH STRUCT    //
////////////////////////////
typedef struct
{
    STMetaDataIdHeader  metaDataIdHeader;
    uint8_t    version;
    uint8_t reserved[3];
    uint32_t    flags;
    uint32_t    opticalTimestamp;   // In millisecond unit
    uint32_t    hwTimestamp;
    uint32_t    exposureTime;       //The exposure time in microsecond second unit
    uint32_t    manualExposure;
    uint16_t	laserPower;	    // Laser power value
    STTriggerMode trigger; 	    /* Byte <0> - 0 free-running
						  1 in sync
						  2 external trigger (depth only)*/
    uint8_t	projectorMode;
    uint8_t	preset;
    uint8_t	manualGain;
    uint8_t	autoExposureMode;
    uint16_t	inputWidth;
    uint16_t	inputHeight;
    STSubPresetInfo subpresetInfo;
    uint32_t	crc32;
} __attribute__((packed))STMetaDataExtMipiDepthIR;

////////////////////////////
//     NEW RGB STRUCT     //
////////////////////////////
typedef struct
{
    STMetaDataIdHeader  metaDataIdHeader;
    uint8_t    	version;
    uint8_t 	reserved[3];
    uint32_t    flags;
    uint32_t    hwTimestamp;
    uint8_t    	brightness;
    uint8_t    	contrast;
    uint8_t    	saturation;
    uint8_t    	sharpness;
    uint16_t    auto_WB_Temp;
    uint16_t    gamma;
    uint16_t    manual_Exp;
    uint16_t    manual_WB;
    uint8_t    	auto_Exp_Mode;
    uint8_t    	gain;
    uint8_t    	backlight_Comp;
    uint8_t    	hue;
    uint8_t    	powerLineFrequency;
    uint8_t    	low_Light_comp;
    uint16_t    inputWidth;
    uint16_t    inputHeight;
    uint8_t 	reserved1[6];
    uint32_t	crc32;
} __attribute__((packed))STMetaDataExtMipiRgb;

#endif // _METADATA_
