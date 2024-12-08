# FS Input Device Emitter
An emitter that sends UDP packets with pointer, keyboard and/or gamepad information.

Intended to be used in Warudo for tracking purpose with FS Input Receiver Plugin, but it's generic enough to be usable anywhere it fits.

## Usage Guide
Run executable. Command line flags available.

<img src="https://github.com/user-attachments/assets/1cb4f8c0-69ae-442d-9f2a-0814e20c19fc" width="600" />

## Features
* Pointer type recognition (Pen vs. Mouse)
* Gamepad support (DirectInput)
* Keyboard support
* Remote PC support

## Limitations
* As this hooks to application late through global hook, some signals may be innacurate or lost 
* Touch not supported yet; It is recognized as Pen

## Test Environment Information
* OS
  * Windows 11
* Pen Device
  * Wacom Intuos Pro
* Pen-hovered apps
  * Clip Paint Studio
  * Blender
* Gamepad
  * Switch Pro Controller
  * Playstation 5
  * Xbox 360
