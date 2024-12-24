#include "GamepadTracker.h"
#include <format>
#include <hidusage.h>
#include <atlstr.h>

#include "Utility.h"

GamepadTracker::GamepadTracker() {
    this->directInput = 0;
    this->device = 0;
    this->st = {};
    this->buttonFlags = 0;
    this->_isSetup = false;

    writeState();
}

bool GamepadTracker::setup() {

    auto hr = DirectInput8Create(
        GetModuleHandle(NULL),
        DIRECTINPUT_VERSION,
        IID_IDirectInput8W,
        (void**)&directInput,
        NULL
    );
    if (FAILED(hr)) {
        lastError = Utility::hr2str(hr);
        return false;
    }

    _isSetup = true;
    return true;
}

bool GamepadTracker::getDeviceChoices(std::vector<std::string> &devices) {

    if (!directInput) {
        lastError = "Not initialized";
        return false;
    }

    auto hr = directInput->EnumDevices(
        DI8DEVCLASS_GAMECTRL,
        &GamepadTracker::onEnumDevice,
        this,
        DIEDFL_ALLDEVICES
    );
    if (FAILED(hr)) {
        lastError = Utility::hr2str(hr);
        return false;
    }

    auto count = registeredDevices.size();
    int idx = 0;
    for (const auto& device : registeredDevices) {
        WCHAR wszGUID[40] = { 0 };
        StringFromGUID2(device.guidProduct, wszGUID, 40);
        devices.push_back(std::format("[{}] {} {}", idx, std::string(CW2A(device.tszProductName)), std::string(CW2A(wszGUID))));
        ++idx;
    }

    return true;
}

bool GamepadTracker::bind(int deviceIndex) {

    if (device) {
        lastError = "Already bound to a device";
        return false;
    }

    // Iterate through devices
    // NOTE: Blocking until it goes through all devices
    auto hr = directInput->EnumDevices(
        DI8DEVCLASS_GAMECTRL,
        &GamepadTracker::onEnumDevice,
        this,
        DIEDFL_ALLDEVICES
    );
    if (FAILED(hr)) {
        lastError = std::format("Could not enumerate devices: {}", Utility::hr2str(hr));
        return false;
    }

    auto count = registeredDevices.size();

    if (count == 0) {
        lastError = std::format("No game controller found at index {}\nEnsure it is connected before starting the program", deviceIndex);
        return false;
    }

    if (deviceIndex < 0 || deviceIndex >= count) {
        lastError = std::format("Gamepad device may only be a number between 0 and {} inclusively", count - 1);
        return false;
    }

    // Register device
    hr = directInput->CreateDevice(
        registeredDevices.at(deviceIndex).guidInstance,
        &device,
        NULL
    );
    if (FAILED(hr)) {
        lastError = std::format("Could not create device object: {}", Utility::hr2str(hr));
        return false;
    }

    hr = device->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(hr)) {
        lastError = std::format("Could not set data format: {}", Utility::hr2str(hr));
        return false;
    }

    hr = device->SetCooperativeLevel(
        GetDesktopWindow(), // Out of pure laziness. Surely an invisible window is better
        DISCL_BACKGROUND | DISCL_NONEXCLUSIVE
    );
    if (FAILED(hr)) {
        lastError = std::format("Could not set cooperative level: {}", Utility::hr2str(hr));
        return false;
    }

    hr = device->Acquire();
    if (FAILED(hr)) {
        lastError = std::format("Could not acquire device: {}", Utility::hr2str(hr));
        return false;
    }

    return true;
}

bool GamepadTracker::unbind() {

    if (!device) {
        return true;
    }

    HRESULT hr = device->Unacquire();
    if (FAILED(hr)) {
        lastError = std::format("Could not unacquire device: {}", Utility::hr2str(hr));
        return false;
    }
    device = nullptr;

    return true;
}

void GamepadTracker::teardown() {

    unbind();
    auto hr = directInput->Release();
    directInput = nullptr;
    _isSetup = false;
}

bool GamepadTracker::refreshState() {
    if (!device) {
        lastError = "Not initialized";
        return false;
    }

    auto hr = device->Poll();
    if (FAILED(hr)) {
        lastError = Utility::hr2str(hr);
        return false;
    }

    ZeroMemory(&st, sizeof(DIJOYSTATE2));
    device->GetDeviceState(sizeof(DIJOYSTATE2), &st);

    // Retrieve first 32 buttons
    for (short i = 31; i >= 0; i--)
    {
        buttonFlags = buttonFlags << 1;
        buttonFlags = buttonFlags | (bool)(this->st.rgbButtons[i] & 0x80);
    }

    writeState();

    return true;
}

void GamepadTracker::writeState() {

    state = std::format(
        "{};{};{};{};{};{};{};{};{}",
        PROTOCOL_VERSION,
        this->buttonFlags,
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
