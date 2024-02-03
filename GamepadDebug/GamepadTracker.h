#pragma once
#include <string>
#include <vector>
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

class GamepadTracker {
public:
    IDirectInput8W* directInput;
    std::vector<DIDEVICEINSTANCEW> registeredDevices;
    LPDIRECTINPUTDEVICE8W device;
    DIJOYSTATE2 st;
    bool _isBound;
    int deviceIndex;

    static BOOL CALLBACK onEnumDevice(LPCDIDEVICEINSTANCEW lpddi, LPVOID pvRef);

    static const unsigned short PROTOCOL_VERSION = 1;

    GamepadTracker(int deviceIndex);

    bool isBound() { return _isBound; };

    bool setup();
    void save(DIDEVICEINSTANCEW di);
    void teardown();

    void poll();
    short translateDPad(DWORD input);
};

