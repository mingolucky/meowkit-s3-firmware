#ifndef ESP32_BLE_MOUSE_H
#define ESP32_BLE_MOUSE_H
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "BleConnectionStatus.h"
#include "BLEHIDDevice.h"
#include "BLECharacteristic.h"

#ifndef MOUSE_LEFT
#define MOUSE_LEFT 1
#endif
#ifndef MOUSE_RIGHT
#define MOUSE_RIGHT 2
#endif
#ifndef MOUSE_MIDDLE
#define MOUSE_MIDDLE 4
#endif
#ifndef MOUSE_BACK
#define MOUSE_BACK 8
#endif
#ifndef MOUSE_FORWARD
#define MOUSE_FORWARD 16
#endif
#ifndef MOUSE_ALL
#define MOUSE_ALL (MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE) // For compatibility with the Mouse library
#endif

class BleMouse {
private:
  uint8_t _buttons;
  BleConnectionStatus* connectionStatus;
  BLEHIDDevice* hid;
  BLECharacteristic* inputMouse;
  void buttons(uint8_t b);
  void rawAction(uint8_t msg[], char msgSize);
  static void taskServer(void* pvParameter);
public:
  BleMouse(std::string deviceName = "ESP32 Bluetooth Mouse", std::string deviceManufacturer = "Espressif", uint8_t batteryLevel = 100);
  void begin(void);
  void end(void);
  void click(uint8_t b = MOUSE_LEFT);
  void move(signed char x, signed char y, signed char wheel = 0, signed char hWheel = 0);
  void press(uint8_t b = MOUSE_LEFT);   // press LEFT by default
  void release(uint8_t b = MOUSE_LEFT); // release LEFT by default
  bool isPressed(uint8_t b = MOUSE_LEFT); // check LEFT by default
  bool isConnected(void);
  void setBatteryLevel(uint8_t level);
  uint8_t batteryLevel;
  std::string deviceManufacturer;
  std::string deviceName;
protected:
  virtual void onStarted(BLEServer *pServer) { };
};

#endif // CONFIG_BT_ENABLED
#endif // ESP32_BLE_MOUSE_H
