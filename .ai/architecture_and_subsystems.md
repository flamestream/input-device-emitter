# Architecture & Subsystems — FS Input Device Emitter

This document provides a comprehensive architectural breakdown of the **FS Input Device Emitter** (`FsInputEmitter.exe`) and its companion DLLs, diagnostic tools, and test clients.

---

## 1. High-Level System Architecture

FS Input Device Emitter is structured as a modular C++20 Windows desktop application that bridges low-level Windows input APIs and external network receivers (such as the **FS Input Receiver** plugin for Warudo) over low-latency UDP streams.

```
                    ┌────────────────────────────────────────┐
                    │               Windows OS               │
                    └────┬────────────┬─────────────┬────────┘
                         │            │             │
              Low-Level Hooks     COM / DInput8  GameInput / XInput
                         │            │             │
        ┌────────────────┼────────────┼─────────────┼────────────────┐
        │                ▼            ▼             ▼                │
        │  ┌───────────────────────┐ ┌───────────┐ ┌──────────────┐  │
        │  │ FsMouseTracker.dll    │ │ Direct-   │ │ GameInput /  │  │
        │  │ FsKeyboardTracker.dll │ │ Input 8   │ │ XInput       │  │
        │  │ (Shared Memory Segs)  │ │ Tracker   │ │ Trackers     │  │
        │  └───────────┬───────────┘ └─────┬─────┘ └──────┬───────┘  │
        │              │                   │              │          │
        │              └─────────────┬─────┴──────────────┘          │
        │                            ▼                               │
        │            ┌───────────────────────────────┐               │
        │            │     FsInputEmitter.exe        │               │
        │            │  (Main Loop + PeekMessage)    │               │
        │            └───────────────┬───────────────┘               │
        │                            ▼                               │
        │            ┌───────────────────────────────┐               │
        │            │   Winsock UDP Broadcaster     │               │
        │            └───────────────┬───────────────┘               │
        └────────────────────────────┼───────────────────────────────┘
                                     │ UDP Packets (40610..40614)
                                     ▼
                     ┌───────────────────────────────┐
                     │ Warudo: FS Input Receiver     │
                     │ or Custom UDP Listeners       │
                     └───────────────────────────────┘
```

---

## 2. Core Projects in the Solution

