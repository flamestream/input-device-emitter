#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <format>

#include "include/cxxopts.hpp"
#include "include/inquirer.h"

#include "Console.h"
#include "MouseTracker.h"
#include "DirectInputTracker.h"
#include "KeyboardTracker.h"
#include "GameInputTracker.h"
#include "Emitter.h"
#include "Version.h"
#include "Utility.h"

const int DEFAULT_EMISSION_FREQUENCY_MS = 50;

const std::string INQUIRE_TYPE_POINTER = "Pointer (Mouse & Pen)";
const std::string INQUIRE_TYPE_GAMEINPUT = "Controller device using GameInput (Xbox One, Switch Pro, etc.)";
const std::string INQUIRE_TYPE_KEYBOARD = "Keyboard";
const std::string INQUIRE_TYPE_DIRECTINPUT = "Generic Input device using DirectInput (Wheel, pedals, joystick, etc.)";
const std::string INQUIRE_TYPE_NO_MORE = "No. All done";

Emitter* emitterPointer;
Emitter* emitterGamepad;
Emitter* emitterKeyboard;
Emitter* emitterGameInputGamepad;
MouseTracker* mouseTracker;
DirectInputTracker* directInputTracker;
KeyboardTracker* keyboardTracker;
GameInputTracker* gameInputTracker;

bool isPointerWanted;
bool isDirectInputWanted;
bool isKeyboardWanted;
bool isGameInputWanted;

bool gameInputErrorDisplayed = false;

std::string ipAddressPointer = Emitter::DEFAULT_IP_ADDRESS;
std::string ipAddressDirectInput = Emitter::DEFAULT_IP_ADDRESS;
std::string ipAddressKeyboard = Emitter::DEFAULT_IP_ADDRESS;
std::string ipAddressGameInput = Emitter::DEFAULT_IP_ADDRESS;

unsigned short portPointer = Emitter::DEFAULT_POINTER_PORT;
unsigned short portDirectInput = Emitter::DEFAULT_DIRECTINPUT_PORT;
unsigned short portKeyboard = Emitter::DEFAULT_KEYBOARD_PORT;
unsigned short portGameInput = Emitter::DEFAULT_GAMEINPUT_PORT;

int directInputDeviceIndex = 0;

int emissionFrequency = DEFAULT_EMISSION_FREQUENCY_MS;
bool isDebugWanted = false;
bool isInteractiveMode = false;
bool isShutdownMessageWanted = false;

static bool isConsoleApp() {
    DWORD consoleMode;
    HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) {
        return false;
    }
    return GetConsoleMode(hConsole, &consoleMode);
}

