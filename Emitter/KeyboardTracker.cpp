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
    this->st = { 0, 0, 0, 0 };

    writeState();
}

bool KeyboardTracker::setup() {

    // Load DLL
    this->hDll = LoadLibrary(L"FsKeyboardTracker.dll");
    if (this->hDll == NULL) {
        lastError = std::format("Keyboard Tracker DLL could not be loaded: Error {}", GetLastError());
        return false;
    }

    // Retrieve callback procedure
    HOOKPROC LowLevelKeyboardProc = (HOOKPROC)GetProcAddress(this->hDll, "LowLevelKeyboardProc");
    if (LowLevelKeyboardProc == NULL) {
        lastError = std::format("LowLevelKeyboardProc could not be loaded: Error {}", GetLastError());
        return false;
    }

    // Install Low Level Keyboard Global Hook
    this->hWhKeyboardLl = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, this->hDll, 0);
    if (this->hWhKeyboardLl == NULL) {
        lastError = std::format("Hook to WH_KEYBOARD_LL failed: Error {}", GetLastError());
        return false;
    }

    // Retrieve state getter
    this->GetStateProc = (DLL_STATE)GetProcAddress(hDll, "GetStateProc");
    if (GetStateProc == NULL) {
        lastError = std::format("Keyboard Tracker GetStateProc could not be loaded: Error {}", GetLastError());
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

    State newState = this->GetStateProc();

    if (st != newState) {
        st = newState;
        writeState();
    }
}

void KeyboardTracker::writeState() {

    std::bitset<64> p1{ this->st.part1 };
    std::bitset<64> p2{ this->st.part2 };
    std::bitset<64> p3{ this->st.part3 };
    std::bitset<64> p4{ this->st.part4 };

    state = std::format(
        "{};{};{};{};{}",
        PROTOCOL_VERSION,
        p1.to_string(),
        p2.to_string(),
        p3.to_string(),
        p4.to_string()
    );
}
