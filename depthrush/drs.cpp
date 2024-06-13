#pragma pack(push)
#include <windows.h>
#include <thread>
#include "includes.h"

bool kinectRunning = false;
bool kinectStarted = false;
bool threadsCreated = false;
bool mainInitialized = false;

bool isDown = false;
bool isJump = false;

bool downHack = true;
bool jumpHack = true;

VRFoot feet[16];


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


std::uint8_t* PatternScan(void* module, const char* signature)
{
	static auto pattern_to_byte = [](const char* pattern) {
		auto bytes = std::vector<int>{};
		auto start = const_cast<char*>(pattern);
		auto end = const_cast<char*>(pattern) + strlen(pattern);

		for (auto current = start; current < end; ++current) {
			if (*current == '?') {
				++current;
				if (*current == '?')
					++current;
				bytes.push_back(-1);
			}
			else {
				bytes.push_back(strtoul(current, &current, 16));
			}
		}
		return bytes;
	};

	auto dosHeader = (PIMAGE_DOS_HEADER)module;
	auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)module + dosHeader->e_lfanew);

	auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
	auto patternBytes = pattern_to_byte(signature);
	auto scanBytes = reinterpret_cast<std::uint8_t*>(module);

	auto s = patternBytes.size();
	auto d = patternBytes.data();

	for (auto i = 0ul; i < sizeOfImage - s; ++i) {
		bool found = true;
		for (auto j = 0ul; j < s; ++j) {
			if (scanBytes[i + j] != d[j] && d[j] != -1) {
				found = false;
				break;
			}
		}
		if (found) {
			return &scanBytes[i];
		}
	}
	return nullptr;
}


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

DWORD pollKinect(LPVOID _) {
	/*
	std::thread t([] {

		});
	t.detach();
	*/

	// initialize Kinect
	HRESULT hr = NuiInitialize(NUI_INITIALIZE_FLAG_USES_SKELETON);
	if (FAILED(hr)) {
		std::cout << "Failed to initialize Kinect." << std::endl;
		return 1;
	}

	// open the skeleton stream
	HANDLE skeletonStream = nullptr;
	hr = NuiSkeletonTrackingEnable(nullptr, 0);
	if (FAILED(hr)) {
		std::cout << "Failed to open the skeleton stream." << std::endl;
		NuiShutdown();
		return 1;
	}

	// load config from ini
	std::cout << "Loading calibration from depthrush.ini\n";
	mINI::INIFile file("depthrush.ini");
	mINI::INIStructure ini;
	file.read(ini);
	std::string& readValue = ini["calibration"]["xGrad"];
	float xGrad = std::stof(readValue);
	readValue = ini["calibration"]["xOffset"];
	float xOffset = std::stof(readValue);
	readValue = ini["calibration"]["yGrad"];
	float yGrad = std::stof(readValue);
	readValue = ini["calibration"]["yOffset"];
	float yOffset = std::stof(readValue);
	readValue = ini["calibration"]["zGrad"];
	float zGrad = std::stof(readValue);
	readValue = ini["calibration"]["zOffset"];
	float zOffset = std::stof(readValue);
	readValue = ini["calibration"]["errorMargin"];
	float errorMargin = std::stof(readValue); // stepping error margin
	readValue = ini["calibration"]["jumpHeight"];
	float jumpHeight = std::stof(readValue); // jumping error margin


	// main loop to read and process skeleton data
	NUI_SKELETON_FRAME skeletonFrame = { 0 };
	// for down & jumps
	float joints[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	float prevJoints[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	float highestDown = 0;
	std::chrono::steady_clock::time_point jumpTime;
	std::chrono::steady_clock::time_point landTime;

	while (true) {
		// get the latest skeleton frame
		hr = NuiSkeletonGetNextFrame(0, &skeletonFrame);
		if (FAILED(hr)) {
			continue;
		}

		// Process each tracked skeleton
		for (int i = 0; i < NUI_SKELETON_COUNT; ++i) {
			if (skeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED) {
				// get the position of both legs
				Vector4 leftLegPos = skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_ANKLE_LEFT];
				Vector4 rightLegPos = skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_ANKLE_RIGHT];

				// clamp
				feet[1].event.x = std::clamp((xGrad * leftLegPos.x + xOffset), 0.0f, 1.0f);
				feet[1].event.y = std::clamp((zGrad * leftLegPos.z + zOffset), 0.0f, 1.0f);
				feet[2].event.x = std::clamp((xGrad * rightLegPos.x + xOffset), 0.0f, 1.0f);
				feet[2].event.y = std::clamp((zGrad * rightLegPos.z + zOffset), 0.0f, 1.0f);

				// Fix feet height
				float fixedLeft = leftLegPos.y - (yGrad * leftLegPos.z + yOffset);
				float fixedRight = rightLegPos.y - (yGrad * rightLegPos.z + yOffset);

				// check for stepping if not jumping
				if (isJump == false) {
					if (fixedLeft > (fixedRight + errorMargin)) {
						feet[2].touching = true;
						feet[1].touching = false;
						// std::cout << "right step\n";
					}
					else if (fixedRight > (fixedLeft + errorMargin)) {
						feet[1].touching = true;
						feet[2].touching = false;
						// std::cout << "left step \n";
					}
					else {
						feet[1].touching = true;
						feet[2].touching = true;
						// std::cout << "both step\n";
					}
				}
				
				// y axis tracking (jump and down)
				for (int j = 0; j < 20; j++) { // get current coords
					joints[j] = skeletonFrame.SkeletonData[i].SkeletonPositions[j].y;
				}

				// get highest down value
				for (int j = 0; j < 20; j++) {
					if (joints[j] - prevJoints[j] < highestDown) {
						highestDown = joints[j];
					}
				}
				// down handling
				if (highestDown < -0.2) {
					isDown = true;
				}
				else {
					isDown = false;
				}

				// jump handling
				if (fixedLeft > jumpHeight && fixedRight > jumpHeight && isJump == false) {
					jumpTime = std::chrono::high_resolution_clock::now();
					isJump = true;
				}
				if (isJump) {
					landTime = std::chrono::high_resolution_clock::now();
					feet[1].touching = false;
					feet[2].touching = false;
					// jump for 180ms
					if (std::chrono::duration_cast<std::chrono::milliseconds>(landTime - jumpTime).count() > 180) {
						isJump = false;
					}
				}


				// save current coords as previous coord
				for (int j = 0; j < 20; j++) { 
					prevJoints[j] = skeletonFrame.SkeletonData[i].SkeletonPositions[j].y;
				}

				
				highestDown = 0;
			}
		}
	}

	// Clean up and exit
	NuiSkeletonTrackingDisable();
	NuiShutdown();
	return 0;
}

