#include "pch.h"
#include <Windows.h>

#pragma data_seg (".sharedkb")
unsigned long long vkParts[4] = { 0, 0, 0, 0 };
#pragma data_seg()
#pragma comment(linker, "/SECTION:.sharedkb,RWS")

struct State {
    unsigned long long part1;
    unsigned long long part2;
    unsigned long long part3;
    unsigned long long part4;
};

extern "C" __declspec(dllexport) State GetStateProc() {

    State out = {
        vkParts[0],
        vkParts[1],
        vkParts[2],
        vkParts[3],
    };

    return out;
}

// For handling WH_KEYBOARD_LL
extern "C" __declspec(dllexport) LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {

    if (nCode == HC_ACTION) {
        tagKBDLLHOOKSTRUCT* info = (tagKBDLLHOOKSTRUCT*)lParam;
        if (info && info->vkCode < 256) {
            DWORD vk = info->vkCode;

            // Differentiate Numpad Enter (extended VK_RETURN) from main keyboard Enter
            // FS Input Receiver maps 14 (0x0E) to VKCode.NUMPAD_RETURN
            if (vk == VK_RETURN && (info->flags & LLKHF_EXTENDED)) {
                vk = 14;
            }
            // Defensive alias for synthetic macro software emitting generic modifier VKs
            else if (vk == VK_SHIFT) {
                vk = (info->scanCode == 0x36) ? VK_RSHIFT : VK_LSHIFT;
            }
            else if (vk == VK_CONTROL) {
                vk = (info->flags & LLKHF_EXTENDED) ? VK_RCONTROL : VK_LCONTROL;
            }
            else if (vk == VK_MENU) {
                vk = (info->flags & LLKHF_EXTENDED) ? VK_RMENU : VK_LMENU;
            }

            size_t idx = vk / 64;
            unsigned long long mask = 1ULL << (vk % 64);
            switch (wParam) {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                vkParts[idx] |= mask;
                break;
            case WM_KEYUP:
            case WM_SYSKEYUP:
                vkParts[idx] &= ~mask;
                break;
            }
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