#include "GamepadTracker.h"
#include <iostream>
#include <format>
#include <hidusage.h>

GamepadTracker::GamepadTracker() {
    directInput = 0;
    device = 0;
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
    std::cout << std::format("{} game controller(s) found", count) << std::endl;
    if (count == 0) {
        return true;
    }

    // Register device
    // NOTE: Only first one for now...
    std::cout << "Binding to first device... " << std::endl;
    hr = directInput->CreateDevice(
        registeredDevices.at(0).guidInstance,
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
    if (device) {
        device->Unacquire();
        _isBound = false;
    }
}

std::string GamepadTracker::getUdpMessage() {
    if (!device)
        return "";

    device->Poll();

    ZeroMemory(&st, sizeof(DIJOYSTATE2));
    device->GetDeviceState(sizeof(DIJOYSTATE2), &st);

    // Note: Switch Controller Notations
    return std::format(
        "{};{};{};{};{};{};{};{};{};{};{};{};{};{};{};{};{};{};{};{}",
        PROTOCOL_VERSION,
        this->st.rgbButtons[0]  & 0x80 ? 1 : 0, // B
        this->st.rgbButtons[1]  & 0x80 ? 1 : 0, // A
        this->st.rgbButtons[2]  & 0x80 ? 1 : 0, // Y
        this->st.rgbButtons[3]  & 0x80 ? 1 : 0, // X
        this->st.rgbButtons[4]  & 0x80 ? 1 : 0, // L
        this->st.rgbButtons[5]  & 0x80 ? 1 : 0, // R
        this->st.rgbButtons[6]  & 0x80 ? 1 : 0, // ZL
        this->st.rgbButtons[7]  & 0x80 ? 1 : 0, // ZR
        this->st.rgbButtons[8]  & 0x80 ? 1 : 0, // +
        this->st.rgbButtons[9]  & 0x80 ? 1 : 0, // -
        this->st.rgbButtons[10] & 0x80 ? 1 : 0, // Stick1
        this->st.rgbButtons[11] & 0x80 ? 1 : 0, // Stick2
        this->st.rgbButtons[12] & 0x80 ? 1 : 0, // Home
        this->st.rgbButtons[13] & 0x80 ? 1 : 0, // Screenshot
        this->st.lX, // Left Stick
        this->st.lY,
        this->st.lRx, // Right Stick
        this->st.lRy,
        translateDPad(this->st.rgdwPOV[0]) // D-PAD
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