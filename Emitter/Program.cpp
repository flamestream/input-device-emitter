#include <iostream>
#include <chrono>

#include "include/cxxopts.hpp"

#include "Console.h"
#include "MouseTracker.h"
#include "GamepadTracker.h"
#include "KeyboardTracker.h"
#include "Emitter.h"
#include "Version.h"

const int DEFAULT_EMISSION_FREQUENCY_MS = 50;

Emitter* emitterPointer;
Emitter* emitterGamepad;
Emitter* emitterKeyboard;
MouseTracker* mouseTracker;
GamepadTracker* gamepadTracker;
KeyboardTracker* keyboardTracker;

bool isPointerWanted;
bool isGamepadWanted;
bool isKeyboardWanted;
int emissionFrequency = DEFAULT_EMISSION_FREQUENCY_MS;
bool debug;

void cleanExit(int exitCode = 0) {
    if (mouseTracker) {
        mouseTracker->teardown();
        delete mouseTracker;
    }
    if (gamepadTracker) {
        gamepadTracker->teardown();
        delete gamepadTracker;
    }
    if (keyboardTracker) {
        keyboardTracker->teardown();
        delete keyboardTracker;
    }

    if (emitterPointer) {
        emitterPointer->teardown();
        delete emitterPointer;
    }
    if (emitterGamepad) {
        emitterGamepad->teardown();
        delete emitterGamepad;
    }
    if (emitterKeyboard) {
        emitterKeyboard->teardown();
        delete emitterKeyboard;
    }

    Console::info("Program has ended execution.\nTo exit, press ctrl-c or close the terminal");
    std::cin.get();

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
                //if (isPointerWanted) skipLines += 2;
                //if (isGamepadWanted) skipLines += 3;
                //if (isKeyboardWanted) skipLines += 3;
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
                if (!emitterPointer->send(out1)) {
                    return false;
                }
            }

            if (gamepadTracker) {
                auto out2 = gamepadTracker->getUdpMessage();
                if (debug) {
                    std::cout << std::endl << out2 << "                         " << std::endl;
                }
                if (!emitterGamepad->send(out2)) {
                    return false;
                }
            }

            if (keyboardTracker) {
                keyboardTracker->refreshState();
                auto out1 = keyboardTracker->getUdpMessage();
                if (debug) {
                    std::cout << std::endl << out1 << "                         " << std::endl;
                }
                // Broadcast state
                if (!emitterKeyboard->send(out1)) {
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

std::wstring GetExecutableName() {
    WCHAR buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);

    std::wstring fullPath(buffer);
    size_t pos = fullPath.find_last_of(L"\\/");
    std::wstring fileName = fullPath.substr(pos + 1);

    return fileName;
}

std::string ConvertWStringToString(const std::wstring& wstr) {
    if (wstr.empty()) {
        return std::string();
    }

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), &str[0], size_needed, NULL, NULL);
    return str;
}

int main(int argc, char* argv[]) {
    Console::saveState();

    std::string executableName = ConvertWStringToString(GetExecutableName());
    std::string version = VERSION_STRING;
    std::string description = "FS Input Emitter " + version + "\n"
                              "========================\n"
                              "A simple input device tracker and emitter.\n";
    
    cxxopts::Options options(executableName, description);
    options.add_options()
        ("P,no-pointer", "Disable pointer emitter")
        ("t,target", "Target IP for pointer emitter", cxxopts::value<std::string>()->default_value(Emitter::DEFAULT_IP_ADDRESS))
        ("p,port", "Pointer emitter port", cxxopts::value<unsigned short>()->default_value(std::to_string(Emitter::DEFAULT_MOUSE_PORT)))
        ("g,gamepad", "Enable gamepad emitter")
        ("gamepad-index", "Connect to gamepad at specified index", cxxopts::value<int>()->default_value(std::to_string(0)))
        ("gamepad-target", "Target IP for gamepad emitter", cxxopts::value<std::string>()->default_value(Emitter::DEFAULT_IP_ADDRESS))
        ("gamepad-port", "Gamepad emitter port", cxxopts::value<unsigned short>()->default_value(std::to_string(Emitter::DEFAULT_GAMEPAD_PORT)))
        ("k,keyboard", "Enable keyboard emitter")
        ("keyboard-target", "Target IP for keyboard emitter", cxxopts::value<std::string>()->default_value(Emitter::DEFAULT_IP_ADDRESS))
        ("keyboard-port", "Keyboard emitter port", cxxopts::value<unsigned short>()->default_value(std::to_string(Emitter::DEFAULT_KEYBOARD_PORT)))
        ("f,frequency", "Emission frequency in milliseconds", cxxopts::value<int>()->default_value(std::to_string(DEFAULT_EMISSION_FREQUENCY_MS)))
        ("v,version", "Print version number")
        ("d,debug", "Print debug logs")
        ("h,help", "Print usage")
        ;
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    if (result.count("version")) {
        std::cout << version << std::endl;
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

    isKeyboardWanted = result["keyboard"].as<bool>();
    unsigned short portKeyboard = result["keyboard-port"].as<unsigned short>();
    std::string ipAddressKeyboard = result["keyboard-target"].as<std::string>();

    if (!isPointerWanted && !isGamepadWanted && !isKeyboardWanted) {
        std::cout << "Please specify a device to track." << std::endl;
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
        gamepadTracker = new GamepadTracker(gamepadIndex);
        if (!gamepadTracker->setup()) {
            cleanExit(1);
        }
    }

    // Setup keyboard tracker
    if (isKeyboardWanted) {
        keyboardTracker = new KeyboardTracker();
        if (!keyboardTracker->setup()) {
            cleanExit(1);
        }
    }

    // Setup pointer server
    if (isPointerWanted) {
        emitterPointer = new Emitter(ipAddress, port);
        if (!emitterPointer->setup()) {
            cleanExit(1);
        }
        std::cout << "Broadcasting pointer data (Protocol version " << MouseTracker::PROTOCOL_VERSION << ") to " << emitterPointer->getIpAddress() << ":" << emitterPointer->getPort() << std::endl;
    }

    // Setup gamepad server
    if (isGamepadWanted) {
        emitterGamepad = new Emitter(ipAddressGamepad, portGamepad);
        if (!emitterGamepad->setup()) {
            cleanExit(1);
        }
        std::cout << "Broadcasting gamepad data (Protocol version " << GamepadTracker::PROTOCOL_VERSION << ") to " << emitterGamepad->getIpAddress() << ":" << emitterGamepad->getPort() << std::endl;
    }

    // Setup keynoard server
    if (isKeyboardWanted) {
        emitterKeyboard = new Emitter(ipAddressKeyboard, portKeyboard);
        if (!emitterKeyboard->setup()) {
            cleanExit(1);
        }
        std::cout << "Broadcasting keyboard data (Protocol version " << KeyboardTracker::PROTOCOL_VERSION << ") to " << emitterKeyboard->getIpAddress() << ":" << emitterKeyboard->getPort() << std::endl;
    }

    int errCode = 0;
    if (!start()) {
        errCode = 1;
    }
    
    cleanExit(errCode);
}
