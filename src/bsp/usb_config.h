#pragma once
// Pre-included for C++ builds: load the SDK pin map once, then override only
// USB identity before its USB.cpp creates the boot-time composite device.
#include <pins_arduino.h>

#ifndef MEOW_USB_VID
#define MEOW_USB_VID 0x303A
#endif
#ifndef MEOW_USB_PID
#define MEOW_USB_PID 0x0002
#endif

#undef USB_VID
#undef USB_PID
#define USB_VID MEOW_USB_VID
#define USB_PID MEOW_USB_PID
