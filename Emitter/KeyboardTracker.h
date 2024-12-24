#pragma once
#include <wtypes.h>
#include <string>

class KeyboardTracker {
private:
    struct State {
        unsigned long long part1;
        unsigned long long part2;
        unsigned long long part3;
        unsigned long long part4;

        bool operator==(const State& other) const {
            return part1 == other.part1
                && part2 == other.part2
                && part3 == other.part3
                && part4 == other.part4;
        }
    };
    typedef State(*DLL_STATE)();

    HMODULE hDll;
    HHOOK hWhKeyboardLl;
    State st;
    DLL_STATE GetStateProc;

    void writeState();
public:

    static const unsigned short PROTOCOL_VERSION = 1;

    std::string state;
    std::string lastError;

    KeyboardTracker();

    bool setup();
    void teardown();

    void refreshState();

    LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
};

