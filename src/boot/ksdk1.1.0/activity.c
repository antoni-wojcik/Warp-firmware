#include <stdlib.h>
#include <math.h>

/*
 *	config.h needs to come first
 */
#include "config.h"
#include "warp.h"

#include "activity.h"

#include "devMMA8451Q.h"

volatile float featureBuff[TRACKER_NUM_FEATURES]      = {0};

void
trackerUpdate()
{
    uint16_t accX = getRegisterValueCombined(kWarpSensorOutputRegisterMMA8451QOUT_X_MSB);
	uint16_t accY = getRegisterValueCombined(kWarpSensorOutputRegisterMMA8451QOUT_Y_MSB);
	uint16_t accZ = getRegisterValueCombined(kWarpSensorOutputRegisterMMA8451QOUT_Z_MSB);

    // Mean values

    featureBuff[0] += (float)(accX) / (float)(TRACKER_NUM_MEASUREMENTS);
    featureBuff[1] += (float)(accY) / (float)(TRACKER_NUM_MEASUREMENTS);
    featureBuff[2] += (float)(accZ) / (float)(TRACKER_NUM_MEASUREMENTS);

    // Mean of squares of values

    float accX2 = (float)(accX) * (float)(accX);
    float accY2 = (float)(accY) * (float)(accY);
    float accZ2 = (float)(accZ) * (float)(accZ);

    featureBuff[3] += accX2 / (float)(TRACKER_NUM_MEASUREMENTS);
    featureBuff[4] += accY2 / (float)(TRACKER_NUM_MEASUREMENTS);
    featureBuff[5] += accZ2 / (float)(TRACKER_NUM_MEASUREMENTS);

    // Mean magnitude

    featureBuff[9] += sqrtf(accX2 + accY2 + accZ2) / (float)(TRACKER_NUM_MEASUREMENTS);
}

void
trackerProcess()
{
    // Mean values - done already

    // Variance

    featureBuff[3] = (featureBuff[0] * featureBuff[0] - featureBuff[3]) / (float)(TRACKER_NUM_MEASUREMENTS);
    featureBuff[4] = (featureBuff[1] * featureBuff[1] - featureBuff[4]) / (float)(TRACKER_NUM_MEASUREMENTS);
    featureBuff[5] = (featureBuff[2] * featureBuff[2] - featureBuff[5]) / (float)(TRACKER_NUM_MEASUREMENTS);

    // Standard deviation

    featureBuff[6] = sqrtf(featureBuff[3]);
    featureBuff[7] = sqrtf(featureBuff[4]);
    featureBuff[8] = sqrtf(featureBuff[5]);

    // Mean magnitude (resultant) - done already
}

void
trackerClassify()
{
    for(int i = 0; i < TRACKER_NUM_FEATURES; i++)
    {
        warpPrint("Feature %d value = %d\n", i, (uint64_t)(featureBuff[i]));
    }
}

void
trackerClearFeatures()
{
    for(int i = 0; i < TRACKER_NUM_FEATURES; i++)
    {
        featureBuff[i] = 0.0f;
    }
}