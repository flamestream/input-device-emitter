// GamepadDebug.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <chrono>
#include "include/cxxopts.hpp"
#include "GamepadTracker.h"

const int DEFAULT_EMISSION_FREQUENCY_MS = 50;
int emissionFrequency = DEFAULT_EMISSION_FREQUENCY_MS;
GamepadTracker* joystickTracker;

void cleanExit(int exitCode = 0) {
    if (!joystickTracker) {
        joystickTracker->teardown();
        delete joystickTracker;
    }
    exit(exitCode);
}

int main(int argc, char* argv[])
{
    cxxopts::Options options("FS Input Emitter", "A simple input device tracker and emitter.");
    options.add_options()
        ("gamepad-index", "Connect to gamepad at specified index", cxxopts::value<int>()->default_value(std::to_string(0)))
        ("f,frequency", "Emission frequency in milliseconds", cxxopts::value<int>()->default_value(std::to_string(DEFAULT_EMISSION_FREQUENCY_MS)))
        ("h,help", "Print usage")
        ;
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    int gamepadIndex = result["gamepad-index"].as<int>();
    joystickTracker = new GamepadTracker(gamepadIndex);
    if (!joystickTracker->setup()) {
        cleanExit(1);
    }

    auto start = std::chrono::steady_clock::now();
    while (true) {
        // Dispatch message at defined frequency
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        if (elapsed > std::chrono::milliseconds(emissionFrequency)) {

            joystickTracker->poll();
            system("cls");

            std::cout << "Connected to Joystick Index " << joystickTracker->deviceIndex << std::endl << std::endl;

            //std::cout << "\033[" << 4 << ";1H";
            std::cout << "lX " << joystickTracker->st.lX << std::endl;
            std::cout << "lY " << joystickTracker->st.lY << std::endl;
            std::cout << "lZ " << joystickTracker->st.lZ << std::endl;
            std::cout << "lRx " << joystickTracker->st.lRx << std::endl;
            std::cout << "lRy " << joystickTracker->st.lRy << std::endl;
            std::cout << "lRz " << joystickTracker->st.lRz << std::endl;
            std::cout << "rglSlider[0] " << joystickTracker->st.rglSlider[0] << std::endl;
            std::cout << "rglSlider[1] " << joystickTracker->st.rglSlider[1] << std::endl;
            std::cout << "rgdwPOV[0] " << joystickTracker->st.rgdwPOV[0] << std::endl;
            std::cout << "rgdwPOV[1] " << joystickTracker->st.rgdwPOV[1] << std::endl;
            std::cout << "rgdwPOV[2] " << joystickTracker->st.rgdwPOV[2] << std::endl;
            std::cout << "rgdwPOV[3] " << joystickTracker->st.rgdwPOV[3] << std::endl;
            //std::cout << "rgbButtons" << joystickTracker->st.rgbButtons[128] << std::endl;
            std::cout << "lVX " << joystickTracker->st.lVX << std::endl;
            std::cout << "lVY " << joystickTracker->st.lVY << std::endl;
            std::cout << "lVZ " << joystickTracker->st.lVZ << std::endl;
            std::cout << "lVRx " << joystickTracker->st.lVRx << std::endl;
            std::cout << "lVRy " << joystickTracker->st.lVRy << std::endl;
            std::cout << "lVRz " << joystickTracker->st.lVRz << std::endl;
            std::cout << "rglVSlider[0] " << joystickTracker->st.rglVSlider[0] << std::endl;
            std::cout << "rglVSlider[1] " << joystickTracker->st.rglVSlider[1] << std::endl;
            std::cout << "lAX " << joystickTracker->st.lAX << std::endl;
            std::cout << "lAY " << joystickTracker->st.lAY << std::endl;
            std::cout << "lAZ " << joystickTracker->st.lAZ << std::endl;
            std::cout << "lARx " << joystickTracker->st.lARx << std::endl;
            std::cout << "lARy " << joystickTracker->st.lARy << std::endl;
            std::cout << "lARz " << joystickTracker->st.lARz << std::endl;
            std::cout << "rglASlider[0] " << joystickTracker->st.rglASlider[0] << std::endl;
            std::cout << "rglASlider[1] " << joystickTracker->st.rglASlider[1] << std::endl;
            std::cout << "lFX " << joystickTracker->st.lFX << std::endl;
            std::cout << "lFY " << joystickTracker->st.lFY << std::endl;
            std::cout << "lFZ " << joystickTracker->st.lFZ << std::endl;
            std::cout << "lFRx " << joystickTracker->st.lFRx << std::endl;
            std::cout << "lFRy " << joystickTracker->st.lFRy << std::endl;
            std::cout << "lFRz " << joystickTracker->st.lFRz << std::endl;
            std::cout << "rglFSlider[0] " << joystickTracker->st.rglFSlider[0] << std::endl;
            std::cout << "rglFSlider[1] " << joystickTracker->st.rglFSlider[1] << std::endl;
            std::cout << std::endl;
            std::cout << "rgbButtons indices: ";
            for (int i = 0; i < 128; ++i) {
                if (joystickTracker->st.rgbButtons[i] & 0x80) {
                    std::cout << "[" << i << "] ";
                }
            }

            start = std::chrono::steady_clock::now();
        }
    }
}