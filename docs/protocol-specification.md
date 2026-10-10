# FS Input Device Emitter: Protocol Specification

This document details the network protocol and packet format used by **FS Input Device Emitter** (`FsInputEmitter.exe`) to stream input data over UDP to **FS Input Receiver** (or any custom UDP listener).

---

## 1. Network Overview

- **Transport**: UDP (User Datagram Protocol)
- **Host / IP**: `127.0.0.1` (localhost by default; configurable via CLI / interactive menu)
- **Encoding**: ASCII / UTF-8
- **Emission Rate**: Default `30ms` per packet (configurable via `--freq` / `-f`)
- **Protocol Version**: Version `2` (identified by prefix e.g., `2P`, `2K`, `2G`, `2D`, `2X`)

---

## 2. Default Ports & Packet Structure

| Channel | Default Port | Protocol Tag | Packet Format |
| :--- | :--- | :--- | :--- |
| **Pointer (Mouse & Pen)** | `40610` | `2P` | `2P;<x>;<y>;<source>;<mouse1>;<mouse2>` |
| **DirectInput (Generic Controller)** | `40611` | `2D` | `2D;<buttons>;<dpad0>\|<dpad1>\|<dpad2>\|<dpad3>;<axes>` |
| **Keyboard** | `40612` | `1K` | `1K;<part1>;<part2>;<part3>;<part4>` |
| **GameInput (Modern Xbox / Controller)** | `40613` | `2G` | `2G;<buttons>;<switches>;<axes>` |
| **XInput (Standard Controller)** | `40614` | `2X` | `2X;<buttons>;;<axes>` |

---

## 3. Channel Specifications

### 3.1 Pointer Channel (`40610` / `2P`)

Used to stream pointer coordinates, input device type, and primary/secondary button states.

#### Packet Syntax
```
2P;<x>;<y>;<source>;<mouse1>;<mouse2>
```

#### Fields
| Field | Type | Description |
| :--- | :--- | :--- |
| `x` | `int` | Global screen X coordinate (virtual screen space). |
| `y` | `int` | Global screen Y coordinate (virtual screen space). |
| `source` | `int` | Input source type:<br>• `0` = Mouse<br>• `1` = Pen / Stylus (e.g. Wacom Intuos Pro)<br>• `2` = Touch |
| `mouse1` | `int` (`0` or `1`) | Primary button / tip contact (`1` if pressed/down, `0` if released). |
| `mouse2` | `int` (`0` or `1`) | Secondary button / barrel button (`1` if pressed/down, `0` if released). |

#### Example
```text
2P;960;540;1;1;0
```

---

### 3.2 Keyboard Channel (`40612` / `2K`)

Transmits the state of all 256 Windows Virtual Key codes (`VK_...`) packed into four 64-bit binary strings (256 bits total).

#### Packet Syntax
```
2K;<part1>;<part2>;<part3>;<part4>
```

#### Fields
| Field | Type | Description |
| :--- | :--- | :--- |
| `part1` | `64-char string` (`'0'`/`'1'`) | Bits 0–63 representing VK codes 0x00 to 0x3F. |
| `part2` | `64-char string` (`'0'`/`'1'`) | Bits 64–127 representing VK codes 0x40 to 0x7F. |
| `part3` | `64-char string` (`'0'`/`'1'`) | Bits 128–191 representing VK codes 0x80 to 0xBF. |
| `part4` | `64-char string` (`'0'`/`'1'`) | Bits 192–255 representing VK codes 0xC0 to 0xFF. |

Bit value `'1'` indicates key is pressed; `'0'` indicates key is released.

