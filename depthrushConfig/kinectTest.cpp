#include "includes.h"
Vector4 leftLegPosPreview = { 1.5F, 1.5F, 1.5F, 1.5F };
Vector4 rightLegPosPreview = { 1.5F, 1.5F, 1.5F, 1.5F };

int kinectTest(float xGrad, float xOffset, float yGrad, float yOffset, float zGrad, float zOffset) {
	//std::cout << "Kinect Preview\n";
	//std::cin >> xGrad;
	//std::cin >> xOffset;
	// Wait 10 secs for kinect shutdown
	// Sleep(10000);
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

	// main loop to read and process skeleton data
	NUI_SKELETON_FRAME skeletonFrame = { 0 };
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	int width = 0;
	float drsLeft = 0;
	float drsRight = 0;
	int toolLeft = 0;
	int toolRight = 0;
	int toolWidth = 0;
	bool leftTouch = false;
	bool rightTouch = false;

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
				leftLegPosPreview = skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_ANKLE_LEFT];
				rightLegPosPreview = skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_ANKLE_RIGHT];
			}
		}

		// keep updating screen width for fancy
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		width = (int)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
		
		// fetch calibrated feet
		drsLeft = xGrad * leftLegPosPreview.x + xOffset;
		drsRight = xGrad * rightLegPosPreview.x + xOffset;

		// translate to console window width
		toolLeft = width * drsLeft;
		toolRight = width * drsRight;
		toolWidth = width * 0.05;

		// foot lifting detection
		// see how far foot currently is from known ground value
		float fixedLeft = leftLegPosPreview.y - (yGrad * leftLegPosPreview.z + yOffset);
		float fixedRight = rightLegPosPreview.y - (yGrad * rightLegPosPreview.z + yOffset);

		// check for stepping
		float errorMargin = 0.05;
		if (fixedLeft > (fixedRight + errorMargin)) {
			rightTouch = true;
			leftTouch = false;
			// std::cout << "right step\n";
		}
		else if (fixedLeft > (fixedRight + errorMargin)) {
			leftTouch = true;
			rightTouch = false;
			// std::cout << "left step \n";
		}
		else {
			leftTouch = true;
			rightTouch = true;
			// std::cout << "both step\n";
		}
		/*
		// print feet
		for (int i = 0; i < width; i++) {
			if ((i <= (toolLeft + toolWidth)) && (i >= (toolLeft - toolWidth)) && leftTouch) {
				std::cout << "L";
			}
			else if ((i <= (toolRight + toolWidth)) && (i >= (toolRight - toolWidth)) && rightTouch) {
				std::cout << "R";
			}
			else {
				std::cout << " ";
			}
		}
		*/
		std::cout << std::to_string(fixedLeft) << " " << std::to_string(fixedRight) << std::endl;
	}

	// Clean up and exit
	NuiSkeletonTrackingDisable();
	NuiShutdown();
	return 0;
}