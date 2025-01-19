#pragma once
#include <string>
#include <vector>
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

class DirectInputTracker {
private:
    IDirectInput8W* directInput;
    std::vector<DIDEVICEINSTANCEW> registeredDevices;
    LPDIRECTINPUTDEVICE8W device;
    DIJOYSTATE2 st;
    bool _isSetup;

    std::string buttonFlags;

    static BOOL CALLBACK onEnumDevice(LPCDIDEVICEINSTANCEW lpddi, LPVOID pvRef);
public:

    static const unsigned short PROTOCOL_VERSION = 3;

    std::string state;
    std::string lastError;

    DirectInputTracker();

    bool isSetup() const { return _isSetup; };

    bool setup();
    bool getDeviceChoices(std::vector<std::string> &devicesChoices);
    bool bind(int deviceIndex);
    bool unbind();
    void save(DIDEVICEINSTANCEW di);
    void teardown();

    bool refreshState();
    void writeState();
    short translateDPad(DWORD input);
};
