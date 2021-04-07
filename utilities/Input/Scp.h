typedef struct
{
    float    lambdaCensus;
    float    lambdaAD;
    uint32_t ignoreSAD;

    TRegScpColorControl colorControl;

    /* compare thresholds */
    TRegScpDeepseaLrThreshold leftRightThrld;
    TRegScpDeepseaScoreThreshold scoreThrld;
    TRegScpDeepseaMedianThreshold medianThrld;

    TRegScpDeepseaNeighborThreshold neighborThrld;
    TRegScpDeepseaRobbinsMunroe robbinsMunroe;
    TRegScpDeepseaSecondPeakThreshold secondPeakThrld;
    TRegScpDeepseaTextureThreshold textureThrld;

    TRegScpRauSupportVectorMinima rauVectorMinima;
    TRegScpRauThreshold rauThrld;
    TRegScpRsmControlReg rsmControl;

    TRegScpSloK1penalty sloK1P;
    TRegScpSloK1penaltymod1 sloK1PMod1;
    TRegScpSloK1penaltymod2 sloK1PMod2;
    TRegScpSloK2penalty sloK2P;
    TRegScpSloK2penaltymod1 sloK2PMod1;
    TRegScpSloK2penaltymod2 sloK2PMod2;
    TRegScpSloThreshold sloThrld;

    ETDisparityMode disparityMode;
    uint32_t disparityMultiplier;
    uint32_t disparityShift;

    uint32_t censusUSize;
    uint32_t censusVSize;

    int32_t minZ;
    int32_t maxZ;
    uint32_t zUnits;
}STScpParams;