static void cleanExit(int exitCode = 0) {
    if (mouseTracker) {
        mouseTracker->teardown();
        delete mouseTracker;
    }
    if (directInputTracker) {
        directInputTracker->teardown();
        delete directInputTracker;
    }
    if (keyboardTracker) {
        keyboardTracker->teardown();
        delete keyboardTracker;
    }
    if (gameInputTracker) {
        gameInputTracker->teardown();
        delete gameInputTracker;
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
    if (emitterGameInputGamepad) {
        emitterGameInputGamepad->teardown();
        delete emitterGameInputGamepad;
    }

    if (isShutdownMessageWanted) {
        Console::info("\nProgram has ended execution.\nTo exit, press enter or close the terminal");
        std::cin.get();
    }

    exit(exitCode);
}

static void startInteractiveMode() {

    std::string version = VERSION_STRING;
    auto heading = std::format("FS Input Emitter {} Interactive Mode", version);
    Console::log(heading); // Not sure why it's not printed by below statement, so we print it ourselves

    auto inquirer = alx::Inquirer(heading);
    while (true) {

        Console::log("");

        auto choices = std::vector<std::string>{};
        auto isTrackingSomething = isPointerWanted || isDirectInputWanted || isKeyboardWanted || isGameInputWanted;
        if (isTrackingSomething) choices.push_back(INQUIRE_TYPE_NO_MORE);
        if (!isPointerWanted) choices.push_back(INQUIRE_TYPE_POINTER);
        if (!isGameInputWanted) choices.push_back(INQUIRE_TYPE_GAMEINPUT);
        if (!isKeyboardWanted) choices.push_back(INQUIRE_TYPE_KEYBOARD);
        choices.push_back(INQUIRE_TYPE_DIRECTINPUT);

        auto answer = inquirer.add_question(
            {
                "type",
                isTrackingSomething
                    ? "Any other device to track?"
                    : "Select device to track (Use arrow keys to navigate choices)",
                choices,
            }
        ).ask();

        if (answer == INQUIRE_TYPE_POINTER) {
            isPointerWanted = true;
            answer = inquirer.add_question(
                {
                    "pointer-target",
                    std::format("Send pointer data to [{}:{}]?", ipAddressPointer, portPointer),
                    alx::Type::yesNo
                }
            ).ask();
            if (answer == "no") {
                answer = inquirer.add_question(
                    {
                        "target",
                        std::format("Enter new IP address [{}]:", ipAddressPointer),
                    }
                    ).ask();
                if (answer != "") {
                    ipAddressPointer = answer;
                }
                answer = inquirer.add_question(
                    {
                        "port",
                        std::format("Enter new port [{}]:", portPointer),
                        alx::Type::integer
                    }
                ).ask();
                if (answer != "") {
                    portPointer = std::stoi(answer);
                }
            }

        }
        else if (answer == INQUIRE_TYPE_GAMEINPUT) {

            isGameInputWanted = true;
            answer = inquirer.add_question(
                {
                    "gameinput-target",
                    std::format("Send GameInput data to [{}:{}]?", ipAddressGameInput, portGameInput),
                    alx::Type::yesNo
                }
            ).ask();
            if (answer == "no") {
                answer = inquirer.add_question(
                    {
                        "gameinput-target",
                        std::format("Enter new IP address [{}]:", ipAddressGameInput),
                    }
                    ).ask();
                if (answer != "") {
                    ipAddressGameInput = answer;
                }
                answer = inquirer.add_question(
                    {
                        "gameinput-port",
                        std::format("Enter new port [{}]:", portGameInput),
                        alx::Type::integer
                    }
                ).ask();
                if (answer != "") {
                    portGameInput = std::stoi(answer);
                }
            }

        }
        else if (answer == INQUIRE_TYPE_KEYBOARD) {

            isKeyboardWanted = true;
            answer = inquirer.add_question(
                {
                    "keyboard-target",
                    std::format("Send keyboard data to [{}:{}]?", ipAddressKeyboard, portKeyboard),
                    alx::Type::yesNo
                }
            ).ask();
            if (answer == "no") {
                answer = inquirer.add_question(
                    {
                        "keyboard-target",
                        std::format("Enter new IP address [{}]:", ipAddressKeyboard),
                    }
                    ).ask();
                if (answer != "") {
                    ipAddressKeyboard = answer;
                }
                answer = inquirer.add_question(
                    {
                        "keyboard-port",
                        std::format("Enter new port [{}]:", portKeyboard),
                        alx::Type::integer
                    }
                ).ask();
                if (answer != "") {
                    portKeyboard = std::stoi(answer);
                }
            }

        }
        else if (answer == INQUIRE_TYPE_DIRECTINPUT) {

            if (isDirectInputWanted) {
                Console::error("Only one DirectInput device can be tracked at a time per program instance.");
                Console::log("To track additional devices, run another instance of FS Input Emitter");
                Console::warn("Ensure that a different port is used to emit its signal!");
                continue;
            }
            isDirectInputWanted = true;

            if (!directInputTracker) {
                directInputTracker = new DirectInputTracker();
            }
            if (!directInputTracker->setup()) {
                Console::error("Failed to initialize DirectInput tracker. Try again.");
                isDirectInputWanted = false;
                continue;
            }

            if (directInputTracker->isSetup()) {

                std::vector<std::string> deviceChoices;
                if (!directInputTracker->getDeviceChoices(deviceChoices)) {
                    Console::error(std::format("Failed to list devices: {}", directInputTracker->lastError));
                    isDirectInputWanted = false;
                    continue;
                }
                if (deviceChoices.empty()) {
                    Console::error("No device found. Ensure it's connected and try again.");
                    isDirectInputWanted = false;
                    continue;
                }

                answer = inquirer.add_question(
                    {
                        "type",
                        "Choose device to bind to",
                        deviceChoices,
                    }
                    ).ask();
                auto it = std::find(deviceChoices.begin(), deviceChoices.end(), answer);
                if (it == deviceChoices.end()) {
                    isDirectInputWanted = false;
                    continue;
                }
                directInputDeviceIndex = std::distance(deviceChoices.begin(), it);

                answer = inquirer.add_question(
                    {
                        "directinput-target",
                        std::format("Send DirectInput data to [{}:{}]?", ipAddressDirectInput, portDirectInput),
                        alx::Type::yesNo
                    }
                ).ask();
                if (answer == "no") {
                    answer = inquirer.add_question(
                        {
                            "directinput-target",
                            std::format("Enter new IP address [{}]:", ipAddressDirectInput),
                        }
                        ).ask();
                    if (answer != "") {
                        ipAddressDirectInput = answer;
                    }
                    answer = inquirer.add_question(
                        {
                            "directinput-port",
                            std::format("Enter new port [{}]:", portDirectInput),
                            alx::Type::integer
                        }
                    ).ask();
                    if (answer != "") {
                        portDirectInput = std::stoi(answer);
                    }
                }
            }

        }
        else if (answer == INQUIRE_TYPE_NO_MORE) {
            Console::log("");
            break;
        }
    }
}

static bool start() {
    MSG msg;
    auto wantedInterval = std::chrono::milliseconds(emissionFrequency);
    auto start = std::chrono::steady_clock::now();
    auto end = start - wantedInterval;

    while (true) {

        start = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(start - end);

        if (mouseTracker || keyboardTracker) {
            // LowLevelMouseProc callback function must be implemented in the same thread that installed the hook.
            // Therefore, you need to have a message loop running in that thread to process the mouse messages.
            // NOTE: It may not be throttled!
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        } else {
            // Sleep optimization
            std::this_thread::sleep_for(wantedInterval - elapsed);
            elapsed = wantedInterval;
        }

        // Dispatch message at defined frequency
        if (elapsed >= wantedInterval) {

            if (isDebugWanted) {
                int skipLines = 0;
                if (mouseTracker) skipLines += 1;
                if (directInputTracker) skipLines += 2;
                if (keyboardTracker) skipLines += 1;
                if (gameInputTracker) skipLines += 2;

                std::cout << "\033[" << (skipLines + 2) << ";1H";
            }

            // Track mouse
            if (mouseTracker) {
                mouseTracker->refreshState();
                auto out = mouseTracker->getUdpMessage();
                if (isDebugWanted) {
                    Console::log(out);
                    Console::log(mouseTracker->getExtraDebugData());
                    Console::log("");
                }
                // Broadcast state
                if (!emitterPointer->send(out)) {
                    return false;
                }
            }

            // Track DirectInput devices
            if (directInputTracker) {
                if (!directInputTracker->refreshState()) {
                    Console::error(std::format("DirectInput tracker could not read state: {}", directInputTracker->lastError));
                    return false;
                }
                if (isDebugWanted) {
                    Console::log(directInputTracker->state);
                    Console::log("");
                }
                if (!emitterGamepad->send(directInputTracker->state)) {
                    return false;
                }
            }

            // Track keyboard
            if (keyboardTracker) {
                keyboardTracker->refreshState();
                if (isDebugWanted) {
                    Console::log(keyboardTracker->state);
                    Console::log("");
                }
                // Broadcast state
                if (!emitterKeyboard->send(keyboardTracker->state)) {
                    return false;
                }
            }

            // Track GameInput devices
            if (gameInputTracker) {
                if (!gameInputTracker->refreshState()) {
                    if (isDebugWanted || !gameInputErrorDisplayed) {
                        Console::error(std::format("Failed to get GameInput reading: {}", gameInputTracker->lastError));
                        Console::error("To fix this, you may need to reconnect the device");
                        gameInputErrorDisplayed = true;
                    }
                } else if (gameInputErrorDisplayed) {
                    Console::success("GameInput has detected a device");
                    gameInputErrorDisplayed = false;
                }
                if (isDebugWanted) {
                    Console::log(gameInputTracker->state);
                    Console::log("");
                }
                // Broadcast state
                if (!emitterGameInputGamepad->send(gameInputTracker->state)) {
                    return false;
                }
            }

            if (isDebugWanted) {
                // Buffer lines
                Console::log("");
            }

            end = std::chrono::steady_clock::now();
        }
    }
}

static std::wstring GetExecutableName() {
    WCHAR buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);

    std::wstring fullPath(buffer);
    size_t pos = fullPath.find_last_of(L"\\/");
    std::wstring fileName = fullPath.substr(pos + 1);

    return fileName;
}

