#pragma once
#include <wtypes.h>
#include <string>

class MouseTracker {
private:
    struct State {
        LONG x = 0;
        LONG y = 0;
        bool mouse1 = false;
        bool mouse2 = false;
        ULONG_PTR extraInfo = 0;
    };
    typedef State(*DLL_STATE)();

    HMODULE hDll;
    HHOOK hWhMouseLl;
    State st;
    DLL_STATE GetStateProc;
public:

    enum EventSource {
        Mouse,
        Touch,
        Pen,
    };

    static const unsigned short PROTOCOL_VERSION = 1;

    static const wchar_t* CLIP_STUDIO_PAINT_CLASSNAME;
    static const wchar_t* BLENDER_CLASSNAME;

    std::string lastError;

    MouseTracker();

    bool setup();
    void teardown();

    void refreshState();
    std::string getUdpMessage();
    EventSource getEventSource();
    std::string getExtraDebugData();

    LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
};

