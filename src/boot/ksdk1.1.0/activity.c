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

#define G_VAL           9.80665
#define LINE_SPACING    4

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
warpPrintDoubleVec(REG_VAL_TYPE x, REG_VAL_TYPE y, REG_VAL_TYPE z) {
    warpPrint("%de-3, %de-3, %de-3\n", (uint32_t)(x * 1000.0), (uint32_t)(y * 1000.0), (uint32_t)(z * 1000.0));
}


REG_VAL_TYPE
gaussian(REG_VAL_TYPE x, REG_VAL_TYPE mean, REG_VAL_TYPE std) {
    REG_VAL_TYPE temp = (x - mean) / std;

    return 1.0 / (sqrt(2.0 * M_PI) * std) * exp(-0.5 * temp * temp);
}

void
getClass(REG_VAL_TYPE* prob_class) {
    const REG_VAL_TYPE mean[TRACKER_NUM_FEATURES * TRACKER_NUM_CLASSES] = {0.04192712, 0.04903356, 0.04862857,  9.76841105,  // stationary
                                                                           0.25256254, 0.21520796, 0.18676971, 11.47028526,  // walking
                                                                           0.61087809, 0.70592706, 0.4157007,  15.44014891}; // jogging/running

    const REG_VAL_TYPE std[TRACKER_NUM_FEATURES * TRACKER_NUM_CLASSES]  = {0.0768613,  0.07855051, 0.08087657, 0.21409689,   // stationary
                                                                           0.11724238, 0.11178535, 0.14741955, 1.21774021,   // walking
                                                                           0.14744818, 0.16891946, 0.20719342, 3.5296267 };  // jogging/running

    REG_VAL_TYPE prob_sum = 0.0;

    for(uint8_t j = 0; j < TRACKER_NUM_CLASSES; j++)
    {
        prob_class[j] = 1.0;

        for(uint8_t i = 0; i < TRACKER_NUM_FEATURES; i++)
        {
            REG_VAL_TYPE prob_feature = gaussian(featureBuff[i], mean[j * TRACKER_NUM_FEATURES + i], std[j * TRACKER_NUM_FEATURES + i]);

            // warpPrint("Prob feature %d, classm %d: %de-3\n", i, j, (uint32_t)(prob_feature * 1000.0));

            prob_class[j] *= prob_feature;
        }

        prob_sum += prob_class[j];
    }

    for(uint8_t j = 0; j < TRACKER_NUM_CLASSES; j++)
    {
        prob_class[j] /= prob_sum;
    }
}

void
drawResults(REG_VAL_TYPE* prob_class)
{
    const char class_names[3][8] = {"Resting:", "Walking:", "Running:"};

    REG_VAL_TYPE max_prob = 0.0;
    uint8_t class_max_prob = 0;
        
    for(uint8_t j = 0; j < TRACKER_NUM_CLASSES; j++)
    {
        warpPrint("Classification %d prob: %de-3\n", j, (uint32_t)(prob_class[j] * 1000.0));

        if(prob_class[j] > max_prob)
        {
            max_prob = prob_class[j];
            
            class_max_prob = j;
        }
    }

    clearScreen();


    uint8_t x_offset;
    SSD1331Colors text_color;

    for(uint8_t j = 0; j < TRACKER_NUM_CLASSES; j++)
    {
        if(j == class_max_prob) {
            text_color = kSSD1331ColorGREEN;
        }
        else{
            text_color = kSSD1331ColorWHITE;
        }

        x_offset = drawText(class_names[j], 8, 0, (SSD1331_CHAR_HEIGHT + LINE_SPACING) * j, text_color);
        drawProb(prob_class[j], x_offset + 6, (SSD1331_CHAR_HEIGHT + LINE_SPACING) * j, text_color);
    }

    drawLine(0, 0, SSD1331_SCR_WIDTH - 1, 0, 0, (SSD1331_CHAR_HEIGHT + LINE_SPACING) * 3 + LINE_SPACING, kSSD1331ColorBLUE);
}


void
trackerUpdate(void)
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
trackerProcess(void)
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
trackerClassify(void)
{
    REG_VAL_TYPE prob_class[TRACKER_NUM_CLASSES];

    for(uint8_t i = 0; i < TRACKER_NUM_FEATURES; i++)
    {
        warpPrint("Feature %d: %de-3\n", i, (uint32_t)(featureBuff[i] * 1000.0));
    }

    getClass(prob_class);

    drawResults(prob_class);
}

void
trackerClearFeatures(void)
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
trackerInit(void)
{
    drawText("Booted", 6, 0, 0, kSSD1331ColorWHITE);
    drawText("Running...", 10, 0, (SSD1331_CHAR_HEIGHT + LINE_SPACING), kSSD1331ColorWHITE);
    drawLine(0, 0, SSD1331_SCR_WIDTH - 1, 0, 0, (SSD1331_CHAR_HEIGHT + LINE_SPACING) * 3 + LINE_SPACING, kSSD1331ColorBLUE);
}

void
trackerDrawCountdown(uint8_t time)
{
    if(time == 9)
    {
        drawText("Time left:", 10, 0, (SSD1331_CHAR_HEIGHT + LINE_SPACING) * 4, kSSD1331ColorBLUE);
    }
    drawDigit(time, SSD1331_SCR_WIDTH - SSD1331_CHAR_WIDTH, (SSD1331_CHAR_HEIGHT + LINE_SPACING) * 4, kSSD1331ColorBLUE);
}