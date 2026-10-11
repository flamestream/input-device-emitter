#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <format>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")

#include "include/cxxopts.hpp"
#include "include/inquirer.h"

#include "Console.h"
#include "MouseTracker.h"
#include "DirectInputTracker.h"
#include "KeyboardTracker.h"
#include "GameInputTracker.h"
#include "XInputTracker.h"
#include "Emitter.h"
#include "Version.h"
#include "Utility.h"

const int DEFAULT_EMISSION_FREQUENCY_MS = 30;

const std::string INQUIRE_TYPE_POINTER = "Pointer (Mouse & Pen)";
const std::string INQUIRE_TYPE_KEYBOARD = "Keyboard";
const std::string INQUIRE_TYPE_DIRECTINPUT = "Generic Input device using DirectInput";
const std::string INQUIRE_TYPE_XINPUT = "Controller device using XInput (Most controllers)";
const std::string INQUIRE_TYPE_GAMEINPUT = "Controller device using GameInput (Experimental)";
const std::string INQUIRE_TYPE_NO_MORE = "No. All done";

const std::string INQUIRE_GAMEINPUT_CHANNEL_GAMEPAD = "High-level Gamepad Interface (For Xbox Series S|X, etc.)";
const std::string INQUIRE_GAMEINPUT_CHANNEL_CONTROLLER = "Low-level Generic Controller Interface (For Switch Pro, etc.)";

Emitter* emitterPointer;
Emitter* emitterDirectInput;
Emitter* emitterKeyboard;
Emitter* emitterGameInputGamepad;
Emitter* emitterXInput;
MouseTracker* mouseTracker;
DirectInputTracker* directInputTracker;
KeyboardTracker* keyboardTracker;
GameInputTracker* gameInputTracker;
XInputTracker* xInputTracker;

bool isPointerWanted;
bool isDirectInputWanted;
bool isKeyboardWanted;
bool isGameInputWanted;
bool isXInputWanted;
bool isGameInputLowLevelChannelWanted;

bool gameInputErrorDisplayed = false;
bool xInputErrorDisplayed = false;
bool directInputErrorDisplayed = false;

std::string ipAddressPointer = Emitter::DEFAULT_IP_ADDRESS;
std::string ipAddressDirectInput = Emitter::DEFAULT_IP_ADDRESS;
std::string ipAddressKeyboard = Emitter::DEFAULT_IP_ADDRESS;
std::string ipAddressGameInput = Emitter::DEFAULT_IP_ADDRESS;
std::string ipAddressXInput = Emitter::DEFAULT_IP_ADDRESS;

unsigned short portPointer = Emitter::DEFAULT_POINTER_PORT;
unsigned short portDirectInput = Emitter::DEFAULT_DIRECTINPUT_PORT;
unsigned short portKeyboard = Emitter::DEFAULT_KEYBOARD_PORT;
unsigned short portGameInput = Emitter::DEFAULT_GAMEINPUT_PORT;
unsigned short portXInput = Emitter::DEFAULT_XINPUT_PORT;

int directInputDeviceIndex = 0;
DWORD xInputControllerIndex = 0;

