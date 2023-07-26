#pragma once

namespace drs {

    enum DRS_TOUCH_TYPE {
        DRS_DOWN = 0,
        DRS_UP   = 1,
        DRS_MOVE = 2,
    };

    typedef struct drs_touch_t {
        int type = DRS_UP;
        int id = 0;
        double x = 0.0;
        double y = 0.0;
        double width = 1;
        double height = 1;
    };

    struct VRFoot {
        unsigned int id;
        unsigned int index = 2;
        float length = 3.1;
        float size_base = 0.05;
        float size_scale = 0.1;
        float height = 3;
        //linalg::aliases::float4 rotation {0, 0, 0, 1};
        drs_touch_t event{};
        //unsigned int get_index();
        //linalg::aliases::float3 to_world(linalg::aliases::float3 pos);
    };

    extern char DRS_TAPELED[38 * 49][3];
    //extern linalg::aliases::float3 VR_SCALE;
    //extern linalg::aliases::float3 VR_OFFSET;
    extern float VR_ROTATION;
    extern VRFoot VR_FOOTS[2];

    void fire_touches(drs_touch_t* events, size_t event_count);
    //void start_vr();

    void hookDancepad();
}
