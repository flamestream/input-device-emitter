#include "MouseTracker.h"
#include <format>
#include <iostream>
#include <bitset>

const wchar_t* MouseTracker::CLIP_STUDIO_PAINT_CLASSNAME = L"742DEA58-ED6B-4402-BC11-20DFC6D08040";
const wchar_t* MouseTracker::BLENDER_CLASSNAME = L"GHOST_WindowClass";

MouseTracker::MouseTracker() {

    this->hDll = 0;
    this->hWhMouseLl = 0;
    this->GetStateProc = 0;
}

bool MouseTracker::setup() {

    // Load DLL
    this->hDll = LoadLibrary(L"MouseTracker.dll");
    if (this->hDll == NULL) {
        std::cout << "DLL could not be loaded: Error " << GetLastError() << std::endl;
        return false;
    }

    // Retrieve callback procedure
    HOOKPROC LowLevelMouseProc = (HOOKPROC)GetProcAddress(this->hDll, "LowLevelMouseProc");
    if (LowLevelMouseProc == NULL) {
        std::cout << "LowLevelMouseProc could not be loaded: Error " << GetLastError() << std::endl;
        return false;
    }

    // Install Low Level Mouse Global Hook
    this->hWhMouseLl = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, this->hDll, 0);
    if (this->hWhMouseLl == NULL) {
        std::cout << "Hook to WH_MOUSE_LL failed: Error " << GetLastError() << std::endl;
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

void MouseTracker::teardown() {

    if (this->hWhMouseLl) {
        UnhookWindowsHookEx(this->hWhMouseLl);
    }
    FreeLibrary(this->hDll);
}

void MouseTracker::refreshState() {

    this->st = this->GetStateProc();
}

std::string MouseTracker::getUdpMessage() {

    return std::format(
        "{};{};{};{};{};{}",
        PROTOCOL_VERSION,
        this->st.x,
        this->st.y,
        (int)this->getEventSource(),
        (int)this->st.mouse1,
        (int)this->st.mouse2
    );
}

MouseTracker::EventSource MouseTracker::getEventSource() {
    // It is generally good enough to check if extra info contains something or not to
    // detect pen detection. It can also mean touch, but it won't be fully supported for now.
    if (this->st.extraInfo == 0) {
        return MouseTracker::EventSource::Mouse;
    }

    return MouseTracker::EventSource::Pen;

    //std::bitset<64> b{ this->st.extraInfo };
    //std::cout << b.to_string() << std::endl;
    //
    // Get window handle u
    //POINT p;
    //GetCursorPos(&p);
    //HWND hwnd = WindowFromPoint(p);
    //wchar_t bufferClassName[256];
    //int numChar = GetClassName(hwnd, bufferClassName, sizeof(bufferClassName));
    //std::wstring className = std::wstring(bufferClassName, numChar);
    //std::wcout << className << L"                                               " << std::endl;
    //
    //// CLIP STUDIO PAINT taints the extra info with something that keeps on
    //// contantly increasing, making it seemingly unusable.
    //// However, this is done when the input source is a pen.
    //// We can take advantage of that behaviour
    //if (className.starts_with(MouseTracker::CLIP_STUDIO_PAINT_CLASSNAME)) {
    //    if (this->st.extraInfo == 0) {
    //        return MouseTracker::EventSource::Mouse;
    //    }
    //    else {
    //        // Good enough
    //        return MouseTracker::EventSource::Pen;
    //    }
    //}
    //// BLENDER taints the extra info with something that keeps on
    //// contantly increasing, making it seemingly unusable.
    //// However, this is done when the input source is a pen.
    //// We can take advantage of that behaviour
    //else if (className == MouseTracker::BLENDER_CLASSNAME) {
    //    if (this->st.extraInfo == 0) {
    //        return MouseTracker::EventSource::Mouse;
    //    }
    //    else {
    //        // Good enough
    //        return MouseTracker::EventSource::Pen;
    //    }
    //}

    //// Normal parsing
    //bool isTouchOrPen = ((this->st.extraInfo & 0xFFFFFF00) == 0xFF515700);
    //if (!isTouchOrPen)
    //    return MouseTracker::EventSource::Mouse;

    //bool isTouch = ((this->st.extraInfo & 0x00000080) == 0x00000080);

    //return isTouch ? MouseTracker::EventSource::Touch : MouseTracker::EventSource::Pen;
}

void MouseTracker::printDebugData() {
    std::bitset<64> b{ this->st.extraInfo };
    std::cout << b.to_string() << std::endl;
}