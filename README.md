# FS Input Device Emitter
An emitter that sends UDP packets with pointer, keyboard and/or gamepad information.

Intended to be used in Warudo for tracking purpose with FS Input Receiver Plugin, but it's generic enough to be usable anywhere it fits.

## Usage Guide
Run executable. Command line flags available.

For discoverability and the less tech-savvy, an interactive mode is presented when executing the program without any options.

https://github.com/user-attachments/assets/08d56584-8d5f-461b-a46c-56f2c83cf8fe

## Features
* Pointer type recognition (Pen vs. Mouse)
* Controller support (DirectInput and GameInput)
* Keyboard support
* General Input devices (Wheel, pedal, handle, joystick, etc.)
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
* Controller
  * Switch Pro Controller
  * Playstation 4
  * Playstation 5
  * Xbox 360
  * Xbox One
* Misc
  * Moza R9 wheel base
  * Moza CS steering wheel
