#pragma once
#include <string>
#include <vector>
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

class GamepadTracker {
private:
    IDirectInput8W* directInput;
    std::vector<DIDEVICEINSTANCEW> registeredDevices;
    LPDIRECTINPUTDEVICE8W device;
    DIJOYSTATE2 st;
    bool _isBound;

    static BOOL CALLBACK onEnumDevice(LPCDIDEVICEINSTANCEW lpddi, LPVOID pvRef);
public:

    static const unsigned short PROTOCOL_VERSION = 1;

    GamepadTracker();

    bool isBound() { return _isBound; };

    bool setup();
    void save(DIDEVICEINSTANCEW di);
    void teardown();

    std::string getUdpMessage();
    short translateDPad(DWORD input);
};

