#define TRACKER_REFRESH_RATE_MS         50  // 20Hz sample rate - each measurement can take up to 50 ms
#define TRACKER_NUM_AXES                3   // 3 measuremnt axes
#define TRACKER_NUM_FEATURES            4   // 4 features to extract
#define TRACKER_NUM_CLASSES             3   // 3 classes
#define TRACKER_NUM_MEASUREMENTS        200 // 200 measurements in 10s and 20Hz sample rate


void trackerUpdate(void);
void trackerProcess(void);
void trackerClassify(void);
void trackerClearFeatures(void);
void trackerInit(void);
void trackerDrawCountdown(uint8_t time);