#include "pch.h"
#include <Windows.h>
#include <bitset>

std::bitset<256> DIVIDER(0b0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001111111111111111111111111111111111111111111111111111111111111111);

#pragma data_seg (".sharedkb")
std::bitset<256> vkCodeDown;
bool vkCodeDown2[256];
#pragma data_seg()
#pragma comment(linker, "/SECTION:.sharedkb,RWS")

struct State {
    unsigned long long part1;
    unsigned long long part2;
    unsigned long long part3;
    unsigned long long part4;

};

//struct State {
//    bool vkCodeDown2[256];
//};

extern "C" __declspec(dllexport) State GetStateProc() {

    auto part1 = (vkCodeDown & DIVIDER).to_ullong();
    auto part2 = ((vkCodeDown >> 64) & DIVIDER).to_ullong();
    auto part3 = ((vkCodeDown >> (64 * 2)) & DIVIDER).to_ullong();
    auto part4 = ((vkCodeDown >> (64 * 3)) & DIVIDER).to_ullong();

    State out = {
        part1,
        part2,
        part3,
        part4,
    };

    return out;
}

// For handling WH_KEYBOARD_LL
extern "C" __declspec(dllexport) LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {

    if (nCode == HC_ACTION) {
        tagKBDLLHOOKSTRUCT* info;
        info = (tagKBDLLHOOKSTRUCT*)lParam;
        switch (wParam) {
        case WM_KEYDOWN:
            vkCodeDown.set((int)info->vkCode, true);
            break;
        case WM_KEYUP:
            vkCodeDown.set((int)info->vkCode, false);
            break;
        case WM_SYSKEYDOWN:
            vkCodeDown.set((int)info->vkCode, true);
            break;
        case WM_SYSKEYUP:
            vkCodeDown.set((int)info->vkCode, false);
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