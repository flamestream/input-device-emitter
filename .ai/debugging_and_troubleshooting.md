# Debugging & Troubleshooting — FS Input Device Emitter

This guide outlines practical debugging strategies, testing tools, common error signatures, and diagnosis workflows for **FS Input Device Emitter**.

---

## 1. Diagnostic Tools in the Repository

The solution includes multiple tools to verify hardware input and network packets without needing Warudo running:

### 1.1 `UdpClientTest` (.NET 8 Test Listener)
Located in [`UdpClientTest/Program.cs`](file:///d:/Code/input-device-emitter/UdpClientTest/Program.cs). Listens on port `40610` (or modify to any port) and prints incoming UDP datagrams in real time:

```powershell
# Run from repository root
dotnet run --project UdpClientTest\UdpClientTest.csproj
```

### 1.2 `GamepadDebug` (Raw DirectInput Diagnostic)
Located in [`GamepadDebug/Program.cpp`](file:///d:/Code/input-device-emitter/GamepadDebug/Program.cpp). Compiles to `x64/Release/FsGamepadDebug.exe` and outputs raw joystick axis positions and button bitmasks:

```powershell
# Inspect gamepad at index 0 at 50ms intervals
.\x64\Release\FsGamepadDebug.exe --gamepad-index 0 --frequency 50
```

### 1.3 Lightweight PowerShell UDP Listener (One-Liner)
If you need to quickly inspect packets on any specific port without compiling:

```powershell
# Listen on Pointer port (40610)
$port = 40610
$client = New-Object System.Net.Sockets.UdpClient($port)
$endpoint = New-Object System.Net.IPEndPoint([System.Net.IPAddress]::Any, 0)
Write-Host "Listening for UDP packets on port $port... (Ctrl+C to stop)" -ForegroundColor Cyan
try {
    while ($true) {
        $bytes = $client.Receive([ref]$endpoint)
        Write-Host [$(Get-Date -Format 'HH:mm:ss.fff')] ([System.Text.Encoding]::ASCII.GetString($bytes))
    }
} finally {
    $client.Close()
}
```

---

## 2. Emitter CLI Debugging Flags

When running `FsInputEmitter.exe` from PowerShell or Command Prompt, use the built-in diagnostic flags:

| Flag | Description | Typical Use Case |
|:---|:---|:---|
| `--debug` | Prints active tracker state strings to console in real-time. | Verify that coordinates, keys, and axes are changing as you interact. |
| `--show-shutdown-message` | Keeps console window open until Enter is pressed upon program termination. | Prevent terminal from disappearing when launched by double-clicking in Explorer. |
| `-f <ms>`, `--freq <ms>` | Sets emission interval in milliseconds (default: `30`). | Test high-rate tracking (`-f 10`) or reduce network bandwidth (`-f 60`). |
| `-p`, `--pointer` | Enables pointer channel only. | Isolates mouse/tablet troubleshooting. |
| `-k`, `--keyboard` | Enables keyboard channel only. | Isolates keycode bitset troubleshooting. |

#### Example Debug Command
```powershell
.\x64\Release\FsInputEmitter.exe --debug -p --pointer-port 40610
```

---

## 3. Common Error Signatures & Solutions

### 3.1 DLL Loading Failure (`Error 126: ERROR_MOD_NOT_FOUND`)
- **Symptom**:
  ```text
  Mouse Tracker DLL could not be loaded: Error 126
  ```
- **Cause**: `FsMouseTracker.dll` or `FsKeyboardTracker.dll` cannot be found in the current working directory or application directory.
- **Fix**:
  1. Ensure you ran `.\build.ps1` to build all projects into `x64\Release\`.
  2. Both `FsMouseTracker.dll` and `FsKeyboardTracker.dll` must be co-located in the same directory as `FsInputEmitter.exe`.
  3. If running from VS or PowerShell, verify your working directory (`Set-Location x64\Release`).

---

### 3.2 Inputs Ignored When Focused on Elevated Windows (UIPI)
- **Symptom**: Mouse movements and keyboard strokes stop being captured when the user clicks on Task Manager, elevated Command Prompt, or an installer.
- **Cause**: Windows **User Interface Privilege Isolation (UIPI)** blocks lower-integrity processes from intercepting inputs belonging to higher-integrity (Administrator) processes.
- **Fix**: Run `FsInputEmitter.exe` from an **Elevated (Administrator)** PowerShell / Command Prompt window.

---

### 3.3 GameInput Fails to Get Reading
- **Symptom**:
  ```text
  GameInput has failed to get reading: ...
  To fix this, you may need to reconnect the controller
  ```
- **Cause**: The COM interface lost connection to the device or the controller went to sleep.
- **Fix**: Unplug and reconnect the controller USB cable or cycle Bluetooth pairing. Alternatively, switch to XInput mode (`-x`) if the device supports standard XInput.

---

### 3.4 Winsock Socket Error (`Error 10048: WSAEADDRINUSE`)
- **Symptom**:
  ```text
  Error sending message: 10048
  ```
- **Cause**: Another process or a stale instance of `FsInputEmitter.exe` is already holding the target socket/port.
- **Fix**:
  ```powershell
  # Check which process holds port 40610
  Get-NetUDPEndpoint -LocalPort 40610 | Select-Object OwningProcess
  # Terminate orphan emitter processes
  Get-Process FsInputEmitter -ErrorAction SilentlyContinue | Stop-Process -Force
  ```

---

### 3.5 Stylus / Pen Input Detected as Mouse
- **Symptom**: Tablet strokes arrive with `source = 0` (Mouse) instead of `source = 1` (Pen).
- **Cause**: Tablet driver is operating in standard HID mouse emulation mode rather than Windows Ink / WinTab mode, leaving `dwExtraInfo == 0`.
- **Fix**:
  1. Open Wacom Tablet Properties (or your tablet driver software).
  2. Ensure "Use Windows Ink" is checked for the target drawing application.
  3. Verify tablet driver service (e.g. `Wacom Professional Service`) is running.

---

## 4. Network Packet Inspection with Wireshark

To confirm datagrams are exiting the network interface:
1. Open Wireshark and bind to the **Adapter for loopback traffic** (if targeting `127.0.0.1`) or your active Ethernet/Wi-Fi adapter.
2. Apply display filter:
   ```text
   udp.port >= 40610 and udp.port <= 40614
   ```
3. Look for datagram payloads beginning with `1P;`, `1K;`, `3D;`, `2G;`, or `1X;`.

---

## 5. Attaching a Debugger (Visual Studio / WinDbg)

1. Open `InputEmitter.sln` in Visual Studio 2022.
2. Set configuration to `Debug` and platform to `x64`.
3. In **Solution Explorer**, right-click `Emitter` → **Properties** → **Debugging**:
   - Set **Command Arguments** (e.g., `--debug -p -k`).
   - Set **Working Directory** to `$(OutDir)`.
4. Press **F5** to start debugging. Breakpoints can be set in `Program.cpp` or directly inside `MouseTracker/dllmain.cpp` and `KeyboardTracker/dllmain.cpp`.
