# Protocol & Networking Guide — FS Input Device Emitter

This document provides the definitive specification for the UDP packet protocols, port allocations, data serialization formats, and network behaviors used by **FS Input Device Emitter** and received by **FS Input Receiver** (Warudo).

---

## 1. Network Architecture Overview

- **Transport**: UDP (User Datagram Protocol), IPv4
- **Default Target**: `127.0.0.1` (localhost / loopback)
- **Supported Targets**: Any valid IPv4 address, LAN subnet broadcast, or `255.255.255.255`
- **Emission Rate**: Configurable via `--freq` / `-f` (Default: `30ms` / ~33.3 Hz)
- **Encoding**: ASCII / UTF-8 text datagrams
- **Delimiters**: Semicolon (`;`) separates top-level packet fields; Pipe (`|`) separates sub-fields (such as axis lists or D-pad states).

---

## 2. Channel & Port Allocation Summary

Each input device category operates over its own dedicated UDP port and protocol tag to prevent packet collisions and simplify receiver filtering:

| Channel / Device | Default Port | Active Protocol Tag | Tag Meaning | Payload Summary |
|:---|:---:|:---:|:---|:---|
| **Pointer (Mouse & Pen)** | `40610` | `1P` | Protocol V1, Pointer | Coordinates, input source, mouse button 1 & 2 |
| **DirectInput (Flight / Wheel / Gamepad)** | `40611` | `3D` | Protocol V3, DirectInput | 32 buttons bitstring, 4 POV hats, 8 float axes |
| **Keyboard** | `40612` | `1K` | Protocol V1, Keyboard | 256 Virtual Key states in four 64-bit binary strings |
| **GameInput (Modern Xbox / Generic)** | `40613` | `2G` | Protocol V2, GameInput | 32-bit buttons, switch states, 6 normalized float axes |
| **XInput (Standard Xbox 360 / One)** | `40614` | `1X` | Protocol V1, XInput | 32-bit buttons, empty switches, 6 normalized float axes |

> [!IMPORTANT]
> **Active Code vs. Legacy Docs**: Some historical documentation references a generic "Version 2" for all devices (`2P`, `2K`, `2X`). In actual runtime code, **each tracker defines its own protocol version**: `MouseTracker` uses `1P`, `KeyboardTracker` uses `1K`, `DirectInputTracker` uses `3D`, `GameInputTracker` uses `2G`, and `XInputTracker` uses `1X`. Receivers validate the exact tag defined by each tracker.

---

## 3. Channel Specifications & Packet Layouts

### 3.1 Pointer Channel (`40610` / `1P`)

Streams global mouse cursor position, hardware input source discriminator, and primary/secondary button states.

#### Packet Format
```text
1P;<x>;<y>;<source>;<mouse1>;<mouse2>
```

#### Fields
| Field | Type | Description |
|:---|:---|:---|
| `x` | `int` | Global virtual screen X coordinate in pixels. Can be negative on multi-monitor setups. |
| `y` | `int` | Global virtual screen Y coordinate in pixels. Can be negative on multi-monitor setups. |
| `source` | `int` | Input source type:<br>• `0` = Mouse<br>• `1` = Pen / Stylus (e.g. Wacom Intuos Pro)<br>• `2` = Touch *(reserved/detected as Pen)* |
| `mouse1` | `int` | Primary button state (`1` = pressed / tip contact, `0` = released). |
| `mouse2` | `int` | Secondary button state (`1` = pressed / barrel button, `0` = released). |

#### Example
```text
1P;1920;1080;1;1;0
```

---

### 3.2 Keyboard Channel (`40612` / `1K`)

Streams the state of all 256 Windows Virtual Key codes (`VK_...`, `0x00` through `0xFF`) packed into four 64-bit binary strings.

#### Packet Format
```text
1K;<part1>;<part2>;<part3>;<part4>
```

#### Fields
| Field | Type | Description |
|:---|:---|:---|
| `part1` | 64-char string (`'0'`/`'1'`) | Virtual keys `0x00` to `0x3F` (Bits 0–63). |
| `part2` | 64-char string (`'0'`/`'1'`) | Virtual keys `0x40` to `0x7F` (Bits 64–127). |
| `part3` | 64-char string (`'0'`/`'1'`) | Virtual keys `0x80` to `0xBF` (Bits 128–191). |
| `part4` | 64-char string (`'0'`/`'1'`) | Virtual keys `0xC0` to `0xFF` (Bits 192–255). |

- Character `'1'` indicates key is currently pressed; `'0'` indicates key is released.
- Bit indexing within each 64-char string is formatted MSB to LSB by `std::bitset<64>::to_string()`.

#### Example
```text
1K;0000000000000000000000000000000000000000000000000000000000000000;0000000000000000000000000000000000000000000000000000000000000000;...;...
```

---

### 3.3 GameInput Channel (`40613` / `2G`)

Streams modern Windows GameInput controller data. Supports both high-level gamepad mode and low-level generic controller mode.

#### Packet Format
```text
2G;<buttons>;<switches>;<axes>
```

