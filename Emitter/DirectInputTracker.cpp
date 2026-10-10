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
    this->_isBound = false;
    this->targetDeviceIndex = -1;

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

    // Clear previous enumeration
    registeredDevices.clear();

    // Iterate through devices
    // NOTE: Blocking until it goes through all devices
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

    // Store target index for reconnection attempts
    targetDeviceIndex = deviceIndex;

    // If already bound, unbind first
    if (device) {
        unbind();
    }

    // Clear and re-enumerate devices
    registeredDevices.clear();

    auto hr = directInput->EnumDevices(
        DI8DEVCLASS_GAMECTRL,
        &DirectInputTracker::onEnumDevice,
        this,
        DIEDFL_ALLDEVICES
    );
    if (FAILED(hr)) {
        lastError = std::format("Could not enumerate devices: {}", Utility::hr2str(hr));
        _isBound = false;
        return true; // Don't fail, just set unbound state
    }

    auto count = registeredDevices.size();

    if (count == 0) {
        lastError = "No device found. Waiting for connection...";
        _isBound = false;
        return true;
    }

    if (deviceIndex < 0 || deviceIndex >= count) {
        lastError = std::format("Device index must be between 0 and {} (found {} devices)", count - 1, count);
        _isBound = false;
        return true;
    }

    // Register device
    hr = directInput->CreateDevice(
        registeredDevices.at(deviceIndex).guidInstance,
        &device,
        NULL
    );
    if (FAILED(hr)) {
        lastError = std::format("Could not create device object: {}", Utility::hr2str(hr));
        _isBound = false;
        return true;
    }

    hr = device->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(hr)) {
        lastError = std::format("Could not set data format: {}", Utility::hr2str(hr));
        if (device) {
            device->Release();
            device = nullptr;
        }
        _isBound = false;
        return true;
    }

    hr = device->SetCooperativeLevel(
        GetDesktopWindow(), // Out of pure laziness. Surely an invisible window is better
        DISCL_BACKGROUND | DISCL_NONEXCLUSIVE
    );
    if (FAILED(hr)) {
        lastError = std::format("Could not set cooperative level: {}", Utility::hr2str(hr));
        if (device) {
            device->Release();
            device = nullptr;
        }
        _isBound = false;
        return true;
    }

    hr = device->Acquire();
    if (FAILED(hr)) {
        lastError = std::format("Could not acquire device: {}", Utility::hr2str(hr));
        if (device) {
            device->Release();
            device = nullptr;
        }
        _isBound = false;
        return true;
    }

    _isBound = true;
    lastError = ""; // Clear error on successful binding
    return true;
}

bool DirectInputTracker::unbind() {

    if (device) {
        device->Unacquire();
        device->Release();
        device = nullptr;
    }
    _isBound = false;

    return true;
}

void DirectInputTracker::teardown() {

    unbind();
    if (directInput) {
        directInput->Release();
        directInput = nullptr;
    }
    _isSetup = false;
}

bool DirectInputTracker::refreshState() {
    // Try to reconnect if not bound
    if (!_isBound && targetDeviceIndex >= 0) {
        // Attempt to rebind
        bind(targetDeviceIndex);

        if (!_isBound) {
            // Still not bound, write disconnected state
            writeState();
            return true;
        }
    }

    if (!device) {
        // No device, write disconnected state
        _isBound = false;
        writeState();
        return true;
    }

    auto hr = device->Poll();
    if (FAILED(hr)) {
        // Device might be disconnected
        if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
            // Try to reacquire
            hr = device->Acquire();
            if (FAILED(hr)) {
                // Failed to reacquire - device is gone
                lastError = "Device disconnected";
                unbind(); // Clean up properly
                writeState();
                return true;
            }
            // Successfully reacquired, retry poll
            hr = device->Poll();
            if (FAILED(hr)) {
                lastError = "Device polling failed after reacquire";
                unbind();
                writeState();
                return true;
            }
        } else {
            // Other error
            lastError = Utility::hr2str(hr);
            unbind();
            writeState();
            return true;
        }
    }

    DIJOYSTATE2 newState;
    ZeroMemory(&newState, sizeof(DIJOYSTATE2));
    hr = device->GetDeviceState(sizeof(DIJOYSTATE2), &newState);

    if (FAILED(hr)) {
        if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
            lastError = "Device lost input";
            unbind();
        } else {
            lastError = Utility::hr2str(hr);
        }
        writeState();
        return true;
    }

    if (memcmp(&newState, &this->st, sizeof(DIJOYSTATE2)) == 0) {
        return true;
    }

    this->st = newState;

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
    // Include connection status in the state
    state = std::format(
        "{}D;{};{}|{}|{}|{};{:.6f}|{:.6f}|{:.6f}|{:.6f}|{:.6f}|{:.6f}|{:.6f}|{:.6f}",
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

bool DirectInputTracker::isBound() const {
    return _isBound;
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

