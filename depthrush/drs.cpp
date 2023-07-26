#pragma pack(push)
#include <windows.h>
#include <thread>
#include "includes.h"

bool kinectRunning = false;
bool kinectStarted = false;

extern VRFoot VR_FOOTS[2];

typedef struct {
    union {
        struct {
            WORD unk1;
            WORD unk2;
            WORD device_id;
            WORD vid;
            WORD pid;
            WORD pvn;
            WORD max_point_num;
        };
        uint8_t raw[2356];
    };
} dev_info_t;

typedef struct {
    DWORD cid;
    DWORD type;
    DWORD unused;
    DWORD y;
    DWORD x;
    DWORD height;
    DWORD width;
    DWORD unk8;
} touch_data_t;

#pragma pack(pop)

enum touch_type {
    TS_DOWN = 1,
    TS_MOVE = 2,
    TS_UP = 3,
};

void *user_data = nullptr;
void (*touch_callback)(
        dev_info_t *dev_info,
        const touch_data_t *touch_data,
        int touch_points,
        int unk1,
        const void *user_data);





void* TouchSDK_Constructor(void* in) {
	return in;
}

bool TouchSDK_SendData(dev_info_t*,
	unsigned char* const, int, unsigned char* const,
	unsigned char* output, int output_size) {

	// fake success
	if (output_size >= 4) {
		output[0] = 0xfc;
		output[1] = 0xa5;
	}
	return true;
}

bool TouchSDK_SetSignalInit(dev_info_t*, int) {
	return true;
}

void TouchSDK_Destructor(void* This) {
}

int TouchSDK_GetYLedTotal(dev_info_t*, int) {
	return 53;
}

int TouchSDK_GetXLedTotal(dev_info_t*, int) {
	return 41;
}

bool TouchSDK_DisableTouch(dev_info_t*, int) {
	return true;
}

bool TouchSDK_DisableDrag(dev_info_t*, int) {
	return true;
}

bool TouchSDK_DisableWheel(dev_info_t*, int) {
	return true;
}

bool TouchSDK_DisableRightClick(dev_info_t*, int) {
	return true;
}

bool TouchSDK_SetMultiTouchMode(dev_info_t*, int) {
	return true;
}

bool TouchSDK_EnableTouchWidthData(dev_info_t*, int) {
	return true;
}

bool TouchSDK_EnableRawData(dev_info_t*, int) {
	return true;
}

bool TouchSDK_SetAllEnable(dev_info_t*, bool, int) {
	return true;
}

int TouchSDK_GetTouchDeviceCount(void* This) {
	return 1;
}

unsigned int TouchSDK_GetTouchSDKVersion(void) {
	return 0x01030307;
}

int TouchSDK_InitTouch(void* This, dev_info_t* devices, int max_devices, void* touch_event_cb,
	void* hotplug_callback, void* userdata) {

	// fake touch device
	memset(devices, 0, sizeof(devices[0].raw));
	devices[0].unk1 = 0x1122;
	devices[0].unk2 = 0x3344;
	devices[0].device_id = 0;
	devices[0].vid = 0xDEAD;
	devices[0].pid = 0xBEEF;
	devices[0].pvn = 0xC0DE;
	devices[0].max_point_num = 16;

	// remember provided callback and userdata
	touch_callback = (decltype(touch_callback))touch_event_cb;
	user_data = userdata;

	// success
	return 1;
}

inline DWORD scale_double_to_xy(double val) {
	return static_cast<DWORD>(val * 32768);
}

inline DWORD scale_double_to_height(double val) {
	return static_cast<DWORD>(val * 1312);
}

inline DWORD scale_double_to_width(double val) {
	return static_cast<DWORD>(val * 1696);
}

