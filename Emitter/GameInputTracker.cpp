#include "GameInputTracker.h"
#include <format>
#include <GameInput.h>
#include <atlstr.h>
#include <algorithm>

#include "Utility.h"

GameInputTracker::GameInputTracker(bool islowLevelChannelWanted) {
    this->islowLevelChannelMode = islowLevelChannelWanted;
    this->gameInput = nullptr;
    this->reading = nullptr;
    this->boundDevice = nullptr;
    ZeroMemory(&this->gamepadState, sizeof(this->gamepadState));
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
    if (boundDevice) {
        boundDevice->Release();
        boundDevice = nullptr;
    }
    if (gameInput) {
        gameInput->Release();
        gameInput = nullptr;
    }
}

void GameInputTracker::writeState() {
    state = std::format(
        "{}G;{};{};{}",
        PROTOCOL_VERSION,
        buttonCache,
        switchCache,
        axisCache
    );
}

bool GameInputTracker::refreshState() {
    HRESULT hr = gameInput->GetCurrentReading(GameInputKindGamepad, boundDevice, &reading);
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
        // Read controller buttons (stack buffer up to 128 buttons)
        auto buttonCount = reading->GetControllerButtonCount();
        if (buttonCount > 0) {
            bool buttonReading[128] = { 0 };
            uint32_t count = (std::min)(buttonCount, 128u);
            auto readCount = reading->GetControllerButtonState(count, buttonReading);
            std::string str(readCount, '0');
            for (size_t i = 0; i < readCount; ++i) {
                if (buttonReading[i]) {
                    str[readCount - 1 - i] = '1';
                }
            }
            buttonCache = str;
        } else {
            buttonCache = "";
        }

        // Read controller switches (stack buffer up to 32 switches)
        auto switchCount = reading->GetControllerSwitchCount();
        if (switchCount > 0) {
            GameInputSwitchPosition switchReading[32] = {};
            uint32_t count = (std::min)(switchCount, 32u);
            auto readCount = reading->GetControllerSwitchState(count, switchReading);
            std::string str(readCount, '0');
            for (size_t i = 0; i < readCount; ++i) {
                str[i] = '0' + static_cast<int>(switchReading[i]);
            }
            switchCache = str;
        } else {
            switchCache = "";
        }

        // Read controller axes (stack buffer up to 32 axes)
        auto axisCount = reading->GetControllerAxisCount();
        if (axisCount > 0) {
            float axisReading[32] = { 0.0f };
            uint32_t count = (std::min)(axisCount, 32u);
            auto readCount = reading->GetControllerAxisState(count, axisReading);
            std::string str;
            str.reserve(readCount * 10);
            for (size_t i = 0; i < readCount; ++i) {
                if (i != 0) str += '|';
                str += std::format("{:.6f}", axisReading[i]);
            }
            axisCache = str;
        } else {
            axisCache = "";
        }

        writeState();

    } else {
        GameInputGamepadState newState;
        hr = reading->GetGamepadState(&newState);
        if (FAILED(hr)) {
            lastError = Utility::hr2str(hr);
            reading->Release();
            return false;
        }

        // Dirty-check: if gamepad state hasn't changed, skip re-formatting
        if (memcmp(&newState, &gamepadState, sizeof(GameInputGamepadState)) == 0) {
            reading->Release();
            return true;
        }

        gamepadState = newState;

        char btnStr[33];
        btnStr[32] = '\0';
        for (int i = 0; i < 32; ++i) {
            btnStr[i] = ((gamepadState.buttons >> (31 - i)) & 1) ? '1' : '0';
        }

        state = std::format(
            "{}G;{};;{:.6f}|{:.6f}|{:.6f}|{:.6f}|{:.6f}|{:.6f}",
            PROTOCOL_VERSION,
            btnStr,
            gamepadState.leftThumbstickX * 0.5f + 0.5f,
            gamepadState.leftThumbstickY * 0.5f + 0.5f,
            gamepadState.rightThumbstickX * 0.5f + 0.5f,
            gamepadState.rightThumbstickY * 0.5f + 0.5f,
            gamepadState.leftTrigger,
            gamepadState.rightTrigger
        );
    }

    reading->Release();
    return true;
}
