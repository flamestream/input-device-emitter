#include "XInputTracker.h"
#include <format>
#include <sstream>
#include <bitset>

#include "Utility.h"

XInputTracker::XInputTracker(DWORD controllerIndex) {
    this->controllerIndex = controllerIndex;
    this->isConnected = false;
    ZeroMemory(&xinputState, sizeof(XINPUT_STATE));
    ZeroMemory(&capabilities, sizeof(XINPUT_CAPABILITIES));

    writeState();
}

bool XInputTracker::setup() {
    // Check if controller is connected
    DWORD result = XInputGetState(controllerIndex, &xinputState);
    if (result != ERROR_SUCCESS) {
        lastError = std::format("XInput controller index {} not connected or failed to initialize", controllerIndex);
        return false;
    }

    isConnected = true;

    // Get controller capabilities
    if (!getControllerCapabilities()) {
        lastError = std::format("Failed to get XInput controller index {} capabilities", controllerIndex);
        return false;
    }

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
            isConnected = false;
            lastError = std::format("XInput controller index {} disconnected", controllerIndex);
        } else {
            lastError = std::format("Failed to get XInput controller {} state: {}", controllerIndex, result);
        }
        return false;
    }

    if (!isConnected) {
        isConnected = true; // Controller reconnected
    }

    // Update state
    xinputState = newState;

    // Process gamepad data similar to GameInput pattern
    const auto& gamepad = xinputState.Gamepad;

    // Convert buttons to binary string (similar to GameInputTracker)
    unsigned int buttonBits = gamepad.wButtons;
    std::string bitString = std::bitset<sizeof(buttonBits) * 8>(buttonBits).to_string();
    buttonCache = bitString;

    // Convert analog values to normalized floats (similar to GameInputTracker)
    std::stringstream ss;
    ss << (gamepad.sThumbLX / 32767.0f) * 0.5f + 0.5f        // Left stick X (normalize to 0-1)
       << "|" << (gamepad.sThumbLY / 32767.0f) * 0.5f + 0.5f  // Left stick Y (normalize to 0-1)
       << "|" << (gamepad.sThumbRX / 32767.0f) * 0.5f + 0.5f  // Right stick X (normalize to 0-1)
       << "|" << (gamepad.sThumbRY / 32767.0f) * 0.5f + 0.5f  // Right stick Y (normalize to 0-1)
       << "|" << gamepad.bLeftTrigger / 255.0f                // Left trigger (normalize to 0-1)
       << "|" << gamepad.bRightTrigger / 255.0f;              // Right trigger (normalize to 0-1)
    axisCache = ss.str();

    writeState();
    return true;
}

void XInputTracker::writeState() {
    state = std::format(
        "{}X;{};{};{}",
        PROTOCOL_VERSION,
        buttonCache,
        "", // XInput doesn't have switches like GameInput
        axisCache
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
    if (!isConnected) {
        return "Not Connected";
    }

    // XInput doesn't provide detailed type info, but we can make educated guesses
    switch (capabilities.SubType) {
        case XINPUT_DEVSUBTYPE_GAMEPAD:
            return "XInput Gamepad";
        case XINPUT_DEVSUBTYPE_WHEEL:
            return "XInput Wheel";
        case XINPUT_DEVSUBTYPE_ARCADE_STICK:
            return "XInput Arcade Stick";
        case XINPUT_DEVSUBTYPE_FLIGHT_STICK:
            return "XInput Flight Stick";
        case XINPUT_DEVSUBTYPE_DANCE_PAD:
            return "XInput Dance Pad";
        case XINPUT_DEVSUBTYPE_GUITAR:
            return "XInput Guitar";
        case XINPUT_DEVSUBTYPE_DRUM_KIT:
            return "XInput Drum Kit";
        default:
            return "Unknown XInput Device";
    }
}
