#pragma once
#include <GameInput.h>
#include <string>
#include <wrl.h>

class GameInputTracker {
private:
    Microsoft::WRL::ComPtr<IGameInput> gameInput;
    Microsoft::WRL::ComPtr<IGameInputReading> reading;

    std::string axisCache;
    std::string buttonCache;
    std::string switchCache;

    void writeState();

public:
    static const unsigned short PROTOCOL_VERSION = 1;
    std::string state;
    std::string lastError;

    GameInputTracker();

    bool setup();
    void teardown();
    bool refreshState();
};
