// device_id.h
#ifndef DEVICE_ID_H
#define DEVICE_ID_H

#include <Arduino.h>

/**
 * @brief Get unique device identifier from MAC address.
 * @return Device ID as 12-character hex string (e.g., "AABBCC112233")
 * @note Thread-safe: Reads WiFi MAC address (read-only operation)
 * @note Returns uppercase hex string without colons or separators
 */
String getDeviceId();

#endif