void fire_touches(drs_touch_t* events, size_t event_count) {

	// check callback first
	if (!touch_callback) {
		return;
	}

	// generate touch data
	auto game_touches = std::make_unique<touch_data_t[]>(event_count);
	for (size_t i = 0; i < event_count; i++) {

		// initialize touch value
		game_touches[i].cid = (DWORD)events[i].id;
		game_touches[i].unk8 = 0;

		// copy scaled values
		game_touches[i].x = scale_double_to_xy(events[i].x);
		game_touches[i].y = scale_double_to_xy(events[i].y);
		game_touches[i].width = scale_double_to_width(events[i].width);
		game_touches[i].height = scale_double_to_height(events[i].height);

		// decide touch type
		switch (events[i].type) {
		case DRS_DOWN:
			game_touches[i].type = TS_DOWN;
			break;
		case DRS_UP:
			game_touches[i].type = TS_UP;
			break;
		case DRS_MOVE:
			game_touches[i].type = TS_MOVE;
			break;
		default:
			break;
		}
	}

	// build device information
	dev_info_t dev;
	dev.unk1 = 0;
	dev.unk2 = 0;
	dev.device_id = 0;
	dev.vid = 0xDEAD;
	dev.pid = 0xBEEF;
	dev.pvn = 0xC0DE;
	dev.max_point_num = 16;

	// fire callback
	touch_callback(&dev, game_touches.get(), (int)event_count, 0, user_data);
}

