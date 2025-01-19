#include "DirectInputTracker.h"
#include <format>
#include <hidusage.h>
#include <atlstr.h>

#include "Utility.h"

DirectInputTracker::DirectInputTracker() {
    this->directInput = 0;
    this->device = 0;
    this->st = {};
    this->buttonFlags = std::string(32, '\0');
    this->_isSetup = false;

    writeState();
}

bool DirectInputTracker::setup() {

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

bool DirectInputTracker::getDeviceChoices(std::vector<std::string> &devices) {

    if (!directInput) {
        lastError = "Not initialized";
        return false;
    }

    auto hr = directInput->EnumDevices(
        DI8DEVCLASS_GAMECTRL,
        &DirectInputTracker::onEnumDevice,
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

bool DirectInputTracker::bind(int deviceIndex) {

    if (device) {
        lastError = "Already bound to a device";
        return false;
    }

    // Iterate through devices
    // NOTE: Blocking until it goes through all devices
    auto hr = directInput->EnumDevices(
        DI8DEVCLASS_GAMECTRL,
        &DirectInputTracker::onEnumDevice,
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

bool DirectInputTracker::unbind() {

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

void DirectInputTracker::teardown() {

    unbind();
    auto hr = directInput->Release();
    directInput = nullptr;
    _isSetup = false;
}

bool DirectInputTracker::refreshState() {
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
    auto len = buttonFlags.length();
    for (short i = 0; i < len; ++i)
    {
        buttonFlags[len - i - 1] = (bool)(this->st.rgbButtons[i] & 0x80) ? '1' : '0';
    }

    writeState();

    return true;
}

void DirectInputTracker::writeState() {

    state = std::format(
        "{};{};{}|{}|{}|{};{:.6f}|{:.6f}|{:.6f}|{:.6f}|{:.6f}|{:.6f}|{:.6f}|{:.6f}",
        PROTOCOL_VERSION,
        this->buttonFlags,
        translateDPad(this->st.rgdwPOV[0]),
        translateDPad(this->st.rgdwPOV[1]),
        translateDPad(this->st.rgdwPOV[2]),
        translateDPad(this->st.rgdwPOV[3]),
        this->st.lX / 65535.0,
        this->st.lY / 65535.0,
        this->st.lZ / 65535.0,
        this->st.lRx / 65535.0,
        this->st.lRy / 65535.0,
        this->st.lRz / 65535.0,
        this->st.rglSlider[0] / 65535.0,
        this->st.rglSlider[1] / 65535.0
    );
}

BOOL CALLBACK DirectInputTracker::onEnumDevice(LPCDIDEVICEINSTANCEW lpddi, LPVOID pvRef) {


    DirectInputTracker* inst = reinterpret_cast<DirectInputTracker*>(pvRef);
    DIDEVICEINSTANCEW di = *lpddi;
    inst->save(di);

    return DIENUM_CONTINUE;
}

void DirectInputTracker::save(DIDEVICEINSTANCEW di) {

    registeredDevices.push_back(di);
}

short DirectInputTracker::translateDPad(DWORD input) {
    switch (input) {
        case 0: return 1;
        case 4500: return 2;
        case 9000: return 3;
        case 13500: return 4;
        case 18000: return 5;
        case 22500: return 6;
        case 27000: return 7;
        case 31500: return 8;
    }
    return 0;
}

