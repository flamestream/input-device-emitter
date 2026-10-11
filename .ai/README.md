# FS Input Device Emitter — AI & Engineering Knowledge Base

This directory contains key architecture notes, protocol specifications, subsystem deep-dives, developer cheatsheets, and troubleshooting workflows for developing and maintaining the **FS Input Device Emitter** C++ Windows application.

---

## Documents

1. [architecture_and_subsystems.md](file:///d:/Code/input-device-emitter/.ai/architecture_and_subsystems.md)
   - High-level system architecture and data flow from Windows OS inputs to UDP socket broadcast.
   - Solution structure (`InputEmitter.sln`, `Emitter`, `MouseTracker`, `KeyboardTracker`, `UdpClientTest`, `GamepadDebug`).
   - Win32 Message Pump (`PeekMessage`) requirement for low-level hooks (`WH_MOUSE_LL` / `WH_KEYBOARD_LL`).
   - Shared memory segments (`#pragma data_seg(".shared")` / `/SECTION:... ,RWS`) and dynamic DLL loading.
   - Pen vs. mouse discrimination heuristics via `dwExtraInfo` (Wacom, Clip Studio Paint, Blender).
   - Controller subsystems: GameInput COM API, DirectInput 8 GUIDs & axes, XInput 4-controller polling.
   - Winsock UDP broadcast subsystem (`SO_BROADCAST`, IPv4, fire-and-forget streaming).

2. [protocol_and_networking_guide.md](file:///d:/Code/input-device-emitter/.ai/protocol_and_networking_guide.md)
   - Detailed UDP network protocol specifications across all channels.
   - Channel-by-channel packet format, delimiters, normalization ranges, and payload examples:
     - Pointer Channel (`40610`, tag `1P`): Screen coordinates, device source (Mouse/Pen/Touch), mouse 1 & 2.
     - Keyboard Channel (`40612`, tag `1K`): 256 Virtual Key states packed into four 64-bit binary strings.
     - GameInput Channel (`40613`, tag `2G`): Gamepad vs generic controller modes, 32-bit buttons, 6 float axes.
     - DirectInput Channel (`40611`, tag `3D`): 32 buttons bitstring, 4 POV hats (0..8), 8 normalized float axes.
     - XInput Channel (`40614`, tag `1X`): 32-bit buttons, 6 normalized float axes.
   - Multi-monitor virtual desktop coordinates (negative screen origins).
   - Packet timing and emission frequency (default 30ms / ~33.3 Hz).
   - Backward compatibility and receiver validation in Warudo (`CheckValidProtocolVersion`).

3. [development_cheatsheet.md](file:///d:/Code/input-device-emitter/.ai/development_cheatsheet.md)
   - Fast build and test commands with `build.ps1` and MSBuild.
   - Solution structure and project roles.
   - Standardized Tracker lifecycle pattern (`setup()`, `refreshState()`, `writeState()`, `teardown()`).
   - Hook DLL pattern (`dllmain.cpp`, exported procedures, shared segments).
   - Adding new command-line options (`cxxopts`) and interactive prompts (`Inquirer.h`).
   - ANSI console logging conventions (`Console.h`).
   - C++20 standards, MSVC compiler flags, and memory management rules.

4. [debugging_and_troubleshooting.md](file:///d:/Code/input-device-emitter/.ai/debugging_and_troubleshooting.md)
   - Diagnostic tools: `UdpClientTest` (.NET 8), `GamepadDebug` (`FsGamepadDebug.exe`), and PowerShell UDP one-liners.
   - Emitter CLI diagnostic flags (`--debug`, `--show-shutdown-message`, `--freq`).
   - Common error signatures and solutions:
     - DLL loading failure (`Error 126: ERROR_MOD_NOT_FOUND`).
     - UIPI elevation blind spots on Administrator windows.
     - GameInput reading failure and device reconnection.
     - Winsock socket errors (`Error 10048: WSAEADDRINUSE`).
     - Tablet stylus detected as standard mouse.
   - Network packet inspection with Wireshark.
   - Attaching Visual Studio or WinDbg to the running process.

5. [native_runtime_gotchas.md](file:///d:/Code/input-device-emitter/.ai/native_runtime_gotchas.md)
   - 20 real-world runtime quirks, driver pitfalls, and Windows API gotchas:
     - Win32 Message Pump requirement for global hooks (Gotcha #1).
     - UIPI privilege boundaries and Administrator windows (Gotcha #2).
     - Windows Low-Level Hook Timeout (`LowLevelHooksTimeout`) silent drop (Gotcha #3).
     - DLL shared data segment initialization rule (Gotcha #4).
     - Working directory sensitivity for DLL loading (Gotcha #5).
     - Stylus vs. mouse discrimination via `dwExtraInfo` (Gotcha #6).
     - Multi-monitor virtual desktop negative coordinates (Gotcha #7).
     - DPI scaling and virtual screen coordinate bounds (Gotcha #8).
     - GameInput COM lifecycle and device reconnection recovery (Gotcha #9).
     - DirectInput device index reordering across USB reconnection (Gotcha #10).
     - XInput 4-controller limit and polling cost (Gotcha #11).
     - UDP socket broadcast flag (`SO_BROADCAST`) permissions (Gotcha #12).
     - Keyboard 256 VK packing order and bitset string formatting (Gotcha #13).
     - DirectInput POV hat rotational mapping 0..8 (Gotcha #14).
     - Process teardown and unhooking to prevent orphan hooks (Gotcha #15).
     - Protocol version discrepancies: active code vs. legacy docs (Gotcha #16).
     - C++20 standard conformance with MSVC (Gotcha #17).
     - NuGet package restore requirement for `Microsoft.GameInput` (Gotcha #18).
     - Windows console virtual terminal processing for ANSI escape sequences (Gotcha #19).
     - Memory alignment and inter-process data consistency (Gotcha #20).
