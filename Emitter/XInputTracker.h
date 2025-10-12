#pragma once

// Define target architecture before including Windows headers
#ifndef _WIN64
    #define _X86_
#else
    #define _AMD64_
#endif

#include <string>
#include <Xinput.h>

#pragma comment(lib, "xinput.lib")

class XInputTracker {
private:
    XINPUT_STATE xinputState;
    XINPUT_CAPABILITIES capabilities;
    DWORD controllerIndex;
    bool isConnected;
    
    std::string buttonCache;
    std::string axisCache;
    
    void writeState();
    
public:
    static const unsigned short PROTOCOL_VERSION = 1;
    
    std::string state;
    std::string lastError;
    
    XInputTracker(DWORD controllerIndex = 0);
    
    bool setup();
    void teardown();
    bool refreshState();
    bool isControllerConnected();
    
    // Utility methods for getting controller info
    bool getControllerCapabilities();
    std::string getControllerType();
};