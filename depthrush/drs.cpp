#pragma pack(push)
#include <windows.h>
#include <thread>
#include "includes.h"

bool kinectRunning = false;
bool kinectStarted = false;

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

DWORD depthrushWritePort(HANDLE port, char data[], unsigned length)
{
	DWORD numWritten = 0;

	OVERLAPPED ol = { 0, 0, 0, 0, NULL };
	ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	BOOL status = WriteFile(port, data, length, &numWritten, &ol);
	DWORD xferBytes = 0;

	if (!status)
	{
		switch (GetLastError())
		{
		case ERROR_SUCCESS:
			break;
		case ERROR_IO_PENDING:
			// Wait for 16ms
			if (WaitForSingleObject(ol.hEvent, 16) == WAIT_OBJECT_0)
			{
				status = GetOverlappedResult(port, &ol, &xferBytes, FALSE);
			}
			else
			{
				CancelIo(port);
			}
			break;
		}
	}

	CloseHandle(ol.hEvent);

	FlushFileBuffers(port);
	return numWritten;
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

DWORD depthrushTouchThread(HANDLE port)
{
	char fileBuf[32];
	puts("starting serial touch thread");

	DWORD times = 0;

	for (;;)
	{
		DWORD bytesRead = 0;
		memset(fileBuf, 0, 32);

		OVERLAPPED ol = { 0, 0, 0, 0, NULL };
		BOOL ret = 0;
		ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		BOOL rfResult = ReadFile(port, fileBuf, 32, &bytesRead, &ol);
		DWORD xferBytes = 0;

		if (!rfResult)
		{
			switch (GetLastError())
			{
			case ERROR_SUCCESS:
				break;
			case ERROR_IO_PENDING:
				// Wait for 16ms
				if (WaitForSingleObject(ol.hEvent, 16) == WAIT_OBJECT_0)
				{
					rfResult = GetOverlappedResult(port, &ol, &xferBytes, FALSE);
				}
				else
				{
					CancelIo(port);
				}
				break;
			}
		}

		CloseHandle(ol.hEvent);

		if (xferBytes > 0)
		{
			printf("IN: xferred %d bytes\n", xferBytes);
		}

		if (bytesRead > 0)
		{
			printf("Read %d bytes: ", bytesRead);
			for (unsigned x = 0; x < bytesRead; x++)
			{
				printf("%02X ", fileBuf[x]);
			}
			printf("\n");

			BOOL packetRecognised = FALSE;

			if (!packetRecognised)
			{
				puts("unknown packet, responding with OK");
				depthrushWritePort(port, (char*)"1", 1);
			}
		}
		Sleep(16);
	}
}

DWORD depthrushNamedPipeServer(LPVOID _)
{
	puts("init depthrush pipe server");

	HANDLE pipe = CreateNamedPipeW(
		L"\\\\.\\pipe\\depthrush-api",
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_BYTE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		255,
		255,
		25,
		NULL
	);

	if (!pipe)
	{
		puts("named pipe creation failed!");
		return 1;
	}

	BOOL connected = ConnectNamedPipe(pipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

	if (connected)
	{
	puts("client connection established, spawning thread");

	DWORD tid = 0;
	CreateThread(NULL, 0, depthrushTouchThread, pipe, 0, &tid);
	printf("thread spawned, tid=%d\n", tid);
	}

	return 0;
}

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
					continue;
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
	CreateThread(NULL, 0, depthrushNamedPipeServer, NULL, 0, NULL);

	start_kinect();
}