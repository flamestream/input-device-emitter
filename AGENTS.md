# AGENTS.md - Developer & AI Assistant Guide for FS Input Device Emitter

Welcome to the **FS Input Device Emitter** repository. This document provides technical architecture, build processes, protocol specifications, and coding conventions for AI agents and human developers.

---

## 1. Repository Overview & Purpose

**FS Input Device Emitter** is a high-performance Windows C++ application designed to capture real-time user inputs (pointer/pen/mouse, keyboard, modern and legacy game controllers, steering wheels, flight sticks) and stream them over **UDP** to receivers such as the **FS Input Receiver** plugin for the VTuber application **Warudo**.

---

## 2. Solution Architecture

The Visual Studio solution `InputEmitter.sln` is organized into several projects:

```
input-device-emitter/
├── Emitter/              # Main console application (FsInputEmitter.exe)
│   ├── Program.cpp       # CLI entrypoint, argument parsing (cxxopts), interactive inquirer mode, emission loop
│   ├── Emitter.cpp/.h    # Winsock UDP socket broadcaster wrapper
│   ├── MouseTracker.cpp  # Dynamic loader for FsMouseTracker.dll, pen vs. mouse discriminator
│   ├── KeyboardTracker.cpp # Dynamic loader for FsKeyboardTracker.dll, 256-key bitset manager
│   ├── GameInputTracker.cpp# Windows GameInput API reader (Xbox One / Series X|S)
│   ├── DirectInputTracker.cpp# DirectInput 8 reader (flight sticks, wheels, pedals, generic gamepads)
│   ├── XInputTracker.cpp # XInput reader (Xbox 360 / standard controllers)
│   ├── Console.cpp/.h    # ANSI console color & formatting helpers
│   ├── Utility.cpp/.h    # String/array formatters, HRESULT formatters
│   └── packages.config   # Microsoft.GameInput NuGet package reference
├── MouseTracker/         # C++ DLL (FsMouseTracker.dll)
│   └── dllmain.cpp       # WH_MOUSE_LL low-level global hook callback & GetStateProc export
├── KeyboardTracker/      # C++ DLL (FsKeyboardTracker.dll)
│   └── dllmain.cpp       # WH_KEYBOARD_LL low-level global hook callback & GetStateProc export
├── GamepadDebug/         # Standalone diagnostic console tool (FsGamepadDebug.exe)
├── UdpClientTest/        # .NET 8 test receiver for UDP packet validation
├── docs/                 # Architectural and protocol documentation
│   └── protocol-specification.md # Detailed UDP packet format specifications
├── build.ps1             # Automated single-command build script
└── InputEmitter.sln      # Visual Studio 2022 Solution
```

---

## 3. Network & UDP Protocol Specification

Data packets are transmitted via UDP to `127.0.0.1` (or a configured remote IP) using protocol version `2`.

| Input Type | Default Port | Tag | Payload Format |
| :--- | :--- | :--- | :--- |
| **Pointer (Mouse/Pen)** | `40610` | `2P` | `2P;<x>;<y>;<source>;<mouse1>;<mouse2>`<br>• `source`: `0` = Mouse, `1` = Pen, `2` = Touch |
| **DirectInput** | `40611` | `2D` | `2D;<32-bit buttons>;<dpad0>\|<dpad1>\|<dpad2>\|<dpad3>;<8 pipe-delimited normalized floats>` |
| **Keyboard** | `40612` | `2K` | `2K;<part1_64bit>;<part2_64bit>;<part3_64bit>;<part4_64bit>` (256 VK keys) |
| **GameInput** | `40613` | `2G` | `2G;<buttons_bin>;<switches>;<6 pipe-delimited normalized floats>` |
| **XInput** | `40614` | `2X` | `2X;<buttons_bin>;;<6 pipe-delimited normalized floats>` |

> For full field definitions, refer to [`docs/protocol-specification.md`](file:///d:/Code/input-device-emitter/docs/protocol-specification.md).

---

## 4. Build and Verification Workflow

### 4.1 One-Command Build (Recommended)
Run the PowerShell build script from the repository root:
```powershell
.\build.ps1 -Configuration Release -Platform x64
```
This automatically:
1. Downloads `nuget.exe` if not present.
2. Restores native packages (`Microsoft.GameInput`).
3. Discovers `MSBuild.exe` via Visual Studio `vswhere`.
4. Compiles all C++ projects and C# test clients into `x64/Release/`.

### 4.2 Manual MSBuild Command
```powershell
# 1. Restore NuGet packages
.\nuget.exe restore InputEmitter.sln

# 2. Build solution
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" InputEmitter.sln /p:Configuration=Release /p:Platform=x64
```

### 4.3 Output Binaries
- `x64/Release/FsInputEmitter.exe`
- `x64/Release/FsMouseTracker.dll` (Must be in the same folder as `FsInputEmitter.exe`)
- `x64/Release/FsKeyboardTracker.dll` (Must be in the same folder as `FsInputEmitter.exe`)
- `x64/Release/FsGamepadDebug.exe`
- `UdpClientTest/bin/Release/net8.0/UdpClientTest.exe`

---

## 5. Key Implementation Guidelines & Gotchas

1. **Windows Hook DLLs (`WH_MOUSE_LL` / `WH_KEYBOARD_LL`)**:
   - `FsMouseTracker.dll` and `FsKeyboardTracker.dll` must export `LowLevelMouseProc`/`LowLevelKeyboardProc` and `GetStateProc`.
   - Global hooks run in the context of the calling thread. The hook callbacks must be extremely lightweight to avoid introducing OS-level input lag.
   - If an application runs with elevated privileges (Administrator), the emitter must also run elevated to intercept inputs due to Windows UIPI (User Interface Privilege Isolation).

2. **Pen vs. Mouse Discrimination**:
   - In `MouseTracker.cpp`, `st.extraInfo != 0` indicates pen/stylus input (e.g. Wacom). Standard mouse movements produce `st.extraInfo == 0`.

3. **Controller Subsystems**:
   - `GameInput`: Modern Windows API (`Microsoft.GameInput` NuGet package). Requires `GameInputCreate(&gameInput)` and safe COM `Release()` handling.
   - `DirectInput`: DirectInput 8 (`dinput8.lib`, `dxguid.lib`). Enumerate devices with `EnumDevices(DI8DEVCLASS_GAMECTRL, ...)`.
   - `XInput`: `XInputGetState(controllerIndex, &newState)` supporting controllers 0–3.

4. **Resource Management**:
   - Always invoke `teardown()` on trackers and emitters during exit (`cleanExit()`).
   - Clean up Winsock (`WSACleanup()`) and unhook Windows hooks (`UnhookWindowsHookEx`) to prevent orphan hooks.

5. **C++ Standard**:
   - The codebase uses **C++20** (`std::format`, `<chrono>`, `<thread>`, `<bitset>`). Ensure MSVC `/std:c++20` or `/std:c++latest` is used.
