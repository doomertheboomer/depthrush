#include "includes.h"

int main()
{
    // load config from ini
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

    int choice = 0;
    std::cout << "Depthrush test application\n";
    std::cout << "1. Calibrate Kinect\n";
    std::cout << "2. Preview Kinect\n";
    std::cout << "Enter a choice (1,2): ";
    std::cin >> choice;
    if (choice == 1) {
        calibration();
        return 0;
    }
    else if (choice == 2) {
        kinectTest(xGrad, xOffset, yGrad, yOffset, zGrad, zOffset);
        return 0;
    }
    else {
        std::cout << "Invalid option!";
    }
    return 0;
}