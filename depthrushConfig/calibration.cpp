#include "includes.h"
Vector4 leftLegPos = { 1.5F, 1.5F, 1.5F, 1.5F };
Vector4 rightLegPos = { 1.5F, 1.5F, 1.5F, 1.5F };

int calibration() {
	
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

	// start calibration application
	std::thread calibrateMenu([] {
		std::cout << std::to_string(leftLegPos.x);
		});
	calibrateMenu.detach();

	// main loop to read and process skeleton data
	NUI_SKELETON_FRAME skeletonFrame = { 0 };
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
				leftLegPos = skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_ANKLE_LEFT];
				rightLegPos = skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_ANKLE_RIGHT];
			}
		}
	}

	// Clean up and exit
	NuiSkeletonTrackingDisable();
	NuiShutdown();
	return 0;
}