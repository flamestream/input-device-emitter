#include "pch.h"
#include <Windows.h>

#pragma data_seg (".shared")
LONG x = 0;
LONG y = 0;
bool mouse1 = false;
bool mouse2 = false;
ULONG_PTR extraInfo = 0;
#pragma data_seg()
#pragma comment(linker, "/SECTION:.shared,RWS")

struct State {
    LONG x = 0;
    LONG y = 0;
    bool mouse1 = false;
    bool mouse2 = false;
    ULONG_PTR extraInfo = 0;
};

extern "C" __declspec(dllexport) State GetStateProc() {

    State out = {
        x,
        y,
        mouse1,
        mouse2,
        extraInfo
    };
    return out;
}

// For handling WH_MOUSE_LL
extern "C" __declspec(dllexport) LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {

    if (nCode == HC_ACTION) {
        tagMSLLHOOKSTRUCT* info;
        // NOTE: Can't see any other message
        switch (wParam) {
        case WM_LBUTTONDOWN:
            mouse1 = true;
            break;
        case WM_LBUTTONUP:
            mouse1 = false;
            break;
        case WM_MOUSEMOVE:
            info = (tagMSLLHOOKSTRUCT*)lParam;
            x = info->pt.x;
            y = info->pt.y;
            extraInfo = info->dwExtraInfo;
            break;
        case WM_RBUTTONDOWN:
            mouse2 = true;
            break;
        case WM_RBUTTONUP:
            mouse2 = false;
            break;
        case WM_MOUSEWHEEL:
            break;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}