# AGENTS.md — AI Assistant Guide for FS Input Device Emitter

> **For detailed architecture, runtime gotchas, and troubleshooting, read the [`.ai/` knowledge base](.ai/README.md) first.**
>
> **CRITICAL DIRECTIVE — Inconsistency Verification & Pushback**:
> If you notice an inconsistency or potential conflict in any statement or requirement that can be verified against active code, build scripts, or protocol definitions, **do not attempt to blindly accommodate it or force it to work**. Push back with verifiable evidence, explain the conflict clearly, and ask for clarification.

---

## 1. Repository Overview & Purpose

**FS Input Device Emitter** is a high-performance Windows C++ application designed to capture real-time user inputs (pointer/pen/mouse, keyboard, modern and legacy game controllers, steering wheels, flight sticks) and stream them over **UDP** to receivers such as the **FS Input Receiver** plugin for the VTuber application **Warudo**.

---

## 2. Solution Architecture

The Visual Studio solution [`InputEmitter.sln`](file:///d:/Code/input-device-emitter/InputEmitter.sln) is organized into several projects:

```
input-device-emitter/
├── Emitter/              # Main console application (FsInputEmitter.exe)
│   ├── Program.cpp       # CLI entrypoint, argument parsing (cxxopts), interactive inquirer mode, emission loop
│   ├── Emitter.cpp/.h    # Winsock UDP socket broadcaster wrapper (SO_BROADCAST)
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
├── .ai/                  # Engineering knowledge base (read before making changes)
├── build.ps1             # Automated single-command build script
└── InputEmitter.sln      # Visual Studio 2022 Solution
```

---

## 3. Network & UDP Protocol Specification

Data packets are transmitted via UDP to `127.0.0.1` (or a configured remote/broadcast IP).

| Input Type | Default Port | Active Tag | Payload Format |
| :--- | :---: | :---: | :--- |
| **Pointer (Mouse/Pen)** | `40610` | `1P` | `1P;<x>;<y>;<source>;<mouse1>;<mouse2>`<br>• `source`: `0` = Mouse, `1` = Pen, `2` = Touch |
| **DirectInput** | `40611` | `3D` | `3D;<32-bit buttons>;<dpad0>\|<dpad1>\|<dpad2>\|<dpad3>;<8 pipe-delimited normalized floats>` |
| **Keyboard** | `40612` | `1K` | `1K;<part1_64bit>;<part2_64bit>;<part3_64bit>;<part4_64bit>` (256 VK keys) |
| **GameInput** | `40613` | `2G` | `2G;<buttons_bin>;<switches>;<6 pipe-delimited normalized floats>` |
| **XInput** | `40614` | `1X` | `1X;<buttons_bin>;;<6 pipe-delimited normalized floats>` |

> For full field definitions and channel breakdowns, refer to [protocol_and_networking_guide.md](file:///d:/Code/input-device-emitter/.ai/protocol_and_networking_guide.md) and [protocol-specification.md](file:///d:/Code/input-device-emitter/docs/protocol-specification.md).

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
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" InputEmitter.sln /p:Configuration=Release /p:Platform=x64 /m
```

### 4.3 Output Binaries
- `x64/Release/FsInputEmitter.exe`
- `x64/Release/FsMouseTracker.dll` (Must be in the same folder as `FsInputEmitter.exe`)
- `x64/Release/FsKeyboardTracker.dll` (Must be in the same folder as `FsInputEmitter.exe`)
- `x64/Release/FsGamepadDebug.exe`
- `UdpClientTest/bin/Release/net8.0/UdpClientTest.exe`

---

## 5. Critical Implementation Rules & Gotchas

Full explanations and code patterns are in [`.ai/native_runtime_gotchas.md`](file:///d:/Code/input-device-emitter/.ai/native_runtime_gotchas.md).

### 5.1 Win32 Message Pump Required for Global Hooks
Low-level hooks (`WH_MOUSE_LL` / `WH_KEYBOARD_LL`) require an active `PeekMessage` loop running on the thread that called `SetWindowsHookEx`. Never put an unthrottled `sleep_for` call on this thread. → [Gotcha #1](.ai/native_runtime_gotchas.md#gotcha-1-win32-message-pump-peekmessage-requirement-for-global-hooks)

### 5.2 UIPI Privilege Boundaries
Non-elevated processes cannot intercept inputs over elevated (Administrator) windows due to Windows User Interface Privilege Isolation. Run as Administrator if global capture across elevated windows is required. → [Gotcha #2](.ai/native_runtime_gotchas.md#gotcha-2-windows-user-interface-privilege-isolation-uipi--elevated-window-blind-spots)

### 5.3 Low-Level Hook Timeout
Windows silently discards hooks if callbacks take longer than ~200–300 ms. Never perform I/O, console logging, or allocations inside `LowLevelMouseProc` / `LowLevelKeyboardProc`. → [Gotcha #3](.ai/native_runtime_gotchas.md#gotcha-3-windows-low-level-hook-timeout-lowlevelhookstimeout--silent-dropping)

### 5.4 DLL Shared Data Segments
All variables declared inside `#pragma data_seg(".shared")` **must be explicitly initialized** or MSVC places them in unshared memory. Link with `/SECTION:...,RWS`. → [Gotcha #4](.ai/native_runtime_gotchas.md#gotcha-4-dll-shared-data-segment-pragma-data_seg-initialization-rule)

### 5.5 DLL Working Directory Co-location
`LoadLibrary(L"FsMouseTracker.dll")` searches the working directory. Ensure DLLs are in the same folder as `FsInputEmitter.exe`. → [Gotcha #5](.ai/native_runtime_gotchas.md#gotcha-5-working-directory-sensitivity-for-dll-dynamic-loading)

### 5.6 Pen vs. Mouse Discrimination
`st.extraInfo != 0` distinguishes pen/stylus inputs (Wacom, Windows Ink) from standard mice. Drawing applications (Clip Studio Paint, Blender) inject non-zero data into `dwExtraInfo`. → [Gotcha #6](.ai/native_runtime_gotchas.md#gotcha-6-stylus-vs-mouse-discrimination-via-dwextrainfo)

### 5.7 Multi-Monitor Virtual Screen Space
Coordinates can be negative when monitors sit to the left of or above the primary monitor. Treat `x` and `y` as signed integers. → [Gotcha #7](.ai/native_runtime_gotchas.md#gotcha-7-multi-monitor-virtual-desktop-negative-coordinates)

### 5.8 GameInput Reconnection Handling
Always release and nullify `boundDevice = nullptr` when `GetCurrentReading` fails so reconnected controllers can be discovered. → [Gotcha #9](.ai/native_runtime_gotchas.md#gotcha-9-gameinput-com-api-lifecycle--reconnection-quirks)

### 5.9 Clean Process Teardown
Always invoke `UnhookWindowsHookEx` and `WSACleanup()` on exit to prevent system-wide input lag from orphan hooks. → [Gotcha #15](.ai/native_runtime_gotchas.md#gotcha-15-process-teardown--unhooking-to-prevent-orphan-hooks)

---

## 6. Knowledge Base Index

| File | Covers |
|:---|:---|
| [architecture_and_subsystems.md](file:///d:/Code/input-device-emitter/.ai/architecture_and_subsystems.md) | Solution structure, Win32 message pump, shared memory DLLs, COM APIs, Winsock |
| [protocol_and_networking_guide.md](file:///d:/Code/input-device-emitter/.ai/protocol_and_networking_guide.md) | Channel ports (`40610`..`40614`), packet layouts (`1P`,`3D`,`1K`,`2G`,`1X`), normalization |
| [development_cheatsheet.md](file:///d:/Code/input-device-emitter/.ai/development_cheatsheet.md) | Tracker lifecycle pattern, CLI options (`cxxopts`), Inquirer prompts, C++20 conventions |
| [debugging_and_troubleshooting.md](file:///d:/Code/input-device-emitter/.ai/debugging_and_troubleshooting.md) | `UdpClientTest`, `GamepadDebug`, PowerShell one-liners, Wireshark filters, error codes |
| [native_runtime_gotchas.md](file:///d:/Code/input-device-emitter/.ai/native_runtime_gotchas.md) | 20 detailed runtime quirks, driver traps, UIPI, hook timeouts, and serialization gotchas |
