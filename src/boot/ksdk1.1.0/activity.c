#include <stdlib.h>
#include <math.h>

/*
 *	config.h needs to come first
 */
#include "config.h"
#include "warp.h"

#include "activity.h"

#include "devMMA8451Q.h"
#include "devSSD1331.h"

#define REG_VAL_TYPE    double

#define G_VAL           9.81

/* Feature buffer: Features are as follows:
    0-2: mean values along X, Y, Z
    3-5: standard deviations
    6-8: variance
    9:   mean magnitude of acceleration (resultant)
*/
volatile REG_VAL_TYPE featureBuff[TRACKER_NUM_FEATURES]  = {0};

static REG_VAL_TYPE meanAcc[TRACKER_NUM_AXES]     = {0};
static REG_VAL_TYPE meanAccSq[TRACKER_NUM_AXES]   = {0};

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
    const double mean[TRACKER_NUM_FEATURES * TRACKER_NUM_CLASSES] = {0.25307806, 0.21680997, 0.18862757, 11.483962,
                                                                     0.60996965, 0.7063934,  0.41486924, 15.43389935,
                                                                     0.0419608,  0.04912008, 0.04823104, 9.76847904};

    const double std[TRACKER_NUM_FEATURES * TRACKER_NUM_CLASSES]  = {0.1172738,  0.11280127, 0.14980334, 1.2231232,
                                                                     0.14828773, 0.17132559, 0.20834945, 3.56556733,
                                                                     0.07756477, 0.07896084, 0.0804943,  0.21466643};

    double prob_class[TRACKER_NUM_CLASSES];

    double prob_sum = 0.0;

    double max_prob = 0.0;
    uint8_t class_max_prob = 0;

    for(uint8_t j = 0; j < TRACKER_NUM_CLASSES; j++)
    {
        prob_class[j] = 1.0;

        for(uint8_t i = 0; i < TRACKER_NUM_FEATURES; i++)
        {
            double prob_feature = gaussian(featureBuff[i], mean[j * TRACKER_NUM_FEATURES + i], std[j * TRACKER_NUM_FEATURES + i]);

            // warpPrint("Prob feature %d, classm %d: %de-3\n", i, j, (uint32_t)(prob_feature * 1000.0));

            prob_class[j] *= prob_feature;
        }

        prob_sum += prob_class[j];
    }

    for(uint8_t j = 0; j < TRACKER_NUM_CLASSES; j++)
    {
        prob_class[j] /= prob_sum;

        warpPrint("Classification %d prob: %de-3\n", j, (uint32_t)(prob_class[j] * 1000.0));

        if(prob_class[j] > max_prob)
        {
            max_prob = prob_class[j];
            
            class_max_prob = j;
        }
    }

    clearScreen();

    /*uint8_t x_offset;

    switch(class_max_prob)
    {
        case 0:
        {
            x_offset = drawText("Walking:", 8, 0, 0, kSSD1331ColorWHITE);
            break;
        }
        case 1:
        {
            x_offset = drawText("Jogging:", 8, 0, 0, kSSD1331ColorWHITE);
            break;
        }
        case 2:
        {
            x_offset = drawText("Idle:", 5, 0, 0, kSSD1331ColorWHITE);
            break;
        }
    }

    x_offset += drawProb(max_prob, x_offset, 0, kSSD1331ColorWHITE);

    warpPrint("X-offset = %d\n", x_offset);*/

    uint8_t x_offset;

    x_offset = drawText("Walking:", 8, 0, 0, kSSD1331ColorWHITE);
    drawProb(prob_class[0], x_offset, 0, kSSD1331ColorWHITE);

    x_offset = drawText("Jogging:", 8, 0, 0, kSSD1331ColorWHITE);
    drawProb(prob_class[1], x_offset, SSD1331_CHAR_HEIGHT + 2, kSSD1331ColorWHITE);

    x_offset = drawText("Idle:", 5, 0, 0, kSSD1331ColorWHITE);
    drawProb(prob_class[2], x_offset, (SSD1331_CHAR_HEIGHT + 2) * 2, kSSD1331ColorWHITE);

}


void
trackerUpdate()
{
    REG_VAL_TYPE accSqMag = 0.0;

    for(uint8_t i = 0; i < TRACKER_NUM_AXES; i++) 
    {
        int16_t acc_i = getRegisterValueCombined(kWarpSensorOutputRegisterMMA8451QOUT_X_MSB + i * 2);

        REG_VAL_TYPE acc = (REG_VAL_TYPE)(acc_i) * G_VAL / 4096.0;

        // Mean values

        meanAcc[i] += acc / (REG_VAL_TYPE)(TRACKER_NUM_MEASUREMENTS);

        // Mean of squares of values

        REG_VAL_TYPE accSq = acc * acc;

        //warpPrintDoubleVec(accX2, accY2, accZ2);

        meanAccSq[i] += accSq / (REG_VAL_TYPE)(TRACKER_NUM_MEASUREMENTS);

        accSqMag += accSq;
    }

    // Mean magnitude

    featureBuff[3] += sqrt(accSqMag) / (REG_VAL_TYPE)(TRACKER_NUM_MEASUREMENTS);
}

void
trackerProcess()
{
    for(uint8_t i = 0; i < TRACKER_NUM_AXES; i++) 
    {
        // Variance

        REG_VAL_TYPE var = (meanAccSq[i] - meanAcc[i] * meanAcc[i]) / (REG_VAL_TYPE)(TRACKER_NUM_MEASUREMENTS);

        // Standard deviation

        featureBuff[i] = sqrt(var);
    }

    // Mean magnitude (resultant) - done already

}

void
trackerClassify()
{
    for(uint8_t i = 0; i < TRACKER_NUM_FEATURES; i++)
    {
        warpPrint("Feature %d: %de-3\n", i, (uint32_t)(featureBuff[i] * 1000.0));
    }

    getClass();
}

void
trackerClearFeatures()
{
    for(uint8_t i = 0; i < TRACKER_NUM_FEATURES; i++)
    {
        featureBuff[i] = 0.0f;
    }

    for(uint8_t i = 0; i < TRACKER_NUM_AXES; i++)
    {
        meanAcc[i]   = 0.0;
        meanAccSq[i] = 0.0;
    }
}

void
trackerInit()
{
    drawText("Booted", 6, 0, 0, kSSD1331ColorWHITE);
}