#### Fields (High-level Gamepad Mode)
| Field | Type | Description |
|:---|:---|:---|
| `buttons` | 32-char string (`'0'`/`'1'`) | Bitset representing GameInput gamepad buttons (DPad, A, B, X, Y, Shoulders, Thumbs, View, Menu). |
| `switches` | string | Empty in gamepad mode (reserved for low-level switch states). |
| `axes` | 6 pipe-delimited floats | `LeftThumbX\|LeftThumbY\|RightThumbX\|RightThumbY\|LeftTrigger\|RightTrigger`<br>• Thumbsticks: Normalized `0.0` to `1.0` (center resting is `0.5`).<br>• Triggers: Normalized `0.0` to `1.0` (unpressed is `0.0`). |

#### Fields (Low-level Generic Controller Mode)
- `buttons`: Arbitrary length bitstring matching `GetControllerButtonCount()`.
- `switches`: String of switch positions (`0` to `8` per switch).
- `axes`: Pipe-delimited floats for all raw analog axes on the device.

#### Example (Gamepad Mode Resting)
```text
2G;00000000000000000000000000000000;;0.500000|0.500000|0.500000|0.500000|0.000000|0.000000
```

---

### 3.4 DirectInput Channel (`40611` / `3D`)

Streams legacy DirectInput 8 controller data, including steering wheels, pedals, flight sticks, and generic gamepads.

#### Packet Format
```text
3D;<buttons>;<dpad0>|<dpad1>|<dpad2>|<dpad3>;<lx>|<ly>|<lz>|<lrx>|<lry>|<lrz>|<slider0>|<slider1>
```

#### Fields
| Field | Type | Description |
|:---|:---|:---|
| `buttons` | 32-char string (`'0'`/`'1'`) | State of buttons 0 through 31. |
| `dpad0..3` | 4 pipe-delimited numbers | Directional state of POV hats 0–3:<br>• `0` = Centered / Released<br>• `1` = Up (0°)<br>• `2` = Up-Right (45°)<br>• `3` = Right (90°)<br>• `4` = Down-Right (135°)<br>• `5` = Down (180°)<br>• `6` = Down-Left (225°)<br>• `7` = Left (270°)<br>• `8` = Up-Left (315°) |
| `axes` | 8 pipe-delimited floats | `lX\|lY\|lZ\|lRx\|lRy\|lRz\|slider0\|slider1`<br>• Each normalized between `0.0` and `1.0`.<br>• Centered sticks rest at `0.500000`. |

#### Example
```text
3D;00000000000000000000000000000000;0|0|0|0;0.500000|0.500000|0.500000|0.500000|0.500000|0.500000|0.000000|0.000000
```

---

### 3.5 XInput Channel (`40614` / `1X`)

Streams standard XInput controller state (Xbox 360 / Xbox One / standard PC gamepads).

#### Packet Format
```text
1X;<buttons>;;<axes>
```

#### Fields
| Field | Type | Description |
|:---|:---|:---|
| `buttons` | 32-char string (`'0'`/`'1'`) | 16-bit XInput buttons (`wButtons`) zero-padded to 32 bits. |
| `switches` | Empty | Reserved (empty field between two semicolons). |
| `axes` | 6 pipe-delimited floats | `LeftThumbX\|LeftThumbY\|RightThumbX\|RightThumbY\|LeftTrigger\|RightTrigger`<br>• Thumbsticks: Normalized `0.0` to `1.0` (center resting is `0.5`).<br>• Triggers: Normalized `0.0` to `1.0`. |

#### Example
```text
1X;00000000000000000000000000000000;;0.500000|0.500000|0.500000|0.500000|0.000000|0.000000
```

---

## 4. Coordinate System & Screen Space

### Virtual Desktop Coordinates (`MouseTracker`)
- Coordinate space is determined by Windows Virtual Screen bounds (`GetSystemMetrics(SM_CXVIRTUALSCREEN)` and `SM_CYVIRTUALSCREEN`).
- **Primary Monitor**: Origin `(0, 0)` is located at the top-left corner of the primary display.
- **Secondary Monitors**: Monitors positioned to the left of or above the primary monitor produce **negative coordinates** (e.g. `x = -1920`).
- **Warudo Consumption**: The `PointerReceiverAsset` in Warudo uses reference screen bounds and monitor calibration offsets to map these global coordinates onto avatar props or virtual drawing tablets.

---

## 5. Backward Compatibility & Receiver Handling

Warudo's `ReceiverAsset` handles incoming packets with defensive validation:

1. **Tag Inspection (`CheckValidProtocolVersion`)**:
   - Compares the tag against `<expectedVersion><expectedName>` (e.g., `1P`, `1K`, `3D`, `2G`, `1X`).
   - If the incoming tag contains letters, strict version and channel verification is enforced to prevent mismatched packets (e.g., sending keyboard packets to a pointer port).
   - If the tag is a single integer without letters (legacy V1/V2), it falls back to numeric validation for backward compatibility.
2. **Loss Tolerance**:
   - UDP is connectionless and loss-tolerant. Dropped packets are silently skipped; receivers simply retain the most recent state until the next packet arrives.
   - Sockets in Warudo use `SO_REUSEADDR` to permit instant rebound across scene reloads.
