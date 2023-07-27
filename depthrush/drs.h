#pragma once
enum DRS_TOUCH_TYPE {
    DRS_DOWN = 0,
    DRS_UP = 1,
    DRS_MOVE = 2,
};

typedef struct drs_touch_t {
    int type = DRS_UP;
    int id = 0;
    double x = 0.0; // x = pixelX/1920
    double y = 0.0; // y = pixelY/1920
    double width = 1; // width = pixelWidth/1920
    double height = 1; // height = pixelHeight/1920
};

struct VRFoot {
    unsigned int id;
    unsigned int index = 2;
    float length = 3.1;
    float size_base = 0.05;
    float size_scale = 0.1;
    float height = 3;
    drs_touch_t event{};
    bool touching = false;
};

void fire_touches(drs_touch_t* events, size_t event_count);

void start_kinect();

void hookDancepad();