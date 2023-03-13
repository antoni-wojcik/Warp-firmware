#include <stdlib.h>
#include <math.h>

/*
 *	config.h needs to come first
 */
#include "config.h"
#include "warp.h"

#include "tracker.h"

#include "devMMA8451Q.h"
#include "devSSD1331.h"

#define TRACKER_DEBUG   0

#define G_VAL           9.80665 // gravitational acceleration in ms^-2
#define LINE_SPACING    4 // spacing between lines on the display, in pixels

/* Feature buffer: Features are as follows:
    0-2: mean values along X, Y, Z
    3-5: standard deviations
    6-8: variance
    9:   mean magnitude of acceleration (resultant)
*/
volatile double featureBuff[TRACKER_NUM_FEATURES]  = {0};

/* Mean of the accelertions and accelerations squared along each axis of the accelerometer */
volatile double meanAcc[TRACKER_NUM_AXES]     = {0};
volatile double meanAccSq[TRACKER_NUM_AXES]   = {0};

/* gaussian function */
static double
gaussian(double x, double mean, double std) {
    double temp = (x - mean) / std;

    return 1.0 / (sqrt(2.0 * M_PI) * std) * exp(-0.5 * temp * temp);
}

/* classify activity over the last 10s and give the uncertainity in the estimation.
   The chosen method uses Naive Bayes Classifier which was pre-trained on the 
   WISDM Smartphone and Smartwatch Activity and Biometrics Dataset. */
static void
getClass(double* prob_class) {
    /* Mean and standard deviations used in the classification */
    const double mean[TRACKER_NUM_FEATURES * TRACKER_NUM_CLASSES] = {0.04192712, 0.04903356, 0.04862857,  9.76841105,  // stationary
                                                                           0.25256254, 0.21520796, 0.18676971, 11.47028526,  // walking
                                                                           0.61087809, 0.70592706, 0.4157007,  15.44014891}; // jogging/running

    const double std[TRACKER_NUM_FEATURES * TRACKER_NUM_CLASSES]  = {0.0768613,  0.07855051, 0.08087657, 0.21409689,   // stationary
                                                                           0.11724238, 0.11178535, 0.14741955, 1.21774021,   // walking
                                                                           0.14744818, 0.16891946, 0.20719342, 3.5296267 };  // jogging/running

    /* sum of probabilities, used to normalize them */
    double prob_sum = 0.0;

    /* calcuate class probabilities using naive Gaussian approach and the Bayes theorem */
    for(uint8_t j = 0; j < TRACKER_NUM_CLASSES; j++)
    {
        prob_class[j] = 1.0;

        for(uint8_t i = 0; i < TRACKER_NUM_FEATURES; i++)
        {
            double prob_feature = gaussian(featureBuff[i], mean[j * TRACKER_NUM_FEATURES + i], std[j * TRACKER_NUM_FEATURES + i]);

            #if(TRACKER_DEBUG)
                warpPrint("Prob feature %d, class %d: %de-3\n", i, j, (uint32_t)(prob_feature * 1000.0));
            #endif

            prob_class[j] *= prob_feature;
        }

        prob_sum += prob_class[j];
    }

    /* normalize probabilities assuming equal priors for all classes */
    for(uint8_t j = 0; j < TRACKER_NUM_CLASSES; j++)
    {
        prob_class[j] /= prob_sum;
    }
}

/* draw text showing the results on the display */
static void
drawResults(double* prob_class)
{
    const char class_names[3][8] = {"Resting:", "Walking:", "Running:"};

    double max_prob = 0.0;
    uint8_t class_max_prob = 0;
    
    /* find the most likely class */
    for(uint8_t j = 0; j < TRACKER_NUM_CLASSES; j++)
    {
        #if(TRACKER_DEBUG)
            warpPrint("Classification %d prob: %de-3\n", j, (uint32_t)(prob_class[j] * 1000.0));
        #endif

        if(prob_class[j] > max_prob)
        {
            max_prob = prob_class[j];
            
            class_max_prob = j;
        }
    }

    clearScreen();


    uint8_t x_offset;
    SSD1331Colors text_color;

    /* show names of the classes and associated probabilities.
       Mark the class with the highest probability in green */
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

/* update the values of mean acceleartion and mean acceleration squared used to calculate the features */
void
trackerUpdate(void)
{
    double accSqMag = 0.0;

    for(uint8_t i = 0; i < TRACKER_NUM_AXES; i++) 
    {
        int16_t acc_i = getRegisterValueCombined(kWarpSensorOutputRegisterMMA8451QOUT_X_MSB + i * 2);

        double acc = (double)(acc_i) * G_VAL / 2048.0;

        // mean values

        meanAcc[i] += acc / (double)(TRACKER_NUM_MEASUREMENTS);

        // mean of squares of values

        double accSq = acc * acc;

        meanAccSq[i] += accSq / (double)(TRACKER_NUM_MEASUREMENTS);

        accSqMag += accSq;
    }

    // mean magnitude - store in the the feature buffer

    featureBuff[3] += sqrt(accSqMag) / (double)(TRACKER_NUM_MEASUREMENTS);
}

/* calculate standard deviations of acceleration along x, y, z axes 
   and store them in the feature buffer */
void
trackerProcess(void)
{
    for(uint8_t i = 0; i < TRACKER_NUM_AXES; i++) 
    {
        // variance

        double var = (meanAccSq[i] - meanAcc[i] * meanAcc[i]) / (double)(TRACKER_NUM_MEASUREMENTS);

        // standard deviation

        featureBuff[i] = sqrt(var);
    }

    // mean magnitude (resultant) - done already

}

/* call the classifier and drawing functions */
void
trackerClassify(void)
{
    double prob_class[TRACKER_NUM_CLASSES];

    #if(TRACKER_DEBUG)
        for(uint8_t i = 0; i < TRACKER_NUM_FEATURES; i++)
        {
            warpPrint("Feature %d: %de-3\n", i, (uint32_t)(featureBuff[i] * 1000.0));
        }
    #endif

    getClass(prob_class);

    drawResults(prob_class);
}

/* reset the buffers to 0 */
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

/* show the initialization message */
void
trackerInit(void)
{
    drawText("Booted", 6, 0, 0, kSSD1331ColorWHITE);
    drawText("Running...", 10, 0, (SSD1331_CHAR_HEIGHT + LINE_SPACING), kSSD1331ColorWHITE);
    drawLine(0, 0, SSD1331_SCR_WIDTH - 1, 0, 0, (SSD1331_CHAR_HEIGHT + LINE_SPACING) * 3 + LINE_SPACING, kSSD1331ColorBLUE);
}

/* update the countdown on the display */
void
trackerDrawCountdown(uint8_t time)
{
    if(time == 9)
    {
        drawText("Time left:", 10, 0, (SSD1331_CHAR_HEIGHT + LINE_SPACING) * 4, kSSD1331ColorBLUE);
    }
    drawDigit(time, SSD1331_SCR_WIDTH - SSD1331_CHAR_WIDTH, (SSD1331_CHAR_HEIGHT + LINE_SPACING) * 4, kSSD1331ColorBLUE);
}