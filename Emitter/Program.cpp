#include <iostream>
#include <chrono>

#include "include/cxxopts.hpp"

#include "MouseTracker.h"
#include "GamepadTracker.h"
#include "Emitter.h"

const int DEFAULT_EMISSION_FREQUENCY_MS = 50;

Emitter* emitter;
Emitter* emitter2;
MouseTracker* mouseTracker;
GamepadTracker* joystickTracker;

bool isPointerWanted;
bool isGamepadWanted;
int emissionFrequency = DEFAULT_EMISSION_FREQUENCY_MS;
bool debug;

void cleanExit(int exitCode = 0) {
    if (!mouseTracker) {
        mouseTracker->teardown();
        delete mouseTracker;
    }
    if (!joystickTracker) {
        joystickTracker->teardown();
        delete joystickTracker;
    }
    if (!emitter) {
        emitter->teardown();
        delete emitter;
    }
    exit(exitCode);
}

bool start() {
    MSG msg;
    auto start = std::chrono::steady_clock::now();
    while (true) {
        // LowLevelMouseProc callback function must be implemented in the same thread that installed the hook.
        // Therefore, you need to have a message loop running in that thread to process the mouse messages.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Dispatch message at defined frequency
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        if (elapsed > std::chrono::milliseconds(emissionFrequency)) {
            if (debug) {
                int skipLines = 2;
                if (isPointerWanted) skipLines += 2;
                if (isGamepadWanted) skipLines += 3;
                std::cout << "\033[" << skipLines << ";1H";
            }
            // Get state
            if (mouseTracker) {
                mouseTracker->refreshState();
                auto out1 = mouseTracker->getUdpMessage();
                if (debug) {
                    std::cout << out1 << "                         " << std::endl;
                    mouseTracker->printDebugData();
                }
                // Broadcast state
                if (!emitter->send(out1)) {
                    return false;
                }
            }

            if (joystickTracker) {
                auto out2 = joystickTracker->getUdpMessage();
                if (debug) {
                    std::cout << out2 << "                         " << std::endl;
                }
                if (!emitter2->send(out2)) {
                    return false;
                }
            }

            //XINPUT_STATE state;
            //for (int i = 0; i < 4; ++i) {
            //    ZeroMemory(&state, sizeof(XINPUT_STATE));
            //    // Simply get the state of the controller from XInput.
            //    auto connected = XInputGetState(i, &state) == ERROR_SUCCESS;
            //    std::cout << "Controller " << i << " connected: " << connected << std::endl;
            //    std::cout << state.dwPacketNumber << std::endl;
            //}
            start = std::chrono::steady_clock::now();
        }

        //if (GetAsyncKeyState(VK_ESCAPE)) {
        //    return true;
        //}
    }
}

int main(int argc, char* argv[]) {
    cxxopts::Options options("FS Input Emitter", "A simple input device tracker and emitter.");
    options.add_options()
        ("P,no-pointer", "Disable pointer emitter")
        ("t,target", "Target IP for pointer emitter", cxxopts::value<std::string>()->default_value(Emitter::DEFAULT_IP_ADDRESS))
        ("p,port", "Pointer emitter port", cxxopts::value<unsigned short>()->default_value(std::to_string(Emitter::DEFAULT_MOUSE_PORT)))
        ("g,gamepad", "Enable gamepad emitter")
        ("gamepad-index", "Connect to gamepad at specified index", cxxopts::value<int>()->default_value(std::to_string(0)))
        ("gamepad-target", "Target IP for pointer emitter", cxxopts::value<std::string>()->default_value(Emitter::DEFAULT_IP_ADDRESS))
        ("gamepad-port", "Gamepad emitter port", cxxopts::value<unsigned short>()->default_value(std::to_string(Emitter::DEFAULT_GAMEPAD_PORT)))
        ("f,frequency", "Emission frequency in milliseconds", cxxopts::value<int>()->default_value(std::to_string(DEFAULT_EMISSION_FREQUENCY_MS)))
        ("d,debug", "Print debug logs")
        ("h,help", "Print usage")
        ;
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    debug = result["debug"].as<bool>();
    if (debug) {
        system("cls");
    }

    isPointerWanted = !result["no-pointer"].as<bool>();
    unsigned short port = result["port"].as<unsigned short>();
    std::string ipAddress = result["target"].as<std::string>();

    isGamepadWanted = result["gamepad"].as<bool>();
    unsigned short portGamepad = result["gamepad-port"].as<unsigned short>();
    std::string ipAddressGamepad = result["gamepad-target"].as<std::string>();

    if (!isPointerWanted && !isGamepadWanted) {
        std::cout << "Please specify a device." << std::endl;
        cleanExit();
    }

    // Setup mouse tracker
    if (isPointerWanted) {
        mouseTracker = new MouseTracker();
        if (!mouseTracker->setup()) {
            cleanExit(1);
        }
    }

    // Setup joystick tracker
    if (isGamepadWanted) {
        int gamepadIndex = result["gamepad-index"].as<int>();
        joystickTracker = new GamepadTracker(gamepadIndex);
        if (!joystickTracker->setup()) {
            cleanExit(1);
        }
    }

    // Setup server
    if (isPointerWanted) {
        emitter = new Emitter(ipAddress, port);
        if (!emitter->setup()) {
            cleanExit(1);
        }
        std::cout << "Broadcasting pointer data (Protocol version " << MouseTracker::PROTOCOL_VERSION << ") to " << emitter->getIpAddress() << ":" << emitter->getPort() << std::endl;
    }

    // Setup server2
    if (isGamepadWanted) {
        emitter2 = new Emitter(ipAddressGamepad, portGamepad);
        if (!emitter2->setup()) {
            cleanExit(1);
        }
        std::cout << "Broadcasting gamepad data (Protocol version " << GamepadTracker::PROTOCOL_VERSION << ") to " << emitter2->getIpAddress() << ":" << emitter2->getPort() << std::endl;
    }

    std::cout << "Press ctrl+C to exit" << std::endl;

    int errCode = 0;
    if (!start()) {
        errCode = 1;
    }
    
    cleanExit(errCode);
}
