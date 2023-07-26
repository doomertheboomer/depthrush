#include "drs.h"
#include "includes.h"

#include <windows.h>
#include <thread>

#pragma pack(push)

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

namespace drs {

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
	}
}