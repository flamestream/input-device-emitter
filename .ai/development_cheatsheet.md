# Development Cheatsheet — FS Input Device Emitter

This document provides quickstart recipes, API patterns, and coding conventions for extending and maintaining **FS Input Device Emitter**.

---

## 1. Quick Build & Test Commands

### One-Command PowerShell Build
```powershell
# From repository root (d:\Code\input-device-emitter)
.\build.ps1 -Configuration Release -Platform x64

# Force clean rebuild in Debug mode
.\build.ps1 -Configuration Debug -Platform x64 -Rebuild
```

### Direct MSBuild Commands
```powershell
# 1. Restore NuGet dependencies (Microsoft.GameInput)
.\nuget.exe restore InputEmitter.sln

# 2. Build Release x64
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" InputEmitter.sln /p:Configuration=Release /p:Platform=x64 /m
```

### Running Test Clients
```powershell
# Run the test UDP listener (.NET 8)
dotnet run --project UdpClientTest\UdpClientTest.csproj

# Run the emitter with debug logging enabled
.\x64\Release\FsInputEmitter.exe --debug -p -k
```

---

## 2. Solution Structure & Project Roles

```
input-device-emitter/
├── Emitter/              # FsInputEmitter.exe console application
│   ├── Program.cpp       # CLI parsing, Inquirer wizard, main loop, emission dispatcher
│   ├── Emitter.cpp/.h    # Winsock UDP broadcaster (SO_BROADCAST)
│   ├── *Tracker.cpp/.h   # Input tracking classes (Mouse, Keyboard, DirectInput, etc.)
│   ├── Console.cpp/.h    # ANSI color console logging helper
│   ├── Utility.cpp/.h    # String & HRESULT formatters
│   └── Version.h         # Version constants (VERSION_MAJOR, etc.)
├── MouseTracker/         # FsMouseTracker.dll (WH_MOUSE_LL hook)
│   └── dllmain.cpp       # Shared segment, mouse hook proc, GetStateProc export
├── KeyboardTracker/      # FsKeyboardTracker.dll (WH_KEYBOARD_LL hook)
│   └── dllmain.cpp       # Shared segment, keyboard hook proc, GetStateProc export
├── GamepadDebug/         # FsGamepadDebug.exe (standalone raw joystick diagnostic)
├── UdpClientTest/        # C# .NET 8 UDP listener for packet validation
└── docs/                 # Public documentation
```

---

## 3. Tracker Class Lifecycle Pattern

All input tracker implementations follow a standardized lifecycle contract:

```cpp
#pragma once
#include <string>

class CustomTracker {
public:
    static const unsigned short PROTOCOL_VERSION = 1;

    std::string state;      // Current formatted UDP payload string
    std::string lastError;  // Descriptive error message for UI/logs

    CustomTracker();
    bool setup();           // Initialize devices, load DLLs, install hooks
    void teardown();        // Unhook, release COM interfaces, free libraries
    bool refreshState();    // Poll device or read shared state
    void writeState();      // Format the state string according to protocol
};
```

### Implementation Checklist
1. **Constructor**: Initialize member pointers to `nullptr` / `0`, set initial state to default resting values.
2. **`setup()`**:
   - Check return values / `HRESULT`s.
   - Populate `lastError` with `GetLastError()` or `Utility::hr2str(hr)` on failure.
   - Return `false` on failure (do not throw exceptions).
3. **`refreshState()`**:
   - Should be non-blocking and low-latency (called every 30 ms).
   - Only format new state (`writeState()`) if the hardware state actually changed, or format unconditionally if continuous updates are needed.
4. **`teardown()`**:
   - Must be idempotent and safe to call multiple times or when `setup()` partially failed.
   - Check `if (!this) return;` or `if (ptr) { ptr->Release(); ptr = nullptr; }`.

---

## 4. Hook DLL Pattern (`dllmain.cpp`)

To capture global input without injecting DLLs into every process, use low-level hooks (`WH_MOUSE_LL` / `WH_KEYBOARD_LL`) combined with a shared memory segment:

```cpp
#include "pch.h"
#include <Windows.h>

// 1. Shared data segment (shared across all process mappings of this DLL)
#pragma data_seg (".shared_custom")
LONG sharedValue = 0;
#pragma data_seg()
#pragma comment(linker, "/SECTION:.shared_custom,RWS")

// 2. Struct matching host executable definition
struct State {
    LONG value;
};

// 3. Exported state getter
extern "C" __declspec(dllexport) State GetStateProc() {
    State out = { sharedValue };
    return out;
}

// 4. Exported hook procedure
extern "C" __declspec(dllexport) LRESULT CALLBACK LowLevelCustomProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        // Fast, minimal processing
        sharedValue = 123;
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
```

> [!WARNING]
> Keep code inside `LowLevel...Proc` extremely minimal! Any delay or blocking call in the hook procedure causes system-wide input lag or triggers Windows `LowLevelHooksTimeout` to drop the hook.

---

## 5. Adding CLI Options (`cxxopts` & Inquirer)

When adding a new command-line option, update both the CLI parser and the interactive Inquirer wizard in [`Emitter/Program.cpp`](file:///d:/Code/input-device-emitter/Emitter/Program.cpp):

### 1. Register with `cxxopts`:
```cpp
options.add_options()
    ("c,custom", "Enable custom input tracker")
    ("custom-port", "Custom emitter port", cxxopts::value<unsigned short>()->default_value("40615"))
    ("custom-target", "Target IP for custom emitter", cxxopts::value<std::string>()->default_value(Emitter::DEFAULT_IP_ADDRESS))
```

### 2. Parse from `result`:
```cpp
if (result.count("custom")) {
    isCustomWanted = true;
    portCustom = result["custom-port"].as<unsigned short>();
    ipAddressCustom = result["custom-target"].as<std::string>();
}
```

### 3. Add to Inquirer Interactive Flow:
```cpp
if (!isCustomWanted) choices.push_back(INQUIRE_TYPE_CUSTOM);
// ... handle question answer and target prompt
```

---

## 6. Console Logging Conventions (`Console.h`)

Always use the static `Console` wrapper instead of raw `std::cout` for consistent ANSI styling:

```cpp
#include "Console.h"

Console::log("Normal informational message");
Console::success("Device connected successfully");
Console::warn("Warning: You may need to reconnect the device");
Console::error(std::format("Failed to initialize device: {}", lastError));
Console::info("Instructional prompt");
```

---

## 7. C++20 Standards & Coding Conventions

- **MSVC Standard**: Compiled with `/std:c++20`.
- **String Formatting**: Prefer `std::format(...)` over `sprintf` or `stringstream` where available.
- **Header Guards**: Use `#pragma once` at the top of all header files.
- **Resource Management**: Follow RAII or explicit `setup()`/`teardown()` pairs. Clean up Winsock and COM interfaces on exit.
