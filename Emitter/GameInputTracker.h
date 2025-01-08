#pragma once
#include <GameInput.h>
#include <string>
#include <wrl.h>

class GameInputTracker {
private:
    Microsoft::WRL::ComPtr<IGameInput> gameInput;
    Microsoft::WRL::ComPtr<IGameInputReading> reading;

    bool islowLevelChannelMode = false;

    std::string axisCache;
    std::string buttonCache;
    std::string switchCache;

    IGameInputDevice* boundDevice = nullptr;
    GameInputGamepadState gamepadState;

    void writeState();

public:
    static const unsigned short PROTOCOL_VERSION = 2;
    std::string state;
    std::string lastError;

    GameInputTracker(bool lowLevelChannel = false);

    bool setup();
    void teardown();
    bool refreshState();
};