The solution [`InputEmitter.sln`](file:///d:/Code/input-device-emitter/InputEmitter.sln) contains four active projects:

| Project | Type | Output Binary | Primary Responsibility |
|:---|:---|:---|:---|
| **`Emitter`** | C++ Console App | `FsInputEmitter.exe` | Main application: CLI parsing, interactive setup, tracker orchestration, emission loop, UDP broadcast. |
| **`MouseTracker`** | C++ Win32 DLL | `FsMouseTracker.dll` | Global mouse hook (`WH_MOUSE_LL`), screen coordinate capture, mouse button state, pen vs mouse discrimination. |
| **`KeyboardTracker`** | C++ Win32 DLL | `FsKeyboardTracker.dll` | Global keyboard hook (`WH_KEYBOARD_LL`), 256 virtual key (VK) state tracking via bitsets. |
| **`UdpClientTest`** | .NET 8 Console | `UdpClientTest.exe` | Lightweight test receiver for verifying incoming UDP packet streams without Warudo. |

*(Note: `GamepadDebug` contains a standalone project `GamepadDebug.vcxproj` producing `FsGamepadDebug.exe` for raw joystick axis and button inspection).*

---

## 3. Subsystem Breakdown

### 3.1 Main Execution Flow & The Message Pump (`Emitter/Program.cpp`)

`FsInputEmitter.exe` operates either in **CLI mode** (flags parsed with `cxxopts`) or **Interactive mode** (guided CLI wizard built on `Inquirer.h`):

1. **Initialization**: Parses arguments or runs interactive prompts to determine which trackers to activate, destination IP addresses, and UDP ports.
2. **Tracker Setup**: Instantiates selected tracker classes (`MouseTracker`, `KeyboardTracker`, `DirectInputTracker`, `XInputTracker`, `GameInputTracker`) and calls their `setup()` methods.
3. **Emitter Setup**: Instantiates `Emitter` instances bound to target ports and configures broadcast sockets.
4. **Execution Loop (`start()`)**:
   - Runs a high-resolution timing loop using `std::chrono::steady_clock` with OS scheduler resolution locked to 1.0 ms via `timeBeginPeriod(1)`.
   - **Crucial Win32 Requirement & Zero-Spin Event Wakeup**: When mouse or keyboard trackers are active, the main thread drains all pending Windows messages using:
     ```cpp
     while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
     }
     ```
     Low-level Windows hooks (`WH_MOUSE_LL` / `WH_KEYBOARD_LL`) depend on the message queue of the installing thread.
   - If the emission interval hasn't elapsed, the thread sleeps with 0% CPU via `MsgWaitForMultipleObjectsEx(0, NULL, waitMs, QS_ALLINPUT, MWMO_ALERTABLE)`. This wakes up in <0.01ms the instant an input arrives, or at the exact interval deadline, eliminating CPU spin while retaining maximum responsiveness.
5. **Periodic Emission**: Every interval (default 30 ms / ~33 Hz), active trackers refresh their internal state (`refreshState()`) and broadcast UDP packets via zero-copy `emitter->send(std::string_view)`.
6. **Teardown (`cleanExit()`)**: Unhooks Windows hooks, frees dynamically loaded DLLs, releases COM interfaces, and restores timer resolution via `timeEndPeriod(1)`.

---

### 3.2 Dynamic Hook DLLs & Shared Memory Segments

Global hooks require separate DLL modules because Windows injects or marshals hook callbacks across processes:

#### Shared Memory Segments (`#pragma data_seg`)
Both `MouseTracker` and `KeyboardTracker` utilize named shared data segments in their DLLs to store state across process contexts:

```cpp
// MouseTracker/dllmain.cpp:
#pragma data_seg (".shared")
LONG x = 0;
LONG y = 0;
bool mouse1 = false;
bool mouse2 = false;
ULONG_PTR extraInfo = 0;
#pragma data_seg()
#pragma comment(linker, "/SECTION:.shared,RWS")

// KeyboardTracker/dllmain.cpp (POD array, zero static constructor risk):
#pragma data_seg (".sharedkb")
unsigned long long vkParts[4] = { 0, 0, 0, 0 };
#pragma data_seg()
#pragma comment(linker, "/SECTION:.sharedkb,RWS")
```

The linker directive `/SECTION:.shared,RWS` (Read, Write, Shared) ensures that every process instance mapping the DLL reads and writes to the exact same physical memory block.

#### Dynamic Loading Pipeline
The main executable `FsInputEmitter.exe` does not link against `FsMouseTracker.lib` or `FsKeyboardTracker.lib` at link time. Instead, it dynamically loads them via `LoadLibrary(L"FsMouseTracker.dll")` and retrieves procedure pointers using `GetProcAddress`:
- `LowLevelMouseProc` / `LowLevelKeyboardProc`: Passed to `SetWindowsHookEx(WH_MOUSE_LL / WH_KEYBOARD_LL, ...)`.
- `GetStateProc`: Called per-frame to extract the current shared struct snapshot (`State`).

---

### 3.3 Pen vs. Mouse Discrimination (`MouseTracker.cpp`)

To support drawing tablets (such as Wacom Intuos Pro) alongside traditional computer mice, the emitter inspects the `dwExtraInfo` field of the `tagMSLLHOOKSTRUCT` delivered by `WM_MOUSEMOVE`:

- **Mouse Movement**: Standard mice produce `dwExtraInfo == 0`.
- **Pen / Stylus**: Digitizers and tablet drivers (Wacom WinTab / Windows Ink) inject non-zero signatures into `dwExtraInfo`.
- **Software Traps**: Applications such as Clip Studio Paint (`742DEA58-ED6B-4402-BC11-20DFC6D08040`) and Blender (`GHOST_WindowClass`) may modify `dwExtraInfo`. Checking `st.extraInfo != 0` reliably identifies pen interaction across these apps.

---

### 3.4 Controller Subsystems

The emitter supports three distinct game controller APIs to cover modern gamepads, legacy devices, flight sticks, and steering wheels:

#### 1. GameInput (`GameInputTracker.cpp`)
- **API**: Modern Windows GameInput COM interface (`GameInputCreate(&gameInput)`).
- **Target**: Xbox Series X\|S, Xbox One, modern Windows-compatible controllers.
- **Two Operating Modes**:
  1. *High-level Gamepad Mode*: Calls `reading->GetGamepadState(&gamepadState)`. Yields standard buttons and normalized thumbsticks/triggers.
  2. *Low-level Generic Controller Mode*: Reads raw buttons (`GetControllerButtonState`), switches/hats (`GetControllerSwitchState`), and raw axes (`GetControllerAxisState`). Supports exotic hardware like Switch Pro controllers.
- **Disconnection Handling**: On reading failure (`FAILED(hr)`), releases `boundDevice` and attempts clean reacquisition on subsequent cycles.

#### 2. DirectInput 8 (`DirectInputTracker.cpp`)
- **API**: Classic DirectX DirectInput 8 (`DirectInput8Create`, `EnumDevices`, `CreateDevice`).
- **Target**: Generic gamepads, flight simulators, HOTAS, steering wheels, pedal boxes (e.g., Moza R9, Moza CS wheel).
- **Enumeration & GUID Binding**:
  - Enumerates attached game controllers (`DI8DEVCLASS_GAMECTRL`).
  - Formats user choices using device names and product GUIDs (`StringFromGUID2`).
- **Data Normalization**: Reads `DIJOYSTATE2`. Normalizes 8 axes (`lX`, `lY`, `lZ`, `lRx`, `lRy`, `lRz`, `rglSlider[0]`, `rglSlider[1]`) from raw integer ranges (0..65535) into standard floats (`0.0` to `1.0`). Encodes 4 POV hats (0..8) and 32 buttons.

#### 3. XInput (`XInputTracker.cpp`)
- **API**: Standard Windows XInput API (`XInputGetState`, `XInputGetCapabilities`).
- **Target**: Xbox 360 and standard Xbox-compatible gamepads (controller indices 0 through 3).
- **State Handling**:
  - Non-blocking polling: Disconnected slots return `ERROR_DEVICE_NOT_CONNECTED` without crashing.
  - Automatically recovers when a controller is re-plugged.
  - Normalizes signed thumbstick values (`-32768..32767`) to `0.0..1.0` (center at `0.5`) and analog triggers (`0..255`) to `0.0..1.0`.

---

### 3.5 Network Transmission Subsystem (`Emitter/Emitter.cpp`)

UDP communication is handled by a lightweight Winsock wrapper:

- **Socket Creation**: Creates IPv4 datagram sockets (`AF_INET`, `SOCK_DGRAM`).
- **Broadcast Support**: Explicitly enables `SO_BROADCAST` via `setsockopt`:
  ```cpp
  int enabled = 1;
  setsockopt(this->sock, SOL_SOCKET, SO_BROADCAST, (char*)&enabled, sizeof(enabled));
  ```
  This allows sending to `127.0.0.1` (unicast loopback), LAN IPs, or local broadcast (`255.255.255.255` / subnet broadcast).
- **Zero Memory Allocation in Emission Loop**: Pre-allocates destination `sockaddr_in` structures during construction. Packet payload strings are constructed and emitted directly with `sendto`.
- **Loss-Tolerant Streaming**: Packet delivery errors (`SOCKET_ERROR`) are logged without aborting the main application loop, ensuring continuous operation even during transient network interruptions.
