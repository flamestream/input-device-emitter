#include "KeyboardTracker.h"
#include <format>
#include <iostream>
#include <bitset>
#include <iterator>
#include <algorithm>

KeyboardTracker::KeyboardTracker() {

    this->hDll = 0;
    this->hWhKeyboardLl = 0;
    this->GetStateProc = 0;
}

bool KeyboardTracker::setup() {

    // Load DLL
    this->hDll = LoadLibrary(L"FsKeyboardTracker.dll");
    if (this->hDll == NULL) {
        std::cout << "DLL could not be loaded: Error " << GetLastError() << std::endl;
        return false;
    }

    // Retrieve callback procedure
    HOOKPROC LowLevelKeyboardProc = (HOOKPROC)GetProcAddress(this->hDll, "LowLevelKeyboardProc");
    if (LowLevelKeyboardProc == NULL) {
        std::cout << "LowLevelKeyboardProc could not be loaded: Error " << GetLastError() << std::endl;
        return false;
    }

    // Install Low Level Keyboard Global Hook
    this->hWhKeyboardLl = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, this->hDll, 0);
    if (this->hWhKeyboardLl == NULL) {
        std::cout << "Hook to WH_KEYBOARD_LL failed: Error " << GetLastError() << std::endl;
        return false;
    }

    // Retrieve state getter
    this->GetStateProc = (DLL_STATE)GetProcAddress(hDll, "GetStateProc");
    if (GetStateProc == NULL) {
        std::cout << "GetStateProc could not be loaded: Error " << GetLastError() << std::endl;
        return false;
    }

    return true;
}

void KeyboardTracker::teardown() {

    if (this == NULL) return;

    if (this->hWhKeyboardLl) {
        UnhookWindowsHookEx(this->hWhKeyboardLl);
    }
    FreeLibrary(this->hDll);
}

void KeyboardTracker::refreshState() {

    this->st = this->GetStateProc();
}

std::string KeyboardTracker::getUdpMessage() {

    std::bitset<64> p1{ this->st.part1 };
    std::bitset<64> p2{ this->st.part2 };
    std::bitset<64> p3{ this->st.part3 };
    std::bitset<64> p4{ this->st.part4 };

    return std::format(
        "{};{};{};{};{}",
        PROTOCOL_VERSION,
        p1.to_string(),
        p2.to_string(),
        p3.to_string(),
        p4.to_string()
    );
}

void KeyboardTracker::printDebugData() {
    //std::cout << this->st.vkCodeDown[VK_SPACE] << std::endl;
}