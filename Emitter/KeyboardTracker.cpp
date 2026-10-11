#include "KeyboardTracker.h"
#include <format>
#include <iostream>

KeyboardTracker::KeyboardTracker() {
    this->hDll = 0;
    this->hWhKeyboardLl = 0;
    this->GetStateProc = 0;
    this->st = { 0, 0, 0, 0 };

    // Pre-allocate fixed packet template: "1K;<64>;<64>;<64>;<64>" (262 chars total)
    this->state.resize(262, '0');
    this->state[0] = static_cast<char>('0' + PROTOCOL_VERSION);
    this->state[1] = 'K';
    this->state[2] = ';';
    this->state[3 + 64] = ';';
    this->state[3 + 64 + 1 + 64] = ';';
    this->state[3 + 64 + 1 + 64 + 1 + 64] = ';';

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
    if (this->hWhKeyboardLl) {
        UnhookWindowsHookEx(this->hWhKeyboardLl);
        this->hWhKeyboardLl = NULL;
    }
    if (this->hDll) {
        FreeLibrary(this->hDll);
        this->hDll = NULL;
    }
}

void KeyboardTracker::refreshState() {
    State newState = this->GetStateProc();

    if (st != newState) {
        st = newState;
        writeState();
    }
}

void KeyboardTracker::writeState() {
    const unsigned long long parts[4] = { this->st.part1, this->st.part2, this->st.part3, this->st.part4 };
    size_t offset = 3;
    for (int p = 0; p < 4; ++p) {
        unsigned long long val = parts[p];
        for (int b = 63; b >= 0; --b) {
            this->state[offset++] = ((val >> b) & 1ULL) ? '1' : '0';
        }
        offset++; // Skip semicolon delimiter
    }
}
