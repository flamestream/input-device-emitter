# FS Input Device Emitter
An emitter that sends UDP packets with pointer and/or gamepad information.

Intended to be used in Warudo for tracking purpose with FS Input Receiver Plugin, but it's generic enough to be usable anywhere it fits.

## Usage Guide
Run executable. Command line flags available.

<img width="497" alt="image" src="https://github.com/flamestream/input-device-emitter/assets/1697502/ba135670-e9c6-4321-a49f-cd1db9aec3c5">

## Features
* Pointer type recognition (Pen vs. Mouse)
* Gamepad support

## Limitations
* As this hooks late to application, some signals may be innacurate or consumed by underlying and lost 
* Only one gamepad is supported. The first one detected will be hooked on

## Test Environment Information
* OS: Windows 11
* Pen-hovered apps
  * Clip Paint Studio
  * Blender
* Gamepad
  * Switch Pro Controller
