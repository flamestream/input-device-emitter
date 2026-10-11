#include "MouseTracker.h"
#include <format>
#include <bitset>

const wchar_t* MouseTracker::CLIP_STUDIO_PAINT_CLASSNAME = L"742DEA58-ED6B-4402-BC11-20DFC6D08040";
const wchar_t* MouseTracker::BLENDER_CLASSNAME = L"GHOST_WindowClass";

MouseTracker::MouseTracker() {
    this->hDll = 0;
    this->hWhMouseLl = 0;
    this->GetStateProc = 0;
    this->udpMessageBuffer[0] = '\0';
    this->udpMessageLength = 0;
}

bool MouseTracker::setup() {
    // Load DLL
    this->hDll = LoadLibrary(L"FsMouseTracker.dll");
    if (this->hDll == NULL) {
        lastError = std::format("Mouse Tracker DLL could not be loaded: Error {}", GetLastError());
        return false;
    }

    // Retrieve callback procedure
    HOOKPROC LowLevelMouseProc = (HOOKPROC)GetProcAddress(this->hDll, "LowLevelMouseProc");
    if (LowLevelMouseProc == NULL) {
        lastError = std::format("LowLevelMouseProc could not be loaded: Error {}", GetLastError());
        return false;
    }

    // Install Low Level Mouse Global Hook
    this->hWhMouseLl = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, this->hDll, 0);
    if (this->hWhMouseLl == NULL) {
        lastError = std::format("Hook to WH_MOUSE_LL failed: Error {}", GetLastError());
        return false;
    }

    // Retrieve state getter
    this->GetStateProc = (DLL_STATE)GetProcAddress(hDll, "GetStateProc");
    if (GetStateProc == NULL) {
        lastError = std::format("Mouse Tracker GetStateProc could not be loaded: Error {}", GetLastError());
        return false;
    }

    return true;
}

void MouseTracker::teardown() {
    if (this->hWhMouseLl) {
        UnhookWindowsHookEx(this->hWhMouseLl);
        this->hWhMouseLl = NULL;
    }
    if (this->hDll) {
        FreeLibrary(this->hDll);
        this->hDll = NULL;
    }
}

void MouseTracker::refreshState() {
    this->st = this->GetStateProc();
}

std::string_view MouseTracker::getUdpMessage() {
    auto res = std::format_to_n(
        this->udpMessageBuffer,
        sizeof(this->udpMessageBuffer),
        "{}P;{};{};{};{};{}",
        PROTOCOL_VERSION,
        this->st.x,
        this->st.y,
        static_cast<int>(this->getEventSource()),
        static_cast<int>(this->st.mouse1),
        static_cast<int>(this->st.mouse2)
    );
    this->udpMessageLength = static_cast<size_t>(res.size);
    return std::string_view(this->udpMessageBuffer, this->udpMessageLength);
}

MouseTracker::EventSource MouseTracker::getEventSource() {
    // It is generally good enough to check if extra info contains something or not to
    // detect pen detection. It can also mean touch, but it won't be fully supported for now.
    if (this->st.extraInfo == 0) {
        return MouseTracker::EventSource::Mouse;
    }

    return MouseTracker::EventSource::Pen;
}

std::string MouseTracker::getExtraDebugData() {
    std::bitset<64> b{ this->st.extraInfo };
    return b.to_string();
}