> **Key Mapping Nuances**:
> - **Numpad Enter**: Windows assigns `VK_RETURN` (`0x0D` / bit 13) to both Enter and Numpad Enter, distinguishing them solely via `LLKHF_EXTENDED`. To allow downstream receivers (like FS Input Receiver) to animate Numpad Enter independently, Numpad Enter is remapped to synthetic VK code `14` (`0x0E` / bit 14, standard Win32 undefined slot).
> - **Synthetic Modifiers**: If synthetic macros emit generic `VK_SHIFT` (16), `VK_CONTROL` (17), or `VK_MENU` (18), the emitter defensively aliases them to their corresponding left/right variants based on scancode and `LLKHF_EXTENDED`.

#### Example
```text
2K;0000000000000000000000000000000000000000000000000000000000000000;...;...;...
```

---

### 3.3 GameInput Channel (`40613` / `2G`)

Reads modern Windows GameInput gamepads (Xbox Series X|S, Xbox One, or low-level generic controller mode).

#### Packet Syntax
```
2G;<buttons>;<switches>;<axes>
```

#### Fields (Gamepad Mode)
| Field | Type | Description |
| :--- | :--- | :--- |
| `buttons` | `32-char string` (`'0'`/`'1'`) | 32-bit bitset of GameInput gamepad buttons (DPad, A, B, X, Y, Shoulders, Thumbs, View, Menu). |
| `switches` | `string` | Controller switches state (or empty string in high-level gamepad mode). |
| `axes` | Pipe-delimited floats | `LeftThumbX\|LeftThumbY\|RightThumbX\|RightThumbY\|LeftTrigger\|RightTrigger`<br>• Thumbsticks: Normalized `0.0` to `1.0` (center is `0.5`).<br>• Triggers: `0.0` to `1.0`. |

#### Example
```text
2G;00000000000000000000000000000000;;0.500000|0.500000|0.500000|0.500000|0.000000|0.000000
```

---

### 3.4 DirectInput Channel (`40611` / `2D`)

Captures legacy DirectInput devices (flight sticks, steering wheels, pedals, and generic gamepads).

#### Packet Syntax
```
2D;<buttons>;<dpad0>|<dpad1>|<dpad2>|<dpad3>;<lx>|<ly>|<lz>|<lrx>|<lry>|<lrz>|<slider0>|<slider1>
```

#### Fields
| Field | Type | Description |
| :--- | :--- | :--- |
| `buttons` | `32-char string` (`'0'`/`'1'`) | State of buttons 0 to 31. |
| `dpad0..3` | `short` (`0`–`8`) | POV hat directional state (`0` = centered, `1` = Up, `2` = Up-Right, `3` = Right, `4` = Down-Right, `5` = Down, `6` = Down-Left, `7` = Left, `8` = Up-Left). |
| `axes` | 8 pipe-delimited floats | `lX\|lY\|lZ\|lRx\|lRy\|lRz\|slider0\|slider1` normalized between `0.0` and `1.0`. |

---

### 3.5 XInput Channel (`40614` / `2X`)

Captures standard XInput controllers (Xbox 360 / Xbox One).

#### Packet Syntax
```
2X;<buttons>;;<axes>
```

#### Fields
| Field | Type | Description |
| :--- | :--- | :--- |
| `buttons` | `32-char string` (`'0'`/`'1'`) | 16-bit XInput buttons padded to 32 bits. |
| `switches` | Empty | Reserved (empty field between delimiters). |
| `axes` | 6 pipe-delimited floats | `LeftThumbX\|LeftThumbY\|RightThumbX\|RightThumbY\|LeftTrigger\|RightTrigger`<br>• Thumbsticks: Normalized `0.0` to `1.0` (center is `0.5`).<br>• Triggers: `0.0` to `1.0`. |

---

## 4. Error Handling and Backward Compatibility

- The receiver accepts legacy single-number protocols (e.g. `2`) for backward compatibility, but modern format `<version><ID>` (e.g., `2P`, `2G`, `2K`) is enforced when letters are present to prevent misconfigured port collisions.
- Socket exceptions and dropped packets are safely swallowed by design since UDP is loss-tolerant.