int main(int argc, char* argv[]) {
    Console::saveAttributes();

    std::string executableName = Utility::wstr2str(GetExecutableName());
    std::string version = VERSION_STRING;
    std::string description = "FS Input Emitter " + version + "\n"
                              "========================\n"
                              "A simple input device tracker and emitter.\n";

    cxxopts::Options options(executableName, description);
    options.add_options()
        ("p,pointer", "Enable pointer emitter")
        ("pointer-target", "Target IP for pointer emitter", cxxopts::value<std::string>()->default_value(Emitter::DEFAULT_IP_ADDRESS))
        ("pointer-port", "Pointer emitter port", cxxopts::value<unsigned short>()->default_value(std::to_string(Emitter::DEFAULT_POINTER_PORT)))
        ("d,directinput", "Enable gamepad emitter")
        ("directinput-index", "Connect to DirectInput at specified index", cxxopts::value<int>()->default_value(std::to_string(0)))
        ("directinput-target", "Target IP for DirectInput emitter", cxxopts::value<std::string>()->default_value(Emitter::DEFAULT_IP_ADDRESS))
        ("directinput-port", "DirectInput emitter port", cxxopts::value<unsigned short>()->default_value(std::to_string(Emitter::DEFAULT_DIRECTINPUT_PORT)))
        ("k,keyboard", "Enable keyboard emitter")
        ("keyboard-target", "Target IP for keyboard emitter", cxxopts::value<std::string>()->default_value(Emitter::DEFAULT_IP_ADDRESS))
        ("keyboard-port", "Keyboard emitter port", cxxopts::value<unsigned short>()->default_value(std::to_string(Emitter::DEFAULT_KEYBOARD_PORT)))
        ("g,gameinput", "Enable GameInput emitter")
        ("gameinput-target", "Target IP for GameInput emitter", cxxopts::value<std::string>()->default_value(Emitter::DEFAULT_IP_ADDRESS))
        ("gameinput-port", "GameInput emitter port", cxxopts::value<unsigned short>()->default_value(std::to_string(Emitter::DEFAULT_GAMEINPUT_PORT)))
        ("f,frequency", "Emission frequency in milliseconds", cxxopts::value<int>()->default_value(std::to_string(DEFAULT_EMISSION_FREQUENCY_MS)))
        ("show-shutdown-message", "Pause execution before shutting down")
        ("v,version", "Print version number")
        ("x,debug", "Print debug logs")
        ("h,help", "Print usage")
        ;
    
    try {
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            Console::log(options.help());
            exit(0);
        }

        if (result.count("version")) {
            Console::log(version);
            exit(0);
        }

        isDebugWanted = result["debug"].as<bool>();
        if (isDebugWanted) {
            system("cls");
        }

        isShutdownMessageWanted = result["show-shutdown-message"].as<bool>() || !isConsoleApp();

        isInteractiveMode = argc == 1;
        if (isInteractiveMode) {
        
            startInteractiveMode();
        
        } else {

            isPointerWanted = result["pointer"].as<bool>();
            portPointer = result["pointer-port"].as<unsigned short>();
            ipAddressPointer = result["pointer-target"].as<std::string>();

            isDirectInputWanted = result["directinput"].as<bool>();
            portDirectInput = result["directinput-port"].as<unsigned short>();
            ipAddressDirectInput = result["directinput-target"].as<std::string>();
            directInputDeviceIndex = result["directinput-index"].as<int>();

            isKeyboardWanted = result["keyboard"].as<bool>();
            portKeyboard = result["keyboard-port"].as<unsigned short>();
            ipAddressKeyboard = result["keyboard-target"].as<std::string>();

            isGameInputWanted = result["gameinput"].as<bool>();
            portGameInput = result["gameinput-port"].as<unsigned short>();
            ipAddressGameInput = result["gameinput-target"].as<std::string>();

            emissionFrequency = result["frequency"].as<int>();
        }

    } catch (const std::exception& e) {

        Console::error(std::format("Error parsing options: {}", e.what()));
        exit(1);
    }

    if (!isPointerWanted && !isDirectInputWanted && !isKeyboardWanted && !isGameInputWanted) {
        Console::log("Please specify a device to track.");
        cleanExit();
    }

    // Setup mouse tracker
    if (isPointerWanted) {
        mouseTracker = new MouseTracker();
        if (!mouseTracker->setup()) {
            Console::error(std::format("Could not instantiate pointer tracker: {}", mouseTracker->lastError));
            cleanExit(1);
        }
    }

    // Setup DirectInput tracker
    if (isDirectInputWanted) {
        if (!directInputTracker) {
            directInputTracker = new DirectInputTracker();
        }
        if (!directInputTracker->setup()) {
            Console::error(std::format("Could not instantiate DirectInput tracker: {}", directInputTracker->lastError));
            cleanExit(1);
        }
        if (!directInputTracker->bind(directInputDeviceIndex)) {
            Console::error(std::format("DirectInput tracker could bind to device at index {}: {}", directInputDeviceIndex, directInputTracker->lastError));
            cleanExit(1);
        }
    }

    // Setup keyboard tracker
    if (isKeyboardWanted) {
        keyboardTracker = new KeyboardTracker();
        if (!keyboardTracker->setup()) {
            Console::error(std::format("Could not instantiate keyboard tracker: {}", keyboardTracker->lastError));
            cleanExit(1);
        }
    }

    // Setup GameInput gamepad tracker
    if (isGameInputWanted) {
        gameInputTracker = new GameInputTracker();
        if (!gameInputTracker->setup()) {
            Console::error(std::format("Could not instantiate GameInput tracker: {}", gameInputTracker->lastError));
            cleanExit(1);
        }
    }

    // Setup pointer server
    // Setup pointer server
    if (isPointerWanted) {
        emitterPointer = new Emitter(ipAddressPointer, portPointer);
        if (!emitterPointer->setup()) {
            cleanExit(1);
        }
        Console::log(std::format("Broadcasting pointer data (Protocol version {}) to {}:{}", MouseTracker::PROTOCOL_VERSION, emitterPointer->getIpAddress(), emitterPointer->getPort()));
    }

    // Setup DirectInput server
    if (isDirectInputWanted) {
        emitterGamepad = new Emitter(ipAddressDirectInput, portDirectInput);
        if (!emitterGamepad->setup()) {
            cleanExit(1);
        }
        Console::log(std::format("Broadcasting DirectInput data (Protocol version {}) to {}:{}", DirectInputTracker::PROTOCOL_VERSION, emitterGamepad->getIpAddress(), emitterGamepad->getPort()));
        Console::warn("NOTE: If the device gets disconnected, the program would need to be restarted to bind to it again");
    }

    // Setup keyboard server
    if (isKeyboardWanted) {
        emitterKeyboard = new Emitter(ipAddressKeyboard, portKeyboard);
        if (!emitterKeyboard->setup()) {
            cleanExit(1);
        }
        Console::log(std::format("Broadcasting keyboard data (Protocol version {}) to {}:{}", KeyboardTracker::PROTOCOL_VERSION, emitterKeyboard->getIpAddress(), emitterKeyboard->getPort()));
    }

    // Setup GameInput server
    if (isGameInputWanted) {
        emitterGameInputGamepad = new Emitter(ipAddressGameInput, portGameInput);
        if (!emitterGameInputGamepad->setup()) {
            cleanExit(1);
        }
        Console::log(std::format("Broadcasting GameInput data (Protocol version {}) to {}:{}", GameInputTracker::PROTOCOL_VERSION, emitterGameInputGamepad->getIpAddress(), emitterGameInputGamepad->getPort()));
        Console::warn("NOTE: You may need to reconnect the device for it to be trackable");
    }

    // Compute command
    if (isInteractiveMode) {

        std::string longFlags = "--show-shutdown-message";
        if (ipAddressPointer != Emitter::DEFAULT_IP_ADDRESS) {
            longFlags += std::format(" --pointer-target {}", ipAddressPointer);
        }
        if (portPointer != Emitter::DEFAULT_POINTER_PORT) {
            longFlags += std::format(" --pointer-port {}", portPointer);
        }

        if (ipAddressGameInput != Emitter::DEFAULT_IP_ADDRESS) {
            longFlags += std::format(" --gameinput-target {}", ipAddressGameInput);
        }
        if (portGameInput != Emitter::DEFAULT_GAMEINPUT_PORT) {
            longFlags += std::format(" --gameinput-port {}", portGameInput);
        }

        if (ipAddressKeyboard != Emitter::DEFAULT_IP_ADDRESS) {
            longFlags += std::format(" --gameinput-target {}", ipAddressKeyboard);
        }
        if (portKeyboard != Emitter::DEFAULT_KEYBOARD_PORT) {
            longFlags += std::format(" --gameinput-port {}", portKeyboard);
        }

        if (ipAddressDirectInput != Emitter::DEFAULT_IP_ADDRESS) {
            longFlags += std::format(" --directinput-target {}", ipAddressDirectInput);
        }
        if (portDirectInput != Emitter::DEFAULT_DIRECTINPUT_PORT) {
            longFlags += std::format(" --directinput-port {}", portDirectInput);
        }

        std::string flags = "-";
        if (isPointerWanted) {
            flags += "p";
        }
        if (isGameInputWanted) {
            flags += "g";
        }
        if (isKeyboardWanted) {
            flags += "k";
        }
        if (isDirectInputWanted) {
            flags += "d";
            longFlags += std::format(" --directinput-index {}", directInputDeviceIndex);
        }

        std::string ipAddressPointer = Emitter::DEFAULT_IP_ADDRESS;
        std::string ipAddressDirectInput = Emitter::DEFAULT_IP_ADDRESS;
        std::string ipAddressKeyboard = Emitter::DEFAULT_IP_ADDRESS;
        std::string ipAddressGameInput = Emitter::DEFAULT_IP_ADDRESS;

        unsigned short portPointer = Emitter::DEFAULT_POINTER_PORT;
        unsigned short portDirectInput = Emitter::DEFAULT_DIRECTINPUT_PORT;
        unsigned short portKeyboard = Emitter::DEFAULT_KEYBOARD_PORT;
        unsigned short portGameInput = Emitter::DEFAULT_GAMEINPUT_PORT;

        std::string args;
        if (!flags.empty()) {
            args += flags;
        }
        if (longFlags.size()) {
            args += " " + longFlags;
        }

        Console::log("");
        Console::log("Interactive Mode completed. You may skip it by restarting the program with the following flags");
        Console::log("");
        Console::info(std::format("    {} {}", executableName, args));
        Console::log("");
    }

    int errCode = 0;
    if (!start()) {
        errCode = 1;
    }


    cleanExit(errCode);
}
