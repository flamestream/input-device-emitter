# Native Runtime Gotchas — FS Input Device Emitter

This document catalogs real-world runtime quirks, driver traps, Windows API behaviors, and networking pitfalls uncovered while developing and debugging **FS Input Device Emitter**.

---

## Index of Gotchas

- [Gotcha #1: Win32 Message Pump (`PeekMessage`) Requirement for Global Hooks](#gotcha-1-win32-message-pump-peekmessage-requirement-for-global-hooks)
- [Gotcha #2: Windows User Interface Privilege Isolation (UIPI) & Elevated Window Blind Spots](#gotcha-2-windows-user-interface-privilege-isolation-uipi--elevated-window-blind-spots)
- [Gotcha #3: Windows Low-Level Hook Timeout (`LowLevelHooksTimeout`) & Silent Dropping](#gotcha-3-windows-low-level-hook-timeout-lowlevelhookstimeout--silent-dropping)
- [Gotcha #4: DLL Shared Data Segment (`#pragma data_seg`) Initialization Rule](#gotcha-4-dll-shared-data-segment-pragma-data_seg-initialization-rule)
- [Gotcha #5: Working Directory Sensitivity for DLL Dynamic Loading](#gotcha-5-working-directory-sensitivity-for-dll-dynamic-loading)
- [Gotcha #6: Stylus vs. Mouse Discrimination via `dwExtraInfo`](#gotcha-6-stylus-vs-mouse-discrimination-via-dwextrainfo)
- [Gotcha #7: Multi-Monitor Virtual Desktop Negative Coordinates](#gotcha-7-multi-monitor-virtual-desktop-negative-coordinates)
- [Gotcha #8: DPI Scaling & Virtualized Screen Coordinates](#gotcha-8-dpi-scaling--virtualized-screen-coordinates)
- [Gotcha #9: GameInput COM API Lifecycle & Reconnection Quirks](#gotcha-9-gameinput-com-api-lifecycle--reconnection-quirks)
- [Gotcha #10: DirectInput Device Index Reordering Across USB Reconnection](#gotcha-10-directinput-device-index-reordering-across-usb-reconnection)
- [Gotcha #11: XInput 4-Controller Limit & Polling Cost for Disconnected Slots](#gotcha-11-xinput-4-controller-limit--polling-cost-for-disconnected-slots)
- [Gotcha #12: UDP Socket Broadcast Flag (`SO_BROADCAST`) & Access Violations](#gotcha-12-udp-socket-broadcast-flag-so_broadcast--access-violations)
- [Gotcha #13: Keyboard 256 Virtual Key (VK) Packing Order & Bitset String Formatting](#gotcha-13-keyboard-256-virtual-key-vk-packing-order--bitset-string-formatting)
- [Gotcha #14: DirectInput POV Hat Mapping (0 = Center, 1..8 Clockwise)](#gotcha-14-directinput-pov-hat-mapping-0--center-18-clockwise)
- [Gotcha #15: Process Teardown & Unhooking to Prevent Orphan Hooks](#gotcha-15-process-teardown--unhooking-to-prevent-orphan-hooks)
- [Gotcha #16: Protocol Version Discrepancies (Theoretical Docs vs. Active Code)](#gotcha-16-protocol-version-discrepancies-theoretical-docs-vs-active-code)
- [Gotcha #17: C++20 Standard & Visual Studio MSVC Conformance](#gotcha-17-c20-standard--visual-studio-msvc-conformance)
- [Gotcha #18: NuGet Package Restore Requirement for `Microsoft.GameInput`](#gotcha-18-nuget-package-restore-requirement-for-microsoftgameinput)
- [Gotcha #19: Windows Console Virtual Terminal Processing (ANSI Escape Codes)](#gotcha-19-windows-console-virtual-terminal-processing-ansi-escape-codes)
- [Gotcha #20: Memory Alignment & Inter-Process Data Consistency](#gotcha-20-memory-alignment--inter-process-data-consistency)

---

### Gotcha #1: Win32 Message Pump (`PeekMessage`) Requirement for Global Hooks

#### Symptom
When `MouseTracker` or `KeyboardTracker` is active, hook procedures are installed (`SetWindowsHookEx` returns valid handles), but **no mouse movements, clicks, or key presses are ever captured**.

#### Root Cause
Windows low-level hooks (`WH_MOUSE_LL` and `WH_KEYBOARD_LL`) do not execute on arbitrary worker threads. The OS posts hook notifications to the message queue of the specific thread that installed the hook via `SetWindowsHookEx`. If that thread does not continuously pump messages, the callbacks are never invoked.

#### Solution
The main emission loop in [`Emitter/Program.cpp`](file:///d:/Code/input-device-emitter/Emitter/Program.cpp) must explicitly pump the Win32 message queue:

```cpp
if (mouseTracker || keyboardTracker) {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
} else {
    // If only controller trackers are active, sleep optimization can be safely used
    std::this_thread::sleep_for(wantedInterval - elapsed);
}
```

> [!WARNING]
> Never put an unthrottled `sleep_for` call on the thread running `WH_MOUSE_LL` or `WH_KEYBOARD_LL`. If the thread is sleeping, the mouse hook cannot process events in real time.

---

### Gotcha #2: Windows User Interface Privilege Isolation (UIPI) & Elevated Window Blind Spots

#### Symptom
The emitter tracks mouse and keyboard normally, but as soon as the user focuses an elevated application (e.g., Task Manager, Registry Editor, an elevated terminal, or games running as Administrator), **all tracking instantly freezes**.

#### Root Cause
Windows User Interface Privilege Isolation (UIPI) prevents lower-integrity processes from intercepting or injecting window messages into higher-integrity processes. Standard user applications cannot hook input destined for Administrator-level windows.

#### Solution
Launch `FsInputEmitter.exe` as Administrator when you need to capture inputs globally across elevated windows or full-screen administrative games.

---

### Gotcha #3: Windows Low-Level Hook Timeout (`LowLevelHooksTimeout`) & Silent Dropping

#### Symptom
The emitter runs fine for a short period, then suddenly stops receiving mouse or keyboard events altogether without throwing any error or exception.

#### Root Cause
Windows monitors the execution time of low-level hook callbacks. If a callback exceeds the system threshold specified in the registry (`HKEY_CURRENT_USER\Control Panel\Desktop\LowLevelHooksTimeout`, typically 200–300 ms), **the operating system silently unhooks the application** without notifying it.

#### Solution
Keep `LowLevelMouseProc` and `LowLevelKeyboardProc` completely non-blocking:
1. Never perform file I/O, network requests, console printing, or memory allocations inside the hook callback.
2. Only write primitive values to shared variables and immediately return `CallNextHookEx(...)`.

---

### Gotcha #4: DLL Shared Data Segment (`#pragma data_seg`) Initialization Rule

#### Symptom
Different processes running the hook DLL see isolated variables; `GetStateProc()` returns zero or garbage coordinates even though the hook callback is running in target processes.

#### Root Cause
Under MSVC, variables declared inside `#pragma data_seg(".shared")` **must be explicitly initialized**:
```cpp
#pragma data_seg (".shared")
LONG x = 0;          // Correct: placed in .shared segment
LONG y;              // BUG: Uninitialized! MSVC puts this in standard .bss segment!
#pragma data_seg()
#pragma comment(linker, "/SECTION:.shared,RWS")
```
Uninitialized variables are omitted from the shared segment and default to per-process instance storage.

#### Solution
Always provide an explicit initializer (e.g., `= 0`, `= false`) for every variable in a shared data segment.

---

### Gotcha #5: Working Directory Sensitivity for DLL Dynamic Loading

#### Symptom
Running `FsInputEmitter.exe` from a shortcut, task scheduler, or different working directory crashes on startup with:
```text
Mouse Tracker DLL could not be loaded: Error 126
```

#### Root Cause
`MouseTracker.cpp` and `KeyboardTracker.cpp` load DLLs via:
```cpp
LoadLibrary(L"FsMouseTracker.dll");
```
If the current working directory differs from the directory containing `FsInputEmitter.exe`, Windows DLL search order will fail to locate the DLL unless it is in the PATH.

#### Solution
Ensure `FsMouseTracker.dll` and `FsKeyboardTracker.dll` reside in the exact same directory as `FsInputEmitter.exe`, and ensure working directory is set to that folder when invoking via scripts or shortcuts.

---

### Gotcha #6: Stylus vs. Mouse Discrimination via `dwExtraInfo`

#### Symptom
Graphics tablets (Wacom, Huion, XP-Pen) are reported as standard mouse input (`source = 0`) instead of pen input (`source = 1`).

#### Root Cause
In `MouseTracker.cpp`, input discrimination relies on `tagMSLLHOOKSTRUCT::dwExtraInfo`:
- Standard mouse events generate `dwExtraInfo == 0`.
- Tablet drivers, Windows Ink, and digitizers inject non-zero hardware signature bits into `dwExtraInfo`.
- If the tablet driver is running in legacy "Mouse Mode" instead of "Pen Mode" / "Windows Ink", `dwExtraInfo` remains 0.

#### Solution
In the tablet driver properties (e.g., Wacom Desktop Center), ensure **Use Windows Ink** is enabled for the target drawing software.

---

### Gotcha #7: Multi-Monitor Virtual Desktop Negative Coordinates

#### Symptom
When moving the cursor to secondary monitors positioned to the left of or above the primary monitor, coordinates overflow or cause parsing errors in receivers.

#### Root Cause
Windows assigns `(0, 0)` to the top-left corner of the **Primary Display**. Any monitor positioned to the left or above will have negative pixel coordinates (e.g., `x = -1920`, `y = -1080`).

#### Solution
1. `FsInputEmitter` formats `x` and `y` as signed integers (`%d` / `{}`):
   ```cpp
   std::format("{}P;{};{};{};{};{}", PROTOCOL_VERSION, this->st.x, this->st.y, ...);
   ```
2. Any UDP receiver must parse `x` and `y` using signed integer parsers (`int.Parse()` in C#), never unsigned types.

---

### Gotcha #8: DPI Scaling & Virtualized Screen Coordinates

#### Symptom
Cursor coordinates reported on 4K or high-DPI displays do not match physical window pixel boundaries or feel offset.

#### Root Cause
Low-level mouse hook points (`info->pt.x`, `info->pt.y`) are delivered in virtual screen coordinates subject to DPI virtualization if the process is not Per-Monitor DPI aware.

#### Solution
`FsInputEmitter.exe` and `FsGamepadDebug.exe` rely on Windows desktop virtual screen metrics. Receiver plugins in Warudo calibrate these coordinates by mapping the virtual screen bounding box to 3D world/prop anchor dimensions.

---

### Gotcha #9: GameInput COM API Lifecycle & Reconnection Quirks

#### Symptom
Disconnecting and reconnecting an Xbox controller causes `GameInput has failed to get reading` errors to loop indefinitely without reconnecting.

#### Root Cause
When a device disconnects, `GetCurrentReading(...)` returns an error code. If the application continues to hold a reference to `boundDevice`, GameInput cannot cleanly re-bind to the new device instance:
```cpp
hr = gameInput->GetCurrentReading(GameInputKindGamepad, boundDevice, &reading);
if (FAILED(hr)) {
    if (boundDevice) {
        boundDevice->Release();
        boundDevice = nullptr;  // Must be cleared!
    }
    return false;
}
```

#### Solution
Always release and nullify `boundDevice` immediately upon reading failure so that subsequent calls re-acquire the newly connected device.

---

### Gotcha #10: DirectInput Device Index Reordering Across USB Reconnection

#### Symptom
A steering wheel bound to `--directinput-index 0` suddenly controls a flight stick or generic gamepad after a Windows reboot.

#### Root Cause
DirectInput device index enumeration order (`EnumDevices`) is determined by Windows Plug-and-Play enumeration, not persistent physical hardware ports. Unplugging and re-plugging USB devices changes their index.

#### Solution
In interactive mode, the emitter displays product names and GUIDs (`StringFromGUID2`). In production setups, keep USB device topologies stable or verify device names before binding.

---

### Gotcha #11: XInput 4-Controller Limit & Polling Cost for Disconnected Slots

#### Symptom
Attempting to connect to controller index 4 fails; or polling an unplugged controller causes micro-stutters.

#### Root Cause
- The XInput API strictly supports controller indices 0 through 3 (`XUSER_MAX_COUNT = 4`).
- Calling `XInputGetState` on a disconnected controller index causes internal kernel driver queries that take measurable time.

#### Solution
`XInputTracker` caches the connection status. When disconnected, it emits resting default packets (`0.500000|0.500000|...`) without repeatedly stalling the loop.

---

### Gotcha #12: UDP Socket Broadcast Flag (`SO_BROADCAST`) & Access Violations

#### Symptom
Calling `sendto` to `255.255.255.255` or subnet broadcast addresses fails with `WSAGetLastError() == 10013 (WSAEACCES)`.

#### Root Cause
Winsock explicitly blocks broadcast destinations on UDP sockets unless `SO_BROADCAST` is set.

#### Solution
`Emitter::setup()` enables `SO_BROADCAST` unconditionally:
```cpp
int enabled = 1;
setsockopt(this->sock, SOL_SOCKET, SO_BROADCAST, (char*)&enabled, sizeof(enabled));
```

---

### Gotcha #13: Keyboard 256 Virtual Key (VK) Packing Order & Bitset String Formatting

#### Symptom
Pressing key `A` causes the receiver to highlight key `Z`, or keys appear inverted.

#### Root Cause
`std::bitset<64>::to_string()` formats bit `63` at index `0` of the string and bit `0` at index `63` (most significant bit first). If a receiver parses the string from left-to-right assuming index 0 is bit 0, keys are reversed.

#### Solution
In the receiver (e.g. Warudo `KeyboardReceiverAsset`), keys are decoded according to the matching bitset chunk layout:
- `part1` = VK `0x00`..`0x3F`
- `part2` = VK `0x40`..`0x7F`
- `part3` = VK `0x80`..`0xBF`
- `part4` = VK `0xC0`..`0xFF`

---

### Gotcha #14: DirectInput POV Hat Mapping (0 = Center, 1..8 Clockwise)

#### Symptom
D-pad inputs on steering wheels or flight sticks register unexpected directions in Warudo.

#### Root Cause
DirectInput represents POV hats in hundredths of degrees (`0` to `35900`, or `-1` when centered). `DirectInputTracker` converts this into a 9-state integer:
- `0`: Centered
- `1`: Up (0°)
- `2`: Up-Right (45°)
- `3`: Right (90°)
- `4`: Down-Right (135°)
- `5`: Down (180°)
- `6`: Down-Left (225°)
- `7`: Left (270°)
- `8`: Up-Left (315°)

#### Solution
Receivers must map `0` to neutral, and `1..8` to 45° angular sectors clockwise starting from Up.

---

### Gotcha #15: Process Teardown & Unhooking to Prevent Orphan Hooks

#### Symptom
Killing `FsInputEmitter.exe` via Task Manager causes mouse or keyboard stuttering for several seconds across the entire operating system.

#### Root Cause
If a process holding a low-level hook terminates without calling `UnhookWindowsHookEx`, Windows attempts to call the dead hook for pending input events until its internal timeout expires and the dead hook is discarded.

#### Solution
Always exit the emitter gracefully (Ctrl+C or Enter in interactive mode) so that `cleanExit()` invokes:
```cpp
UnhookWindowsHookEx(this->hWhMouseLl);
UnhookWindowsHookEx(this->hWhKeyboardLl);
WSACleanup();
```

---

### Gotcha #16: Protocol Version Discrepancies (Theoretical Docs vs. Active Code)

#### Symptom
Warudo `ReceiverAsset` rejects packets with:
```text
Invalid protocol format '1P;...'. Expected '2P' or '2'
```

#### Root Cause
Some legacy design documents specified protocol tag `2` for all devices. In the actual codebase, each tracker defines its own version constant:
- Pointer: `1P`
- Keyboard: `1K`
- DirectInput: `3D`
- GameInput: `2G`
- XInput: `1X`

#### Solution
Always verify protocol tags against the actual C++ tracker header definitions (`PROTOCOL_VERSION`) and ensure receiver assets in Warudo expect matching versions.

---

### Gotcha #17: C++20 Standard & Visual Studio MSVC Conformance

#### Symptom
Compilation errors on `std::format` or `<bitset>`:
```text
error C2039: 'format': is not a member of 'std'
```

#### Root Cause
`std::format` requires C++20 standard conformance (`/std:c++20` or `/std:c++latest`) and MSVC v143 toolset (Visual Studio 2022).

#### Solution
The Visual Studio project files (`Emitter.vcxproj`) explicitly set `<LanguageStandard>stdcpp20</LanguageStandard>`. Do not downgrade the language standard.

---

### Gotcha #18: NuGet Package Restore Requirement for `Microsoft.GameInput`

#### Symptom
Build fails with:
```text
fatal error C1083: Cannot open include file: 'GameInput.h': No such file or directory
```

#### Root Cause
GameInput headers and libraries are packaged via NuGet (`packages.config`). Cloning a clean repository does not include the restored headers.

#### Solution
Run `.\build.ps1` or run `.\nuget.exe restore InputEmitter.sln` prior to building with MSBuild.

---

### Gotcha #19: Windows Console Virtual Terminal Processing (ANSI Escape Codes)

#### Symptom
Console output displays raw escape characters like `\x1b[32m` instead of colored text.

#### Root Cause
Windows console hosts (`cmd.exe`) historically have ANSI escape sequence processing disabled by default.

#### Solution
[`Console::saveAttributes()`](file:///d:/Code/input-device-emitter/Emitter/Console.cpp) explicitly enables `ENABLE_VIRTUAL_TERMINAL_PROCESSING` on the standard output handle using `SetConsoleMode`.

---

### Gotcha #20: Memory Alignment & Inter-Process Data Consistency

#### Symptom
Data read from `GetStateProc()` displays shifted coordinates or corrupted flags between the DLL and host executable.

#### Root Cause
The `struct State` defined in `dllmain.cpp` must match the `struct State` defined in the tracker class byte-for-byte in memory layout and compiler packing.

#### Solution
Keep `State` structs composed solely of primitive, standard-width types (`LONG`, `bool`, `ULONG_PTR`, `unsigned long long`). Never put dynamic containers (`std::string`, `std::vector`) inside the shared memory segment or `State` structs.

---

### Gotcha #21: Numpad Enter Collapsing to `VK_RETURN` (13) vs Warudo `VKCode.NUMPAD_RETURN` (14)

#### Symptom
Pressing the Numpad Enter key on a physical keyboard animates the main keyboard Enter key in Warudo; the Numpad Enter 3D keycap never depresses or animates.

#### Root Cause
In Win32 low-level keyboard hooks (`WH_KEYBOARD_LL`), Windows assigns `tagKBDLLHOOKSTRUCT.vkCode = VK_RETURN` (`13` / `0x0D`) to both the main keyboard Enter key and the Numpad Enter key. The only differentiator provided by the OS is the `LLKHF_EXTENDED` flag in `tagKBDLLHOOKSTRUCT.flags` (set for Numpad Enter, clear for Main Enter).

In `FS Input Receiver`, `VKCode.cs` explicitly defines:
- `RETURN = 13`
- `NUMPAD_RETURN = 14` (utilizing the unassigned Win32 slot `0x0E`)

If the low-level hook callback does not check `LLKHF_EXTENDED`, Numpad Enter sets bit 13 instead of bit 14, starving the numpad keycap animation.

#### Solution
In `FsKeyboardTracker.dll`'s `LowLevelKeyboardProc`, explicitly intercept `VK_RETURN` with `LLKHF_EXTENDED` and remap it to `14`:
```cpp
if (vk == VK_RETURN && (info->flags & LLKHF_EXTENDED)) {
    vk = 14; // VKCode.NUMPAD_RETURN (0x0E) in FS Input Receiver
}
```
Additionally, defensive aliases are applied for synthetic macro software emitting generic `VK_SHIFT`, `VK_CONTROL`, or `VK_MENU` to map them to their corresponding left/right specific variants.