VRFoot feet[11];
unsigned long holdcounters[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned long releasecounters[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void start_kinect() {

	if (kinectRunning) return;
	if (!kinectStarted) {
		kinectStarted = true;
		std::thread t([] {
			puts("starting kinect thread");

			// main loop
			while (true) {
				

				if (GetKeyState(VK_SHIFT) & 0x8000)
				{
					feet[0].id = 0;
					feet[0].index = 0;

					// update event details
					feet[0].event.id = feet[0].id;
					feet[0].event.x = 0.5;
					feet[0].event.y = 0.5;
					feet[0].event.width = 1;
					feet[0].event.height = feet[0].event.width;
					holdcounters[0] = 0; // holding down
					releasecounters[0]++;
					
					if (releasecounters[0] == 1) {
						feet[0].event.type = DRS_DOWN;
						fire_touches(&feet[0].event, 1);
					}
					else if (releasecounters[0] > 1) {
						feet[0].event.type = DRS_MOVE;
						fire_touches(&feet[0].event, 1);
					}
				}
				else {
					holdcounters[0]++; // released
					releasecounters[0] == 0;
					feet[0].event.type = DRS_UP;
					if (holdcounters[0] == 1) fire_touches(&feet[0].event, 1); // send up event very shortly
				}
				
				if (GetKeyState(0x31) & 0x8000)
				{
					feet[1].id = 1;
					feet[1].index = 1;

					// update event details
					feet[1].event.id = feet[1].id;
					feet[1].event.x = 0.05;
					feet[1].event.y = 0.5;
					feet[1].event.width = 0.1;
					feet[1].event.height = feet[1].event.width;
					holdcounters[1] = 0; // holding down
					releasecounters[1]++;

					if (releasecounters[1] == 1) {
						feet[1].event.type = DRS_DOWN;
						fire_touches(&feet[1].event, 1);
					}
					else if (releasecounters[1] > 1) {
						feet[1].event.type = DRS_MOVE;
						fire_touches(&feet[1].event, 1);
					}
				}
				else {
					holdcounters[1]++; // released
					releasecounters[1] == 0;
					feet[1].event.type = DRS_UP;
					if (holdcounters[1] == 1) fire_touches(&feet[1].event, 1); // send up event very shortly
				}
				if (GetKeyState(0x32) & 0x8000)
				{
					feet[2].id = 2;
					feet[2].index = 2;

					// update event details
					feet[2].event.id = feet[2].id;
					feet[2].event.x = 0.15;
					feet[2].event.y = 0.5;
					feet[2].event.width = 0.1;
					feet[2].event.height = feet[2].event.width;
					holdcounters[2] = 0; // holding down
					releasecounters[2]++;

					if (releasecounters[2] == 1) {
						feet[2].event.type = DRS_DOWN;
						fire_touches(&feet[2].event, 1);
					}
					else if (releasecounters[2] > 1) {
						feet[2].event.type = DRS_MOVE;
						fire_touches(&feet[2].event, 1);
					}
				}
				else {
					holdcounters[2]++; // released
					releasecounters[2] == 0;
					feet[2].event.type = DRS_UP;
					if (holdcounters[2] == 1) fire_touches(&feet[2].event, 1); // send up event very shortly
				}
				if (GetKeyState(0x33) & 0x8000)
				{
					feet[3].id = 3;
					feet[3].index = 3;

					// update event details
					feet[3].event.id = feet[3].id;
					feet[3].event.x = 0.25;
					feet[3].event.y = 0.5;
					feet[3].event.width = 0.1;
					feet[3].event.height = feet[3].event.width;
					holdcounters[3] = 0; // holding down
					releasecounters[3]++;

					if (releasecounters[3] == 1) {
						feet[3].event.type = DRS_DOWN;
						fire_touches(&feet[3].event, 1);
					}
					else if (releasecounters[3] > 1) {
						feet[3].event.type = DRS_MOVE;
						fire_touches(&feet[3].event, 1);
					}
				}
				else {
					holdcounters[3]++; // released
					releasecounters[3] == 0;
					feet[3].event.type = DRS_UP;
					if (holdcounters[3] == 1) fire_touches(&feet[3].event, 1); // send up event very shortly
				}
				if (GetKeyState(0x34) & 0x8000)
				{
					feet[4].id = 4;
					feet[4].index = 4;

					// update event details
					feet[4].event.id = feet[4].id;
					feet[4].event.x = 0.35;
					feet[4].event.y = 0.5;
					feet[4].event.width = 0.1;
					feet[4].event.height = feet[4].event.width;
					holdcounters[4] = 0; // holding down
					releasecounters[4]++;

					if (releasecounters[4] == 1) {
						feet[4].event.type = DRS_DOWN;
						fire_touches(&feet[4].event, 1);
					}
					else if (releasecounters[4] > 1) {
						feet[4].event.type = DRS_MOVE;
						fire_touches(&feet[4].event, 1);
					}
				}
				else {
					holdcounters[4]++; // released
					releasecounters[4] == 0;
					feet[4].event.type = DRS_UP;
					if (holdcounters[4] == 1) fire_touches(&feet[4].event, 1); // send up event very shortly
				}
				if (GetKeyState(0x35) & 0x8000)
				{
					feet[5].id = 5;
					feet[5].index = 5;

					// update event details
					feet[5].event.id = feet[5].id;
					feet[5].event.x = 0.45;
					feet[5].event.y = 0.5;
					feet[5].event.width = 0.1;
					feet[5].event.height = feet[5].event.width;
					holdcounters[5] = 0; // holding down
					releasecounters[5]++;

					if (releasecounters[5] == 1) {
						feet[5].event.type = DRS_DOWN;
						fire_touches(&feet[5].event, 1);
					}
					else if (releasecounters[5] > 1) {
						feet[5].event.type = DRS_MOVE;
						fire_touches(&feet[5].event, 1);
					}
				}
				else {
					holdcounters[5]++; // released
					releasecounters[5] == 0;
					feet[5].event.type = DRS_UP;
					if (holdcounters[5] == 1) fire_touches(&feet[5].event, 1); // send up event very shortly
				}
				if (GetKeyState(0x36) & 0x8000)
				{
					feet[6].id = 6;
					feet[6].index = 6;

					// update event details
					feet[6].event.id = feet[6].id;
					feet[6].event.x = 0.55;
					feet[6].event.y = 0.5;
					feet[6].event.width = 0.1;
					feet[6].event.height = feet[6].event.width;
					holdcounters[6] = 0; // holding down
					releasecounters[6]++;

					if (releasecounters[6] == 1) {
						feet[6].event.type = DRS_DOWN;
						fire_touches(&feet[6].event, 1);
					}
					else if (releasecounters[6] > 1) {
						feet[6].event.type = DRS_MOVE;
						fire_touches(&feet[6].event, 1);
					}
				}
				else {
					holdcounters[6]++; // released
					releasecounters[6] == 0;
					feet[6].event.type = DRS_UP;
					if (holdcounters[6] == 1) fire_touches(&feet[6].event, 1); // send up event very shortly
				}
				if (GetKeyState(0x37) & 0x8000)
				{
					feet[7].id = 7;
					feet[7].index = 7;

					// update event details
					feet[7].event.id = feet[7].id;
					feet[7].event.x = 0.65;
					feet[7].event.y = 0.5;
					feet[7].event.width = 0.1;
					feet[7].event.height = feet[7].event.width;
					holdcounters[7] = 0; // holding down
					releasecounters[7]++;

					if (releasecounters[7] == 1) {
						feet[7].event.type = DRS_DOWN;
						fire_touches(&feet[7].event, 1);
					}
					else if (releasecounters[7] > 1) {
						feet[7].event.type = DRS_MOVE;
						fire_touches(&feet[7].event, 1);
					}
				}
				else {
					holdcounters[7]++; // released
					releasecounters[7] == 0;
					feet[7].event.type = DRS_UP;
					if (holdcounters[7] == 1) fire_touches(&feet[7].event, 1); // send up event very shortly
				}
				if (GetKeyState(0x38) & 0x8000)
				{
					feet[8].id = 8;
					feet[8].index = 8;

					// update event details
					feet[8].event.id = feet[8].id;
					feet[8].event.x = 0.75;
					feet[8].event.y = 0.5;
					feet[8].event.width = 0.1;
					feet[8].event.height = feet[8].event.width;
					holdcounters[8] = 0; // holding down
					releasecounters[8]++;

					if (releasecounters[8] == 1) {
						feet[8].event.type = DRS_DOWN;
						fire_touches(&feet[8].event, 1);
					}
					else if (releasecounters[8] > 1) {
						feet[8].event.type = DRS_MOVE;
						fire_touches(&feet[8].event, 1);
					}
				}
				else {
					holdcounters[8]++; // released
					releasecounters[8] == 0;
					feet[8].event.type = DRS_UP;
					if (holdcounters[8] == 1) fire_touches(&feet[8].event, 1); // send up event very shortly
				}
				if (GetKeyState(0x39) & 0x8000)
				{
					feet[9].id = 9;
					feet[9].index = 9;

					// update event details
					feet[9].event.id = feet[9].id;
					feet[9].event.x = 0.85;
					feet[9].event.y = 0.5;
					feet[9].event.width = 0.1;
					feet[9].event.height = feet[9].event.width;
					holdcounters[9] = 0; // holding down
					releasecounters[9]++;

					if (releasecounters[9] == 1) {
						feet[9].event.type = DRS_DOWN;
						fire_touches(&feet[9].event, 1);
					}
					else if (releasecounters[9] > 1) {
						feet[9].event.type = DRS_MOVE;
						fire_touches(&feet[9].event, 1);
					}
				}
				else {
					holdcounters[9]++; // released
					releasecounters[9] == 0;
					feet[9].event.type = DRS_UP;
					if (holdcounters[9] == 1) fire_touches(&feet[9].event, 1); // send up event very shortly
				}
				if (GetKeyState(0x30) & 0x8000)
				{
					feet[10].id = 10;
					feet[10].index = 10;

					// update event details
					feet[10].event.id = feet[10].id;
					feet[10].event.x = 0.95;
					feet[10].event.y = 0.5;
					feet[10].event.width = 0.1;
					feet[10].event.height = feet[10].event.width;
					holdcounters[10] = 0; // holding down
					releasecounters[10]++;

					if (releasecounters[10] == 1) {
						feet[10].event.type = DRS_DOWN;
						fire_touches(&feet[10].event, 1);
					}
					else if (releasecounters[10] > 1) {
						feet[10].event.type = DRS_MOVE;
						fire_touches(&feet[10].event, 1);
					}
				}
				else {
					holdcounters[10]++; // released
					releasecounters[10] == 0;
					feet[10].event.type = DRS_UP;
					if (holdcounters[10] == 1) fire_touches(&feet[10].event, 1); // send up event very shortly
				}

				// slow down
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			kinectStarted = false;
			return nullptr;
			});
		t.detach();
	}

}

void hookDancepad() {
	MH_Initialize();
	MH_CreateHookApi(L"TouchSDKDll.dll", "??0TouchSDK@@QEAA@XZ", TouchSDK_Constructor, NULL);
	MH_CreateHookApi(L"TouchSDKDll.dll", "?SendData@TouchSDK@@QEAA_NU_DeviceInfo@@QEAEH1HH@Z", TouchSDK_SendData, NULL);
	MH_CreateHookApi(L"TouchSDKDll.dll", "?SetSignalInit@TouchSDK@@QEAA_NU_DeviceInfo@@H@Z", TouchSDK_SetSignalInit, NULL);
	MH_CreateHookApi(L"TouchSDKDll.dll", "?GetYLedTotal@TouchSDK@@QEAAHU_DeviceInfo@@H@Z", TouchSDK_GetYLedTotal, NULL);
	MH_CreateHookApi(L"TouchSDKDll.dll", "?GetXLedTotal@TouchSDK@@QEAAHU_DeviceInfo@@H@Z", TouchSDK_GetXLedTotal, NULL);
	MH_CreateHookApi(L"TouchSDKDll.dll", "?DisableTouch@TouchSDK@@QEAA_NU_DeviceInfo@@H@Z", TouchSDK_DisableTouch, NULL);
	MH_CreateHookApi(L"TouchSDKDll.dll", "?DisableDrag@TouchSDK@@QEAA_NU_DeviceInfo@@H@Z", TouchSDK_DisableDrag, NULL);
	MH_CreateHookApi(L"TouchSDKDll.dll", "?DisableWheel@TouchSDK@@QEAA_NU_DeviceInfo@@H@Z", TouchSDK_DisableWheel, NULL);
	MH_CreateHookApi(L"TouchSDKDll.dll", "?DisableRightClick@TouchSDK@@QEAA_NU_DeviceInfo@@H@Z", TouchSDK_DisableRightClick, NULL);
	MH_CreateHookApi(L"TouchSDKDll.dll", "?SetMultiTouchMode@TouchSDK@@QEAA_NU_DeviceInfo@@H@Z", TouchSDK_SetMultiTouchMode, NULL);
	MH_CreateHookApi(L"TouchSDKDll.dll", "?EnableTouchWidthData@TouchSDK@@QEAA_NU_DeviceInfo@@H@Z", TouchSDK_EnableTouchWidthData, NULL);
	MH_CreateHookApi(L"TouchSDKDll.dll", "?EnableRawData@TouchSDK@@QEAA_NU_DeviceInfo@@H@Z", TouchSDK_EnableRawData, NULL);
	MH_CreateHookApi(L"TouchSDKDll.dll", "?SetAllEnable@TouchSDK@@QEAA_NU_DeviceInfo@@_NH@Z", TouchSDK_SetAllEnable, NULL);
	MH_CreateHookApi(L"TouchSDKDll.dll", "?GetTouchDeviceCount@TouchSDK@@QEAAHXZ", TouchSDK_GetTouchDeviceCount, NULL);
	MH_CreateHookApi(L"TouchSDKDll.dll", "?GetTouchSDKVersion@TouchSDK@@QEAAIXZ", TouchSDK_GetTouchSDKVersion, NULL);
	MH_CreateHookApi(L"TouchSDKDll.dll", "?InitTouch@TouchSDK@@QEAAHPEAU_DeviceInfo@@HP6AXU2@PEBU_TouchPointData@@HHPEBX@ZP6AX1_N3@ZPEAX@Z", TouchSDK_InitTouch, NULL);

	MH_EnableHook(MH_ALL_HOOKS);

	start_kinect();
}