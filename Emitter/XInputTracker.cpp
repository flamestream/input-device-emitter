#include "XInputTracker.h"
#include <format>
#include "Utility.h"

XInputTracker::XInputTracker(DWORD controllerIndex) {
    this->controllerIndex = controllerIndex;
    this->isConnected = false;
    ZeroMemory(&xinputState, sizeof(XINPUT_STATE));
    ZeroMemory(&capabilities, sizeof(XINPUT_CAPABILITIES));

    writeState();
}

bool XInputTracker::setup() {
    // Don't fail if controller is not connected initially
    // Just check and update connection status
    DWORD result = XInputGetState(controllerIndex, &xinputState);
    if (result == ERROR_SUCCESS) {
        isConnected = true;

        // Try to get controller capabilities
        getControllerCapabilities();
        writeState();
    } else {
        isConnected = false;
        lastError = "No controller found. Waiting for connection...";
    }

    // Always return true - we'll handle connection state in refreshState
    return true;
}

void XInputTracker::teardown() {
    // XInput doesn't require explicit cleanup
    isConnected = false;
}

bool XInputTracker::refreshState() {
    XINPUT_STATE newState;
    DWORD result = XInputGetState(controllerIndex, &newState);

    if (result != ERROR_SUCCESS) {
        if (result == ERROR_DEVICE_NOT_CONNECTED) {
            if (isConnected) {
                // Only update error message on state change
                lastError = "Controller has been disconnected";
            }
            isConnected = false;
        } else {
            lastError = std::format("Failed to get controller state: {}", result);
        }

        // Don't fail - just return true with disconnected state
        writeState();
        return true;
    }

    // Controller is connected
    if (!isConnected) {
        // Controller reconnected - get capabilities
        isConnected = true;
        getControllerCapabilities();
        lastError = ""; // Clear error on reconnection
        xinputState = newState;
        writeState();
        return true;
    }

    // If packet number hasn't changed, controller state is identical
    if (newState.dwPacketNumber == xinputState.dwPacketNumber) {
        return true;
    }

    // Update state
    xinputState = newState;
    writeState();
    return true;
}

void XInputTracker::writeState() {
    const auto& gamepad = xinputState.Gamepad;

    // Convert 16-bit buttons to 32-char binary string (MSB to LSB)
    char btnStr[33];
    btnStr[32] = '\0';
    for (int i = 0; i < 32; ++i) {
        btnStr[i] = ((gamepad.wButtons >> (31 - i)) & 1) ? '1' : '0';
    }

    state = std::format(
        "{}X;{};;{:.6f}|{:.6f}|{:.6f}|{:.6f}|{:.6f}|{:.6f}",
        PROTOCOL_VERSION,
        btnStr,
        Utility::normalizeShort(gamepad.sThumbLX),
        Utility::normalizeShort(gamepad.sThumbLY),
        Utility::normalizeShort(gamepad.sThumbRX),
        Utility::normalizeShort(gamepad.sThumbRY),
        gamepad.bLeftTrigger / 255.0f,
        gamepad.bRightTrigger / 255.0f
    );
}

bool XInputTracker::isControllerConnected() {
    return isConnected;
}

bool XInputTracker::getControllerCapabilities() {
    DWORD result = XInputGetCapabilities(controllerIndex, XINPUT_FLAG_GAMEPAD, &capabilities);
    return result == ERROR_SUCCESS;
}

std::string XInputTracker::getControllerType() {
    return getControllerType(controllerIndex);
}

std::string XInputTracker::getControllerType(DWORD controllerIndex) {
    XINPUT_CAPABILITIES caps;
    if (XInputGetCapabilities(controllerIndex, XINPUT_FLAG_GAMEPAD, &caps) != ERROR_SUCCESS) {
        return "Error";
    }

    switch (caps.SubType) {
        case XINPUT_DEVSUBTYPE_GAMEPAD:
            return "Gamepad";
        case XINPUT_DEVSUBTYPE_WHEEL:
            return "Wheel";
        case XINPUT_DEVSUBTYPE_ARCADE_STICK:
            return "Arcade Stick";
        case XINPUT_DEVSUBTYPE_FLIGHT_STICK:
            return "Flight Stick";
        case XINPUT_DEVSUBTYPE_DANCE_PAD:
            return "Dance Pad";
        case XINPUT_DEVSUBTYPE_GUITAR:
            return "Guitar";
        case XINPUT_DEVSUBTYPE_DRUM_KIT:
            return "Drum Kit";
        default:
            return "Unknown Device Type";
    }
}

void XInputTracker::getControllerChoices(std::vector<std::string>& controllerChoices) {
    controllerChoices.clear();

    for (DWORD i = 0; i < XUSER_MAX_COUNT; i++) {
        XINPUT_STATE state;
        DWORD result = XInputGetState(i, &state);

        std::string choice;
        if (result == ERROR_SUCCESS) {
            // Controller is connected, get its type using the static method
            std::string controllerType = getControllerType(i);
            choice = std::format("Controller {} - {}", i, controllerType);
        } else {
            // Controller is not connected
            choice = std::format("Controller {} - Not Connected", i);
        }

        controllerChoices.push_back(choice);
    }
}
