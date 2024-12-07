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
    };
    typedef State(*DLL_STATE)();

    HMODULE hDll;
    HHOOK hWhKeyboardLl;
    State st;
    DLL_STATE GetStateProc;
public:

    static const unsigned short PROTOCOL_VERSION = 1;

    KeyboardTracker();

    bool setup();
    void teardown();

    void refreshState();
    std::string getUdpMessage();
    void printDebugData();

    LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
};

