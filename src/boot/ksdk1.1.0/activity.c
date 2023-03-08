#include <stdlib.h>
#include <math.h>

/*
 *	config.h needs to come first
 */
#include "config.h"
#include "warp.h"

#include "activity.h"

#include "devMMA8451Q.h"

#define REG_VAL_TYPE    double

#define G_VAL           9.81

/* Feature buffer: Features are as follows:
    0-2: mean values along X, Y, Z
    3-5: standard deviations
    6-8: variance
    9:   mean magnitude of acceleration (resultant)
*/
volatile REG_VAL_TYPE featureBuff[TRACKER_NUM_FEATURES]      = {0};

void
warpPrintDoubleVec(double x, double y, double z) {
    warpPrint("%de-3, %de-3, %de-3\n", (uint32_t)(x * 1000.0), (uint32_t)(y * 1000.0), (uint32_t)(z * 1000.0));
}


double
gaussian(double x, double mean, double std) {
    double temp = (x - mean) / std;

    return 1.0 / (sqrt(2.0 * M_PI) * std) * exp(-0.5 * temp * temp);
}

void
getClass() {
    const double mean[TRACKER_NUM_FEATURES * TRACKER_NUM_CLASSES] =   {13.42378107, 
                                                 9.76847904};
    const double std[TRACKER_NUM_FEATURES * TRACKER_NUM_CLASSES]  =    {3.30213513, 
                                                 0.21466643};

    double prob_class[TRACKER_NUM_CLASSES];

    double prob_sum = 0.0;

    for(int j = 0; j < TRACKER_NUM_CLASSES; j++)
    {
        prob_class[j] = 1.0;

        for(int i = 0; i < TRACKER_NUM_FEATURES; i++)
        {
            double prob_feature = gaussian(featureBuff[i], mean[j * TRACKER_NUM_FEATURES + i], std[j * TRACKER_NUM_FEATURES + i]);

            warpPrint("Prob feature %d, classm %d: %de-3\n", i, j, (uint32_t)(prob_feature * 1000.0));

            prob_class[j] *= prob_feature;
        }

        prob_sum += prob_class[j];
    }

    for(int j = 0; j < TRACKER_NUM_CLASSES; j++)
    {
        prob_class[j] /= prob_sum;

        warpPrint("Classification %d prob: %de-3\n", j, (uint32_t)(prob_class[j] * 1000.0));
    }
}


void
trackerUpdate()
{
    int16_t accX_i = getRegisterValueCombined(kWarpSensorOutputRegisterMMA8451QOUT_X_MSB);
	int16_t accY_i = getRegisterValueCombined(kWarpSensorOutputRegisterMMA8451QOUT_Y_MSB);
	int16_t accZ_i = getRegisterValueCombined(kWarpSensorOutputRegisterMMA8451QOUT_Z_MSB);

    REG_VAL_TYPE accX = (REG_VAL_TYPE)(accX_i) * G_VAL / 4096.0;
    REG_VAL_TYPE accY = (REG_VAL_TYPE)(accY_i) * G_VAL / 4096.0;
    REG_VAL_TYPE accZ = (REG_VAL_TYPE)(accZ_i) * G_VAL / 4096.0;

    //warpPrintDoubleVec(accX, accY, accZ);

    // Mean values

    /*featureBuff[0] += accX / (REG_VAL_TYPE)(TRACKER_NUM_MEASUREMENTS);
    featureBuff[1] += accY / (REG_VAL_TYPE)(TRACKER_NUM_MEASUREMENTS);
    featureBuff[2] += accZ / (REG_VAL_TYPE)(TRACKER_NUM_MEASUREMENTS);*/

    // Mean of squares of values

    REG_VAL_TYPE accX2 = accX * accX;
    REG_VAL_TYPE accY2 = accY * accY;
    REG_VAL_TYPE accZ2 = accZ * accZ;

    //warpPrintDoubleVec(accX2, accY2, accZ2);

    /*featureBuff[3] += accX2 / (REG_VAL_TYPE)(TRACKER_NUM_MEASUREMENTS);
    featureBuff[4] += accY2 / (REG_VAL_TYPE)(TRACKER_NUM_MEASUREMENTS);
    featureBuff[5] += accZ2 / (REG_VAL_TYPE)(TRACKER_NUM_MEASUREMENTS);*/

    // Mean magnitude

    featureBuff[0] += sqrt(accX2 + accY2 + accZ2) / (REG_VAL_TYPE)(TRACKER_NUM_MEASUREMENTS);
}

void
trackerProcess()
{
    // Mean values - done already

    // Variance

    /*featureBuff[6] = (featureBuff[3] - featureBuff[0] * featureBuff[0]) / (REG_VAL_TYPE)(TRACKER_NUM_MEASUREMENTS);
    featureBuff[7] = (featureBuff[4] - featureBuff[1] * featureBuff[1]) / (REG_VAL_TYPE)(TRACKER_NUM_MEASUREMENTS);
    featureBuff[8] = (featureBuff[5] - featureBuff[2] * featureBuff[2]) / (REG_VAL_TYPE)(TRACKER_NUM_MEASUREMENTS);

    // Standard deviation

    featureBuff[3] = sqrt(featureBuff[6]);
    featureBuff[4] = sqrt(featureBuff[7]);
    featureBuff[5] = sqrt(featureBuff[8]);*/

    // Mean magnitude (resultant) - done already

}

void
trackerClassify()
{
    for(int i = 0; i < TRACKER_NUM_FEATURES; i++)
    {
        warpPrint("Feature %d: %de-3\n", i, (uint32_t)(featureBuff[i] * 1000.0));
    }

    getClass();
}

void
trackerClearFeatures()
{
    for(int i = 0; i < TRACKER_NUM_FEATURES; i++)
    {
        featureBuff[i] = 0.0f;
    }
}