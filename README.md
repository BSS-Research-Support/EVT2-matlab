# EVT2-matlab
MATLAB MEX interface for communicating with the EVT-2 HID device using HIDAPI.

This repository contains a Matlab binding to communicate with the EVT-2 (+derivatives) USB-device, developed by the Research Support group of the faculty of Behavioral and Social Science from the University of Groningen. The EVT-2 is a TTL event marking/TTL triggering device intended for computer-based psychology experiments.

## Features
- **Platform Independent:** Works on Windows and Linux.
- **Simple API:** Direct commands for `open`, `close`, `write`, `clear`, `set`, `pulse` and `read`.

## Prerequisites

### Windows
1. Install the MATLAB add-on: **MinGW-w64 compiler**.
2. Run `mex -setup` in MATLAB and select the MinGW64 compiler.
3. Download and unzip the HIDAPI drivers [https://github.com/libusb/hidapi/releases/tag/hidapi-0.15.0](https://github.com/libusb/hidapi/releases/tag/hidapi-0.15.0).
4. Copy `hidapi.dll` from inside `x64` to the main directory. The file structure now should look like:

```
EVT2-matlab/
│
├── .git
├── include
├── x64
├── x86
├── build_evt2.m
├── evt_pulse.m
├── evt_test.m
├── evt2.c
├── evt2.h
├── LICENSE
├── README.md
├── rsp_test.m
├── hidapi.dll
└── test_evt2.m
```

### Linux (Ubuntu/Debian)
1. Install the HIDAPI development library:
   ```bash
   sudo apt-get install libhidapi-dev
   ```
2. **Permissions:** You typically need a udev rule to access HID devices without root. Create `/etc/udev/rules.d/99-evt2.rules`:
   ```text
   SUBSYSTEM=="hidraw", ATTRS{idVendor}=="0808", ATTRS{idProduct}=="0001", GROUP="plugdev", MODE="0660"
   ```
   
   - reload rules: `sudo udevadm control --reload-rules && sudo udevadm trigger`

   - become member of the plugdev group `sudo usermod -aG plugdev $USER`

   - plugin the device

   - `ls -l /dev/hidraw0` must give: r+w access to group `crw-rw---- 1 root plugdev 241, 0 mei  4 14:33 /dev/hidraw0`

## Building the MEX file

A unified build script is provided. In the MATLAB Command Window, simply run:

```matlab
build_evt2
```

The script will automatically detect your OS, locate the necessary headers/libraries, and produce the `evt2` MEX file.

## Usage Example

```matlab
% Open the device (VID, PID)
evt2('open', 0x0808, 0x0001);

% Write a value
evt2('write', 1); 

% Pulse a value (value, duration_ms)
evt2('pulse', 255, 500);

% Read data
data = evt2('read');

% Close the device
evt2('close');
```


