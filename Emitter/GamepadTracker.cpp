#include "GamepadTracker.h"
#include <iostream>
#include <format>
#include <hidusage.h>
#include "Console.h"

GamepadTracker::GamepadTracker(int idx) {
    directInput = 0;
    device = 0;
    deviceIndex = idx;
    st = {};
    _isBound = false;
}

bool GamepadTracker::setup() {

    HRESULT hr;
    hr = DirectInput8Create(
        GetModuleHandle(NULL),
        DIRECTINPUT_VERSION,
        IID_IDirectInput8W,
        (void**)&directInput,
        NULL
    );
    if (FAILED(hr)) {
        std::cout << "Could not instantiate IDirectInput8W instance: Error " << hr << std::endl;
        return false;
    }

    // Iterate through devices
    // NOTE: Blocking until it goes through all devices
    hr = directInput->EnumDevices(
        DI8DEVCLASS_GAMECTRL,
        &GamepadTracker::onEnumDevice,
        this,
        DIEDFL_ALLDEVICES
    );
    if (FAILED(hr)) {
        std::cout << "Could not enumerate devices: Error " << hr << std::endl;
        return false;
    }

    auto count = registeredDevices.size();
    
    if (count == 0) {
        Console::error("No game controller found");
        Console::log("Please ensure it is connected before starting the program");
        return false;
    }
    std::cout << std::format("{} game controller(s) found", count) << std::endl;

    if (deviceIndex < 0 || deviceIndex >= count) {
        std::cout << std::format("Gamepad device may only be a number between 0 and {} inclusively", count - 1) << std::endl;
        return false;
    }

    // Register device
    std::cout << std::format("Binding to device at index {}...", deviceIndex) << std::endl;
    hr = directInput->CreateDevice(
        registeredDevices.at(deviceIndex).guidInstance,
        &device,
        NULL
    );
    if (FAILED(hr)) {
        std::cout << "Could not create device object: Error " << hr << std::endl;
        return false;
    }

    hr = device->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(hr)) {
        std::cout << "Could not set data format: Error " << hr << std::endl;
        return false;
    }

    hr = device->SetCooperativeLevel(
        GetDesktopWindow(), // Out of pure laziness. Surely an invisible window is better
        DISCL_BACKGROUND | DISCL_NONEXCLUSIVE
    );
    if (FAILED(hr)) {
        std::cout << "Could not set cooperative level: Error " << hr << std::endl;
        return false;
    }

    hr = device->Acquire();
    if (FAILED(hr)) {
        std::cout << "Could not acquire device: Error " << hr << std::endl;
        return false;
    }

    _isBound = true;

    return true;
}

void GamepadTracker::teardown() {

    if (device == NULL) return;

    device->Unacquire();
    _isBound = false;
}

std::string GamepadTracker::getUdpMessage() {
    if (!device)
        return "";

    device->Poll();

    ZeroMemory(&st, sizeof(DIJOYSTATE2));
    device->GetDeviceState(sizeof(DIJOYSTATE2), &st);

    // Retrieve first 32 buttons
    unsigned int buttonFlags = 0;
    for (short i = 31; i >= 0; i--)
    {
        buttonFlags = buttonFlags << 1;
        buttonFlags = buttonFlags | (bool)(this->st.rgbButtons[i] & 0x80);
    }

    return std::format(
        "{};{};{};{};{};{};{};{};{}",
        PROTOCOL_VERSION,
        buttonFlags,
        this->st.lX,
        this->st.lY,
        this->st.lZ,
        this->st.lRx,
        this->st.lRy,
        this->st.lRz,
        translateDPad(this->st.rgdwPOV[0])
    );
}

BOOL CALLBACK GamepadTracker::onEnumDevice(LPCDIDEVICEINSTANCEW lpddi, LPVOID pvRef) {
    
    GamepadTracker* inst = reinterpret_cast<GamepadTracker*>(pvRef);
    DIDEVICEINSTANCEW di = *lpddi;
    inst->save(di);

    return DIENUM_CONTINUE;
}

void GamepadTracker::save(DIDEVICEINSTANCEW di) {

    registeredDevices.push_back(di);
}

short GamepadTracker::translateDPad(DWORD input) {
    switch (input) {
        case 22500: return 1;
        case 18000: return 2;
        case 13500: return 3;
        case 27000: return 4;
        case 9000: return 6;
        case 31500: return 7;
        case 0: return 8;
        case 4500: return 9;
    }
    return 5;
}
