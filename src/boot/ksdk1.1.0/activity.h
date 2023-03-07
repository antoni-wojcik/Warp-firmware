#define TRACKER_REFRESH_RATE_MS         50
#define TRACKER_NUM_AXES                3
#define TRACKER_NUM_FEATURES            10
#define TRACKER_NUM_MEASUREMENTS        200


void trackerUpdate();
void trackerProcess();
void trackerClassify();
void trackerClearFeatures();