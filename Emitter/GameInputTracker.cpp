#include "GameInputTracker.h"
#include <format>
#include <GameInput.h>
#include <atlstr.h>
#include <sstream>
#include <bitset>
#include <algorithm>

#include "Utility.h"

GameInputTracker::GameInputTracker(bool islowLevelChannelWanted) {
    islowLevelChannelMode = islowLevelChannelWanted;
    writeState();
}

bool GameInputTracker::setup() {
    HRESULT hr = GameInputCreate(&gameInput);
    if (FAILED(hr)) {
        lastError = Utility::hr2str(hr);
        return false;
    }

    return true;
}

void GameInputTracker::teardown() {

    if (!gameInput) return;

    gameInput->Release();
}

void GameInputTracker::writeState() {
    state = std::format(
        "{};{};{};{}",
        PROTOCOL_VERSION,
        buttonCache,
        switchCache,
        axisCache
    );
}

bool GameInputTracker::refreshState() {

    bool updated = false;
    HRESULT hr = 0;
    hr = gameInput->GetCurrentReading(GameInputKindGamepad, boundDevice, &reading);
    if (FAILED(hr)) {
        if (boundDevice) {
            boundDevice->Release();
            boundDevice = nullptr;
        }
        lastError = Utility::hr2str(hr);
        return false;
    }

    if (!boundDevice) {
        reading->GetDevice(&boundDevice);
    }

    if (islowLevelChannelMode) {

        // Read controller buttons
        auto buttonCount = reading->GetControllerButtonCount();
        buttonCache = "";
        if (buttonCount > 0) {
            auto buttonReading = std::make_unique<bool[]>(buttonCount);
            auto readCount = reading->GetControllerButtonState(buttonCount, buttonReading.get());
            buttonCache = Utility::boolArr2str(buttonReading, readCount);
            updated = true;
        }

        // Read controller switches
        auto switchCount = reading->GetControllerSwitchCount();
        if (switchCount > 0) {
            auto switchReading = std::make_unique<GameInputSwitchPosition[]>(switchCount);
            auto readCount = reading->GetControllerSwitchState(switchCount, switchReading.get());
            std::string str(readCount, '0');
            for (size_t i = 0; i < readCount; ++i) {
                str[i] = '0' + static_cast<int>(switchReading[i]);
            }
            switchCache = str;
            updated = true;
        }

        // Read controller axis
        auto axisCount = reading->GetControllerAxisCount();
        if (axisCount > 0) {
            auto axisReading = std::make_unique<float[]>(axisCount);
            auto readCount = reading->GetControllerAxisState(axisCount, axisReading.get());
            axisCache = Utility::floatArr2str(axisReading, readCount, '|');
            updated = true;
        }

    } else {
    
        hr = reading->GetGamepadState(&gamepadState);
        if (FAILED(hr)) {
            lastError = Utility::hr2str(hr);
            return false;
        }

        std::stringstream ss;
        ss << gamepadState.leftThumbstickX * 0.5f + 0.5f
            << "|" << gamepadState.leftThumbstickY * 0.5f + 0.5f
            << "|" << gamepadState.rightThumbstickX * 0.5f + 0.5f
            << "|" << gamepadState.rightThumbstickY * 0.5f + 0.5f
            << "|" << gamepadState.leftTrigger
            << "|" << gamepadState.rightTrigger;
        axisCache = ss.str();

        unsigned int buttonBits = gamepadState.buttons;
        std::string bitString = std::bitset<sizeof(buttonBits) * 8>(buttonBits).to_string();
        buttonCache = bitString;

        updated = true;
    }

    if (updated) {
        writeState();
    }

    return true;
}
