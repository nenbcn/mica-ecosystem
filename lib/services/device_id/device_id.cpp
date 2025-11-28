// device_id.cpp
// Device ID Module
// Purpose: Generates unique device identifier from MAC address
// Architecture: Simple utility function, no state
// Thread-Safety: Read-only WiFi MAC access (thread-safe)
// Dependencies: WiFi library

#include "device_id.h"

#include <Arduino.h>
#include <WiFi.h>

String getDeviceId() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char deviceId[13]; // 12 chars for MAC without colons + null terminator
    snprintf(deviceId, sizeof(deviceId), "%02X%02X%02X%02X%02X%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(deviceId);
}