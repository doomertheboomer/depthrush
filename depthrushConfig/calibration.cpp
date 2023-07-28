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
		std::cout << "Starting calibration... Make sure your kinect is pointed straight ahead at you!\n";

		// X calibration
		std::cout << "Please place your left foot on the left side of the pad for 5 seconds.\n";
		Sleep(5000);
		float xMin = leftLegPos.x;
		std::cout << "Left side recorded.\n";
		std::cout << "Please place your right foot on the right side of the pad for 5 seconds.\n";
		Sleep(5000);
		float xMax = rightLegPos.x;
		std::cout << "Right side recorded.\n";
		float xGrad = (1 - 0) / (xMax - xMin);
		float xOffset = -(xGrad * xMin);

		// Y and Z calibration

		// Z calibration
		std::cout << "Please place your left foot at the front of the pad for 5 seconds.\n";
		Sleep(5000);
		float zMin = leftLegPos.z;
		float yMin = leftLegPos.y;
		std::cout << "Front recorded.\n";
		std::cout << "Please place your left foot at the back of the pad for 5 seconds.\n";
		Sleep(5000);
		float zMax = leftLegPos.z;
		float yMax = leftLegPos.y;
		std::cout << "Back recorded.\n";
		float zGrad = (1 - 0) / (zMax - zMin);
		float zOffset = -(zGrad * zMin);
		
		//Y calibration
		float yGrad = (yMax - yMin) / (zMax - zMin);
		float yOffset = (yMin - yGrad * zMin);
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