int emissionFrequency = DEFAULT_EMISSION_FREQUENCY_MS;
bool isDebugWanted = false;
bool isInteractiveMode = false;
bool isShutdownMessageWanted = false;

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
    if (xInputTracker) {
        xInputTracker->teardown();
        delete xInputTracker;
    }

    if (emitterPointer) {
        emitterPointer->teardown();
        delete emitterPointer;
    }
    if (emitterDirectInput) {
        emitterDirectInput->teardown();
        delete emitterDirectInput;
    }
    if (emitterKeyboard) {
        emitterKeyboard->teardown();
        delete emitterKeyboard;
    }
    if (emitterGameInputGamepad) {
        emitterGameInputGamepad->teardown();
        delete emitterGameInputGamepad;
    }
    if (emitterXInput) {
        emitterXInput->teardown();
        delete emitterXInput;
    }

    Emitter::cleanupWinsock();
    timeEndPeriod(1);

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
        auto isTrackingSomething = isPointerWanted || isKeyboardWanted || isDirectInputWanted || isXInputWanted || isGameInputWanted;
        if (isTrackingSomething) choices.push_back(INQUIRE_TYPE_NO_MORE);
        if (!isPointerWanted) choices.push_back(INQUIRE_TYPE_POINTER);
        if (!isKeyboardWanted) choices.push_back(INQUIRE_TYPE_KEYBOARD);
        choices.push_back(INQUIRE_TYPE_DIRECTINPUT);
        choices.push_back(INQUIRE_TYPE_XINPUT);
        if (!isGameInputWanted) choices.push_back(INQUIRE_TYPE_GAMEINPUT);

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
                    "type",
                    "Which channel to read?",
                    std::vector<std::string> {
                        INQUIRE_GAMEINPUT_CHANNEL_GAMEPAD,
                        INQUIRE_GAMEINPUT_CHANNEL_CONTROLLER
                    }
                }
            ).ask();
            if (answer == INQUIRE_GAMEINPUT_CHANNEL_CONTROLLER) {
                isGameInputLowLevelChannelWanted = true;
            }

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
        else if (answer == INQUIRE_TYPE_XINPUT) {

            if (isXInputWanted) {
                Console::error("Only one XInput controller can be tracked at a time per program instance.");
                Console::log("To track additional devices, run another instance of FS Input Emitter");
                Console::warn("Ensure that a different port is used to emit its signal!");
                continue;
            }
            isXInputWanted = true;

            while (true) {
                std::vector<std::string> controllerChoices;
                XInputTracker::getControllerChoices(controllerChoices);
                controllerChoices.push_back("Refresh list...");

                answer = inquirer.add_question(
                    {
                        "xinput-device",
                        "Choose XInput controller to bind to",
                        controllerChoices,
                    }
                ).ask();

                auto it = std::find(controllerChoices.begin(), controllerChoices.end(), answer);
                if (it != controllerChoices.end()) {
                    auto selectedIndex = std::distance(controllerChoices.begin(), it);

                    if (selectedIndex == static_cast<int>(controllerChoices.size() - 1)) {
                        continue;
                    }

                    xInputControllerIndex = static_cast<DWORD>(selectedIndex);
                    break;
                }
            }

            answer = inquirer.add_question(
                {
                    "xinput-target",
                    std::format("Send XInput data to [{}:{}]?", ipAddressXInput, portXInput),
                    alx::Type::yesNo
                }
            ).ask();
            if (answer == "no") {
                answer = inquirer.add_question(
                    {
                        "xinput-target",
                        std::format("Enter new IP address [{}]:", ipAddressXInput),
                    }
                    ).ask();
                if (answer != "") {
                    ipAddressXInput = answer;
                }
                answer = inquirer.add_question(
                    {
                        "xinput-port",
                        std::format("Enter new port [{}]:", portXInput),
                        alx::Type::integer
                    }
                ).ask();
                if (answer != "") {
                    portXInput = std::stoi(answer);
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
    auto lastEmission = std::chrono::steady_clock::now() - wantedInterval;

    while (true) {

        if (mouseTracker || keyboardTracker) {
            // Drain all pending messages so the shared hook state holds the freshest input snapshot
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastEmission);

        if (elapsed < wantedInterval) {
            auto remaining = wantedInterval - elapsed;
            DWORD waitMs = static_cast<DWORD>(remaining.count());
            if (waitMs > 0) {
                if (mouseTracker || keyboardTracker) {
                    // Sleep efficiently with 0% CPU; wakes up in <0.01ms if an input message arrives
                    MsgWaitForMultipleObjectsEx(0, NULL, waitMs, QS_ALLINPUT, MWMO_ALERTABLE);
                } else {
                    std::this_thread::sleep_for(remaining);
                }
            }
            continue;
        }

        lastEmission = now;

            if (isDebugWanted) {
                int skipLines = 0;
                if (mouseTracker) skipLines += 1;
                if (keyboardTracker) skipLines += 1;
                if (directInputTracker) skipLines += 1;
                if (xInputTracker) skipLines += 1;
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

            // Track DirectInput devices
            if (directInputTracker) {
                directInputTracker->refreshState(); // Always succeeds now

                // Check binding status
                if (!directInputTracker->isBound()) {
                    if (isDebugWanted || !directInputErrorDisplayed) {
                        Console::error(std::format("DirectInput has failed to get reading at index {}: {}", directInputDeviceIndex, directInputTracker->lastError));
                        directInputErrorDisplayed = true;
                    }
                } else if (directInputErrorDisplayed) {
                    Console::success(std::format("DirectInput has detected a device at index {}", directInputDeviceIndex));
                    directInputErrorDisplayed = false;
                }

                if (isDebugWanted) {
                    Console::log(directInputTracker->state);
                    Console::log("");
                }
                if (!emitterDirectInput->send(directInputTracker->state)) {
                    return false;
                }
            }

            // Track XInput devices
            if (xInputTracker) {
                xInputTracker->refreshState();

                if (!xInputTracker->isControllerConnected()) {
                    if (isDebugWanted || !xInputErrorDisplayed) {
                        Console::error(std::format("XInput has failed to get reading at index {}: {}", xInputControllerIndex, xInputTracker->lastError));
                        xInputErrorDisplayed = true;
                    }
                } else if (xInputErrorDisplayed) {
                    Console::success(std::format("XInput has detected a controller at index {}", xInputControllerIndex));
                    xInputErrorDisplayed = false;
                }

                if (isDebugWanted) {
                    Console::log(xInputTracker->state);
                    Console::log("");
                }
                // Broadcast state (even if disconnected, sends empty/default state)
                if (!emitterXInput->send(xInputTracker->state)) {
                    return false;
                }
            }

            // Track GameInput devices
            if (gameInputTracker) {
                if (!gameInputTracker->refreshState()) {
                    if (isDebugWanted || !gameInputErrorDisplayed) {
                        Console::error(std::format("GameInput has failed to get reading: {}", gameInputTracker->lastError));
                        Console::error("To fix this, you may need to reconnect the controller");
                        gameInputErrorDisplayed = true;
                    }
                }
                else if (gameInputErrorDisplayed) {
                    Console::success("GameInput has detected a controller");
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
    timeBeginPeriod(1);
    if (!Emitter::initWinsock()) {
        Console::error("Failed to initialize Winsock");
        return 1;
    }
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
        ("g,gameinput", "Enable GameInput emitter (Experimental)")
        ("gameinput-target", "Target IP for GameInput emitter", cxxopts::value<std::string>()->default_value(Emitter::DEFAULT_IP_ADDRESS))
        ("gameinput-port", "GameInput emitter port", cxxopts::value<unsigned short>()->default_value(std::to_string(Emitter::DEFAULT_GAMEINPUT_PORT)))
        ("gameinput-channel-low", "Read GameInput low-level channel signals")
        ("x,xinput", "Enable XInput emitter")
        ("xinput-index", "XInput controller index (0-3)", cxxopts::value<int>()->default_value(std::to_string(0)))
        ("xinput-target", "Target IP for XInput emitter", cxxopts::value<std::string>()->default_value(Emitter::DEFAULT_IP_ADDRESS))
        ("xinput-port", "XInput emitter port", cxxopts::value<unsigned short>()->default_value(std::to_string(Emitter::DEFAULT_XINPUT_PORT)))
        ("f,frequency", "Period between emissions in milliseconds. Lower is faster", cxxopts::value<int>()->default_value(std::to_string(DEFAULT_EMISSION_FREQUENCY_MS)))
        ("show-shutdown-message", "Pause execution before shutting down")
        ("v,version", "Print version number")
        ("z,debug", "Print debug logs")
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

        isShutdownMessageWanted = result["show-shutdown-message"].as<bool>();

        isInteractiveMode = argc == 1;
        if (isInteractiveMode) {

            startInteractiveMode();
            isShutdownMessageWanted = true;

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
            isGameInputLowLevelChannelWanted = result["gameinput-channel-low"].as<bool>();

            isXInputWanted = result["xinput"].as<bool>();
            portXInput = result["xinput-port"].as<unsigned short>();
            ipAddressXInput = result["xinput-target"].as<std::string>();
            xInputControllerIndex = static_cast<DWORD>(result["xinput-index"].as<int>());

            emissionFrequency = result["frequency"].as<int>();
        }

    } catch (const std::exception& e) {

        Console::error(std::format("Error parsing options: {}", e.what()));
        exit(1);
    }

    if (!isPointerWanted && !isDirectInputWanted && !isKeyboardWanted && !isGameInputWanted && !isXInputWanted) {
        Console::log("Please specify a device to track.");
        cleanExit();
    }

    // Setup mouse tracker
    if (isPointerWanted) {
        mouseTracker = new MouseTracker();
        if (!mouseTracker->setup()) {
            Console::error(std::format(
                "Could not instantiate pointer tracker: {}",
                mouseTracker->lastError
            ));
            cleanExit(1);
        }
    }

    // Setup DirectInput tracker
    if (isDirectInputWanted) {
        if (!directInputTracker) {
            directInputTracker = new DirectInputTracker();
        }
        if (!directInputTracker->setup()) {
            Console::error(std::format(
                "DirectInput tracker could not instantiate: {}",
                directInputTracker->lastError
            ));
            cleanExit(1);
        }
        if (!directInputTracker->bind(directInputDeviceIndex)) {
            Console::error(std::format(
                "DirectInput tracker could not bind to device at index {}: {}",
                directInputDeviceIndex,
                directInputTracker->lastError
            ));
            cleanExit(1);
        }
    }

    // Setup keyboard tracker
    if (isKeyboardWanted) {
        keyboardTracker = new KeyboardTracker();
        if (!keyboardTracker->setup()) {
            Console::error(std::format(
                "Keyboard tracker could not instantiate: {}",
                keyboardTracker->lastError
            ));
            cleanExit(1);
        }
    }

    // Setup GameInput tracker
    if (isGameInputWanted) {
        gameInputTracker = new GameInputTracker(isGameInputLowLevelChannelWanted);
        if (!gameInputTracker->setup()) {
            Console::error(std::format(
                "GameInput tracker could not instantiate: {}",
                gameInputTracker->lastError
            ));
            cleanExit(1);
        }
    }

    // Setup XInput tracker
    if (isXInputWanted) {
        xInputTracker = new XInputTracker(xInputControllerIndex);
        if (!xInputTracker->setup()) {
            Console::error(std::format(
                "XInput tracker could not instantiate: {}",
                xInputTracker->lastError
            ));
            cleanExit(1);
        }
    }

    // Setup pointer server
    if (isPointerWanted) {
        emitterPointer = new Emitter(ipAddressPointer, portPointer);
        if (!emitterPointer->setup()) {
            cleanExit(1);
        }
        Console::log(std::format(
            "Pointer data being broadcasted (Protocol version {}) to {}:{}",
            MouseTracker::PROTOCOL_VERSION,
            emitterPointer->getIpAddress(),
            emitterPointer->getPort()
        ));
    }

    // Setup keyboard server
    if (isKeyboardWanted) {
        emitterKeyboard = new Emitter(ipAddressKeyboard, portKeyboard);
        if (!emitterKeyboard->setup()) {
            cleanExit(1);
        }
        Console::log(std::format(
            "Keyboard data being broadcasted (Protocol version {}) to {}:{}",
            KeyboardTracker::PROTOCOL_VERSION,
            emitterKeyboard->getIpAddress(),
            emitterKeyboard->getPort()
        ));
    }

    // Setup DirectInput server
    if (isDirectInputWanted) {
        emitterDirectInput = new Emitter(ipAddressDirectInput, portDirectInput);
        if (!emitterDirectInput->setup()) {
            cleanExit(1);
        }
        Console::log(std::format(
            "DirectInput data (index {}) being broadcasted (Protocol version {}) to {}:{}",
            directInputDeviceIndex,
            DirectInputTracker::PROTOCOL_VERSION,
            emitterDirectInput->getIpAddress(),
            emitterDirectInput->getPort()
        ));
    }

    // Setup XInput server
    if (isXInputWanted) {
        emitterXInput = new Emitter(ipAddressXInput, portXInput);
        if (!emitterXInput->setup()) {
            cleanExit(1);
        }
        Console::log(std::format(
            "XInput data (index {}) being broadcasted (Protocol version {}) to {}:{}",
            xInputControllerIndex,
            XInputTracker::PROTOCOL_VERSION,
            emitterXInput->getIpAddress(),
            emitterXInput->getPort()
        ));
    }

    // Setup GameInput server
    if (isGameInputWanted) {
        emitterGameInputGamepad = new Emitter(ipAddressGameInput, portGameInput);
        if (!emitterGameInputGamepad->setup()) {
            cleanExit(1);
        }
        Console::log(std::format(
            "GameInput data ({}) being broadcasted (Protocol version {}) to {}:{}",
            isGameInputLowLevelChannelWanted ? "low-level generic controller" : "high-level gamepad",
            GameInputTracker::PROTOCOL_VERSION,
            emitterGameInputGamepad->getIpAddress(),
            emitterGameInputGamepad->getPort()
        ));
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

        if (ipAddressDirectInput != Emitter::DEFAULT_IP_ADDRESS) {
            longFlags += std::format(" --directinput-target {}", ipAddressDirectInput);
        }
        if (portDirectInput != Emitter::DEFAULT_DIRECTINPUT_PORT) {
            longFlags += std::format(" --directinput-port {}", portDirectInput);
        }

        if (ipAddressXInput != Emitter::DEFAULT_IP_ADDRESS) {
            longFlags += std::format(" --xinput-target {}", ipAddressXInput);
        }
        if (portXInput != Emitter::DEFAULT_XINPUT_PORT) {
            longFlags += std::format(" --xinput-port {}", portXInput);
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
        if (isGameInputLowLevelChannelWanted) {
            longFlags += std::format(" --gameinput-channel-low", portKeyboard);
        }

        std::string flags = "-";
        if (isPointerWanted) {
            flags += "p";
        }
        if (isKeyboardWanted) {
            flags += "k";
        }
        if (isDirectInputWanted) {
            flags += "d";
            longFlags += std::format(" --directinput-index {}", directInputDeviceIndex);
        }
        if (isXInputWanted) {
            flags += "x";
            longFlags += std::format(" --xinput-index {}", xInputControllerIndex);
        }
        if (isGameInputWanted) {
            flags += "g";
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
