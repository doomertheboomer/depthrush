#include "includes.h"

int main()
{
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
        std::cout << "okay 2";
        return 0;
    }
    else {
        std::cout << "Invalid option!";
    }
    return 0;
}