DWORD startInputSpam(LPVOID _) {
	/*
	std::thread t([] {

		});
	t.detach();
	*/
	puts("starting input spam thread");

	// temporarily hardcode both kinect feet to touching at size 0.1
	feet[1].touching = true;
	feet[1].id = 1;
	feet[1].index = 1;
	feet[1].event.id = feet[1].id;
	feet[1].event.width = 0.1;
	feet[1].event.height = feet[1].event.width;

	feet[2].touching = true;
	feet[2].id = 2;
	feet[2].index = 2;
	feet[2].event.id = feet[2].id;
	feet[2].event.width = 0.1;
	feet[2].event.height = feet[2].event.width;

	// hardcode debug foot details
	feet[0].id = 0;
	feet[0].index = 0;

	// update event details
	feet[0].event.id = feet[0].id;
	feet[0].event.x = 0.5;
	feet[0].event.y = 0.5;
	feet[0].event.width = 1;
	feet[0].event.height = feet[0].event.width;

	// main loop
	while (true) {
		// debug shift control to touch entire pad
		if (GetKeyState(VK_SHIFT) & 0x8000)
		{

			// check previous event
			switch (feet[0].event.type) {
			case DRS_UP:

				// generate down event

				feet[0].event.type = DRS_DOWN;
				break;

			case DRS_DOWN:
			case DRS_MOVE:

				// generate move event
				feet[0].event.type = DRS_MOVE;
				break;

			default:
				break;
			}
			// send event
			fire_touches(&feet[0].event, 1);
		}
		else {

			switch (feet[0].event.type) {
			case DRS_DOWN:
			case DRS_MOVE:
				// generate up event
				feet[0].event.type = DRS_UP;
				fire_touches(&feet[0].event, 1);
				break;
			case DRS_UP:
			default:
				break;
			}
		}
		if (feet[1].touching) // check if foot1 touch
		{
			// check previous event
			switch (feet[1].event.type) {
			case DRS_UP:
				// generate down event
				feet[1].event.type = DRS_DOWN;
				break;
			case DRS_DOWN:
			case DRS_MOVE:
				// generate move event
				//puts("foot1 is moving");
				feet[1].event.type = DRS_MOVE;
				break;
			default:
				break;
			}
			// send event
			fire_touches(&feet[1].event, 1);
		}
		else {
			switch (feet[1].event.type) {
			case DRS_DOWN:
			case DRS_MOVE:
				// generate up event
				feet[1].event.type = DRS_UP;
				fire_touches(&feet[1].event, 1);
				break;
			case DRS_UP:
			default:
				break;
			}
		}
		if (feet[2].touching) // check if foot2 touch
		{
			// check previous event
			switch (feet[2].event.type) {
			case DRS_UP:
				// generate down event
				feet[2].event.type = DRS_DOWN;
				break;
			case DRS_DOWN:
			case DRS_MOVE:
				// generate move event
				feet[2].event.type = DRS_MOVE;
				break;
			default:
				break;
			}
			// send event
			fire_touches(&feet[2].event, 1);
		}
		else {
			switch (feet[2].event.type) {
			case DRS_DOWN:
			case DRS_MOVE:
				// generate up event
				feet[2].event.type = DRS_UP;
				fire_touches(&feet[2].event, 1);
				break;
			case DRS_UP:
			default:
				break;
			}
		}
		// slow down
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	kinectStarted = false;
	return 0;

}

typedef __int64(__fastcall* PinitializeJudgementMemory)(float a1, float a2, char a3, __int64 a4, __int64 a5, char a6, float* a7);
static PinitializeJudgementMemory Orig_initializeJudgementMemory;

__int64 __fastcall Hook_initializeJudgementMemory(float a1, float a2, char a3, __int64 a4, __int64 a5, char a6, float* a7) {
	// a4 is a1 + 168 usually
	__int64 tmp = a4;
	uintptr_t ptr = (uintptr_t)tmp - 16;

	int noteType = *(int*)ptr;
	float tMinus = fabs(a2 - a1);
	if (noteType == 3) {
		if (tMinus <= 150 && isDown && downHack) { // 150ms timing window for downs
			std::cout << "Hitting down!\n";
			return 5;
		}
	}
	if (noteType == 4) {
		if (tMinus <= 200 && isJump && jumpHack) { // 200ms timing window for jump
			std::cout << "Hitting jump!\n";
			return 5;
		}
	}

	return Orig_initializeJudgementMemory(a1, a2, a3, a4, a5, a6, a7);
}




void hookDancepad() {
	if (!mainInitialized) {
		MH_Initialize();

		// Pad hooks
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
		
		std::cout << "Loading settings from depthrush.ini\n";
		mINI::INIFile file("depthrush.ini");
		mINI::INIStructure ini;
		file.read(ini);
		std::string& readValue = ini["hacks"]["downHack"];
		downHack = !strcmp(readValue.c_str(), "1");
		std::string& readValue2 = ini["hacks"]["jumpHack"];
		jumpHack = !strcmp(readValue2.c_str(), "1");

		// Game hooks
		// uintptr_t imageBase = 0x180000000;
		// HOOK(0x1806D5180, initializeJudgementMemory);
		if (downHack == true || jumpHack == true) {
			// if hacks are enabled
			std::cout << "Hacks enabled, hooking!\n";
			auto pointer = PatternScan((void*)0x180000000, "40 53 48 83 EC 20 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 18 48 8B 5C 24 60 0F 28 E0 0F 28 D9");
			HOOK(pointer, initializeJudgementMemory);
		}
		MH_EnableHook(MH_ALL_HOOKS);
		mainInitialized = true;
	}
	

	

	if (threadsCreated == false) {
		CreateThread(NULL, 0, startInputSpam, NULL, 0, NULL);
		CreateThread(NULL, 0, pollKinect, NULL, 0, NULL);
		threadsCreated = true;
	}
	

	/*
	startInputSpam(); // spams input to the game 
	pollKinect();
	*/
	return;
}