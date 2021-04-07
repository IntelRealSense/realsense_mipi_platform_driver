typedef struct
{
	TPresetName name;

    STScpParams scpParams;

    uint32_t autoExposureSetPoint;  // ok

    T_COLOR_CORRECTION_MATRIX_FLOAT pColorCorrection;   // ok

    // default for controls
    uint32_t laserState;
    uint32_t laserPower;
    uint16_t roiTop;
    uint16_t roiBottom;
    uint16_t roiLeft;
    uint16_t roiRight;
    uint32_t autoExposure;
    uint32_t manualExposure;
    uint32_t gain;

    uint32_t autoExposureFaceAuthLoops;
    uint32_t exposureListSize;
    uint32_t exposureList[AE_FACE_NOF_PARAMS_MAX];

}STPreset;

